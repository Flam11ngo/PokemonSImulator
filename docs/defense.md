# PokemonSimulator — 答辩文档

> 宝可梦在线对战平台 + 大数据分析系统
> 总代码量: ~18,000 行 (56 Python + 30 JS + 34 Vue)

---

## 一、项目概述

这是一个完整的**全栈宝可梦对战平台**，包含实时 PvP 对战、Smogon 数据分析、大数据埋点管道三大子系统。

**技术栈**: Python (FastAPI) + Node.js (Showdown引擎) + Vue 3 + Kafka + SQLite + Nginx

**物理部署**: Windows 开发机 + Ubuntu VM 集群 (Kafka/桥/Stats API) + 阿里云生产服务器

---

## 二、系统架构

```
┌─────────────────────────────────────────────────────────┐
│                    用户浏览器 (:80)                        │
│    Vue 3 SPA — 组队 / 匹配 / 对战 / 统计 / 分析           │
└──────────┬────────────────────────────────┬─────────────┘
           │                                │
    页面浏览/点击打点                   WebSocket 对战
    (HTTP POST)                        (双向通信)
           │                                │
┌──────────▼────────────────────────────────▼─────────────┐
│              standalone_server.py (:9000)                 │
│  路由分发:                                                │
│    /api/v1/smogon/*  → Smogon 数据分析 (SQLite查询)       │
│    /api/v1/stats/*   → 透传 VM stats API                 │
│    /api/v1/analytics → 埋点接收 → Kafka 转发              │
│    /ws               → 对战 WebSocket (Showdown引擎)      │
│    /api/v1/battles   → 对战CRUD                          │
└──────┬────────────────────┬────────────────────┬─────────┘
       │                    │                    │
  Kafka Producer      Showdown Daemon      Smogon DB
  (kafka_producer.py)  (Node.js子进程)     (SQLite 335MB)
       │                    │
       └────────┬───────────┘
                │
┌───────────────▼──────────────────────────────────────────┐
│              VM 集群 (myz, Ubuntu)                        │
│                                                           │
│  Kafka Broker (:9092)                                     │
│  ├─ battle.logs        (3分区, 对战全生命周期)             │
│  └─ player.ui.events   (3分区, 用户行为全量)              │
│                                                           │
│  kafka_to_sqlite.py (ADS Bridge)                         │
│  │ 消费 → 聚合 → 写入 SQLite                              │
│                                                           │
│  stats_server.py (:8080, HTTP API)                       │
│  │ 查询 pokemon_stats.db → JSON 响应                      │
│                                                           │
│  /opt/bigdata/pokemon_stats.db (14张表)                   │
└──────────────────────────────────────────────────────────┘

同时还有:
  data_daemon.js (Windows 开发机)
    8 Battle Workers + 3 UI Workers
    Showdown 引擎模拟 → Kafka
    Smogon 加权随机队伍
```

---

## 三、核心模块逻辑

### 3.1 对战引擎 (engine-adapter/showdown_daemon.js)

基于 Pokemon Showdown 开源引擎。核心流程:

```
1. Battle.join(p1, p2, teams)  → 双方加入
2. battle.choose("p1", "team 1") → 首发选择
3. 循环每回合:
   a. battle.choose(p1, "move N") + battle.choose(p2, "move N")
   b. buildState(battle) → 提取双方精灵状态
   c. parseLog(battle) → 解析 Showdown 协议行
      识别的协议行:
        |switch|POKEMON|DETAILS    → switch_in 事件
        |move|ATTACKER|MOVE|TARGET → 暂存 move 上下文
        |-damage|POKEMON|CUR/MAX   → damage/dot 事件 (含物种追踪)
        |faint|POKEMON             → faint 事件 (含物种追踪)
        |-heal|POKEMON|AMOUNT      → heal 事件
        |-status|POKEMON|STATUS    → status_apply
        |-unboost|POKEMON|STAT     → stat_drop
        |-ability|POKEMON|ABILITY  → ability_trigger
   d. 通过 WebSocket 将 state + events 推给前端
4. 濒死强制换人: 检测 activeRequest.forceSwitch → battle.choose(p, "switch N")
5. 结束: battle.ended → 计算存活数 → 判定胜负
```

**关键设计**: 动画由状态驱动而非事件驱动。用 `_lastSwitch`、`_lastHp` 等指纹去重，避免 Showdown 重复协议行导致动画重播。

### 3.2 前端对战流程 (MatchmakingPage.vue)

```
setupWS() → WebSocket 连接
  ├─ "join_match" → 进入匹配队列 (bot/human)
  ├─ "matched" → 对战创建完成
  │     ├─ trackBattleInit(battle_id, side, teamA, teamB, oppType)
  │     └─ 切换 UI 到对战状态
  ├─ "turn_processed" → 每回合结果
  │     ├─ 更新 battleState
  │     ├─ 解析 events → trackSwitch/trackFaint/trackDamage
  │     └─ 检查 need2switch → 自动切换到换人模式
  ├─ "battle_state_update" → 状态增量推送
  └─ "force_switch_done" → 强制换人完成
```

**埋点覆盖**:
| 时机 | 函数 | 追踪数据 |
|------|------|---------|
| 匹配创建 | `trackBattleInit` | 双方完整6只精灵 (speciesID/moves/item/ability/nature) |
| 每回合 | `trackTurnExecuted` | 双方动作 |
| 换人 | `trackSwitch` | 换上精灵的 speciesId、side、forced/manual |
| 濒死 | `trackFaint` | 倒下精灵的 speciesId、side |
| 伤害 | `trackDamage` | 目标 speciesId、招式名、伤害值、是否致死 |
| 结束 | `trackBattleResult` | 胜负、剩余精灵数、总回合 |

### 3.3 模拟数据生成器 (data_daemon.js)

```
main():
  └─ 8个 BattleWorker + 3个 UIWorker 并发运行
      │
      BattleWorker:
        1. randomTeam() → Smogon加权随机生成3只精灵的队伍
        2. new Battle() + battle.join() → Showdown引擎初始化
        3. 每回合: battle.choose("move N") → send("battle.logs", ...)
           发送: battle_init / turn_executed / turn_damage / turn_faint
                / turn_switch / battle_result
        4. 100-300ms间隔 (模拟真实对战速率)
      │
      UIWorker:
        每3-8秒随机发送: page_view / ui_click / player_state
                        / matchmaking_join / session_start / team_save
```

**Smogon加权队伍生成**:
```
randomTeam():
  1. 从 smogon_meta.json 加权随机选3只精灵 (usage%为权重)
  2. 每只精灵:
     - 技能: 从Top10 Smogon技能中加权选4个 (仅选可学的)
     - 道具: 从Top10 Smogon道具中加权选1个
     - 特性: 从Top10 Smogon特性中加权选1个
     - 努力值: 从Top10 Smogon努力分配中加权选1组
  3. teamToInit() → 转换为 Showdown 引擎初始化格式
```

### 3.4 Kafka 消息格式

所有消息**统一 UTC ISO 8601 时间戳**。

**battle.logs 消息类型**:
```
battle_init     — 每场1次，含双方6只精灵完整信息
turn_executed   — 每回合1次，含双方当前HP+物种ID
turn_damage     — 含受伤方物种ID、招式名、伤害值
turn_faint      — 含倒下精灵物种ID
turn_switch     — 含换上精灵物种ID、forced/manual
battle_result   — 含胜者、总回合、双方剩余数
```

**player.ui.events 消息类型**:
```
page_view       — 页面路径
ui_click        — 按钮/导航元素ID
player_state    — idle→matching→battling→teambuilding 状态转换
session_start   — 会话开始
team_save       — 保存的队伍名+精灵列表
matchmaking_join — 加入匹配(对手类型: bot/human)
```

### 3.5 埋点管道核心逻辑

```
前端 → 两种通道发送事件:

通道1: HTTP POST /api/v1/analytics
  track.js (新建, 页面浏览+点击)
  └─ router.afterEach → trackPageView(path)
  └─ navbar @click → trackClick("nav_matchmaking"等)

通道2: WebSocket analytics_batch
  analytics.js (对战事件)
  └─ MatchmakingPage 调 trackBattleInit / trackDamage 等
  └─ send('analytics_batch', {events: [...]})

服务端:
  standalone_server.py 接收后分流:
    ├─ battle_* 事件 → send_battle_log() → battle.logs topic
    └─ 其他       → send_ui_event()  → player.ui.events topic
```

### 3.6 VM 桥接 (kafka_to_sqlite.py)

消费 Kafka → 聚合写入 SQLite 的**核心SQL逻辑**:

```
消费 battle_init:
  INSERT OR IGNORE INTO meta_species (name, appearance_count)
    VALUES (:speciesID, 1)
    ON CONFLICT(name) DO UPDATE SET appearance_count = appearance_count + 1

  meta_moves / meta_items / meta_abilities 同理

消费 page_view:
  INSERT INTO page_dwell (page, visit_count) VALUES (:page, 1)
    ON CONFLICT(page) DO UPDATE SET visit_count = visit_count + 1

  INSERT INTO recent_events (ts, player_id, event, detail)
    VALUES (ts, pid, event, "浏览了 " + pageLabel)  -- 预格式化中文

消费 ui_click:
  INSERT INTO click_stats (element, count) VALUES (:el, 1)
    ON CONFLICT(element) DO UPDATE SET count = count + 1

每10秒聚合:
  SELECT COUNT(*) FROM event_counts → summary_stats
  UPDATE meta_species SET usage_pct = appearance_count / total  -- 重新计算百分比
  DELETE FROM recent_events WHERE id NOT IN (SELECT id ... LIMIT 500)  -- 只保留最近500条
```

---

## 四、数据库表结构

### 4.1 pokemon.db (游戏数据, 本地)

| 表名 | 关键字段 | 说明 |
|------|---------|------|
| `species` | id, name, type1/2, base_hp/atk/def/spa/spd/spe | 1050只宝可梦种族值 |
| `moves` | id, name, type, category, power, accuracy, pp, chinese_name | 902个招式 |
| `abilities` | id, name, chinese_name | 308个特性 |
| `items` | id, name, chinese_name | 524个道具 |
| `learnsets` | species_id, move_id | 精灵-技能学习关系 |
| `users` | username, created_at | 玩家注册 |
| `user_teams` | id, user_id, team_name, pokemon_json | 保存的队伍 |
| `name_mapping` | english, chinese | 英中名称映射 |
| `species_aliases` | species_id, alias_name, sprite_name | 形态别名 |
| `item_form_map` | base_item, species_id, form_item | 道具形态映射 |

### 4.2 pokemon_stats.db (大数据聚合, VM)

| 表名 | 类型 | 关键字段 | 说明 |
|------|------|---------|------|
| `meta_species` | 聚合 | name(speciesID), appearance_count, usage_pct, ko_rate, faint_count | 精灵使用率排行 |
| `meta_moves` | 聚合 | name(moveID), usage_pct | 招式使用次数 |
| `meta_items` | 聚合 | name(itemID), usage_pct | 道具携带次数 |
| `meta_abilities` | 聚合 | name(abilityID), usage_pct | 特性使用次数 |
| `battle_events` | 明细 | id, battle_id, turn, event_type, damage, fainted, timestamp, winner | 对战事件流 |
| `ui_events` | 明细 | id, player_id, event, page, element, timestamp | UI事件流 |
| `battle_turns` | 明细 | battle_id, turn, event, data | 回合数据 |
| `event_counts` | 聚合 | event_type, count | 各事件类型总数 |
| `click_stats` | 聚合 | element, count | 按钮点击量 |
| `player_stats` | 聚合 | player_id, events, last_seen | 玩家活跃度 |
| `page_dwell` | 聚合 | page, visit_count, total_dwell_seconds | 页面访问 |
| `recent_events` | 明细 | id, ts, player_id, event, detail | 最近500条(预格式化中文) |
| `summary_stats` | 聚合 | key, value (battles/total_events/players) | K/V全局统计 |

### 4.3 output.db (本地战斗分析, 开发机)

| 表名 | 关键字段 | 说明 |
|------|---------|------|
| `battle_pokemon_states` | battle_id, turn, species_id, hp, max_hp, hp_pct, fainted, ability_id, item_id, move_ids | 每只精灵每回合状态快照 |

### 4.4 gen91v1_stats.sqlite (Smogon元数据, 335MB)

| 表名 | 关键字段 | 行数 | 说明 |
|------|---------|------|------|
| `mon` | name, source(smogon/simulator), time_bucket(月度/日), rating(0/1500/1630/1760), usage, viability_ceiling | 15,290 | 精灵排名 |
| `move` | mon, name, usage | 443,867 | 每只精灵的技能使用率 |
| `ability` | mon, name, usage | 30,874 | 每只精灵的特性使用率 |
| `item` | mon, name, usage | 195,466 | 每只精灵的道具使用率 |
| `spread` | mon, nature, evs, usage | 602,844 | 努力值分配 |
| `team` | mon, mate, usage | 954,356 | 队友搭配 |
| `cc` | mon, opp, percentage, stddev | 12,676 | 克制关系(Checks & Counters) |
| `tera` | mon, type, usage | 15,194 | 太晶化类型 |

---

## 五、Smogon 数据集成

### 导出流
```
VM 上 Python 脚本 → 爬取 Smogon 月度数据 → gen91v1_stats.sqlite
                                            │
                          export_smogon_meta.py (加权Top100)
                                            │
                          data/smogon_meta.json (203KB)
                                            │
                          data_daemon.js 随机队伍生成
                          TeamBuilder.vue 一键构建
                          StatsDashboard.vue 分析面板
```

### 查询接口
```
/api/v1/smogon/filters      → 可用的 source/time_bucket/rating
/api/v1/smogon/pokemon       → 精灵排名 (支持搜索、分页)
/api/v1/smogon/pokemon/{name} → 单只精灵详情 (技能/道具/特性/队友/克制)
/api/v1/smogon/summary        → KPI 汇总
/api/v1/smogon/trend/{name}   → 月度趋势
/api/v1/smogon/types          → 太晶化分布
/api/v1/smogon/moves          → 全局招式排行
/api/v1/smogon/items          → 全局道具排行
```

---

## 六、前端页面功能

| 路由 | 页面 | 核心功能 |
|------|------|---------|
| `/` | HomePage | 首页入口 |
| `/matchmaking` | MatchmakingPage | VS Bot / PvP 匹配, 实时对战, WebSocket 状态同步 |
| `/teams` | TeamBuilder | 手动组队 + Smogon 一键构建, 精灵/技能搜索 |
| `/stats` | StatsDashboard | Smogon 大数据分析, 精灵排行/趋势/道具/特性 |
| `/realtime` | RealTimeStats | 实时对战数据看板 (从 VM 拉取) |
| `/analytics` | AnalyticsPage | 用户行为分析 (页面浏览/点击/玩家活跃) |
| `/data` | DataExplorer | 游戏数据浏览 (精灵/技能/特性/道具) |

---

## 七、运维

**部署脚本**: `scripts/Linux/deploy.sh`
- 单环境变量驱动: `MASTER_IP=10.0.1.1 bash deploy.sh`
- 全自动: 系统依赖 → Python/Node → Kafka → Bridge → Stats API → 前端 → Nginx → systemd

**VM 监控**:
```bash
curl http://VM_IP:8080/api/v1/stats/deep/summary  # 数据总量
ssh hadoop@VM_IP "pgrep -f kafka_to_sqlite"        # 桥状态
```

**Kafka 清数据**: `scripts/py/kafka_cleanup.py` (按 UTC 时间戳截断)
