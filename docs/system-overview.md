# PokemonSimulator 系统全景文档

> 最后更新：2026-07-09

## 一、物理拓扑

```
┌──────────────────────────────────────────────────────────────────┐
│  Windows 开发机 (本地)                                            │
│                                                                   │
│  data_daemon.js (:--  )      standalone_server.py (:9000)        │
│  │ 8 battle + 3 UI workers    │ FastAPI + WebSocket              │
│  │ Showdown 引擎               │ REST API + Smogon 查询           │
│  │                             │ analytics_service → Kafka        │
│  │                             │ kafka_producer.py                │
│  └──────────┬──────────────────┘                                  │
│             │                                                     │
│  Vite :5173  (前端开发服务器)                                     │
│  │ Vue 3 SPA                                                    │
│  │ proxy: /api → :9000, /api/v1/stats → VM:8080                 │
│  └──────────────────────────────────────────────────────────────┘ │
└──────────────────────────────┬────────────────────────────────────┘
                               │
                    Kafka Protocol (:9092)
                               │
┌──────────────────────────────▼────────────────────────────────────┐
│  VM: myz (100.107.105.99) — Ubuntu Server                        │
│                                                                   │
│  Apache Kafka (KRaft, :9092)                                      │
│  ├─ battle.logs           (3 partitions)                         │
│  └─ player.ui.events      (3 partitions)                         │
│                                                                   │
│  kafka_to_sqlite.py  (ADS Bridge, consumer group: ads-bridge-fresh)│
│  │ Kafka Consumer → SQLite 聚合写入                               │
│  │                                                                │
│  stats_server.py  (:8080)                                        │
│  │ HTTP API → pokemon_stats.db 查询                              │
│  │ /api/v1/stats/snapshot, /deep/*, /ui/*                        │
│                                                                   │
│  /opt/bigdata/pokemon_stats.db  (SQLite)                          │
│  ├─ meta_species, meta_moves, meta_items, meta_abilities         │
│  ├─ battle_events, ui_events, battle_turns                       │
│  ├─ recent_events, event_counts, summary_stats                   │
│  ├─ click_stats, player_stats, page_dwell                        │
│  └─ player_favorites                                             │
└──────────────────────────────────────────────────────────────────┘
```

---

## 二、数据链路（核心）

### 2.1 模拟数据流（data_daemon）

```
data_daemon.js                        Kafka                      VM Bridge
══════════════                        ═════                      ════════
8 个 Battle Worker ──→ battle.logs ──────────→ kafka_to_sqlite.py
│                        (JSON)                   │
│  battle_init                                     ├─ meta_species (+appearance)
│  turn_executed                                   ├─ meta_moves (+usage)
│  turn_damage                                     ├─ meta_items (+usage)
│  turn_faint                                      ├─ meta_abilities (+usage)
│  turn_switch                                     ├─ event_counts
│  battle_result                                   └─ summary_stats
│
3 个 UI Worker ──→ player.ui.events ────→ kafka_to_sqlite.py
                     (JSON)                   │
  page_view                                   ├─ page_dwell (+visit)
  ui_click                                    ├─ click_stats (+count)
  player_state                                ├─ player_stats (+events)
  matchmaking_join                            ├─ recent_events (+detail)
  session_start                               └─ event_counts
  team_save
```

### 2.2 真实用户埋点流（前端打点）

```
用户浏览器                  standalone_server.py                 Kafka
════════                   ═══════════════════                 ═════
router.afterEach ──→ POST /api/v1/analytics ──→ kafka_producer.py
  trackPageView()         (HTTP JSON batch)        │
                                                     ├─ page_view ──→ player.ui.events
navbar @click ──→ trackClick()                      ├─ ui_click  ──→ player.ui.events
  nav_matchmaking                                    ├─ session_start
  nav_teams                                          ├─ player_state
  ...                                                └─ team_save

MatchmakingPage ──→ WebSocket analytics_batch ──→ standalone_server.py
  trackBattleInit()                                     │
  trackTurnExecuted()                     battle 事件 → send_battle_log()
  trackBattleResult()                                   │
  trackDamage()                           UI 事件     → send_ui_event()
  trackFaint()                                          │
  trackSwitch()                                   ──→ Kafka
```

### 2.3 VS Bot 对战数据流（服务端打点）

```
用户 → WebSocket                standalone_server.py                  Kafka
═══════════                     ═══════════════════                  ═════
"join_match" (bot) ──→ start_bot_engine()
                           │
                           ├─ BattleEngine.start()
                           ├─ daemon.get_state()
                           ├─ _analytics.log_battle_init() ──→ battle.logs
                           └─ send("matched", ...)

"process_turn" ──→ daemon.process_turn()
                       │
                       ├─ _analytics.log_battle_turn()  ──→ battle.logs
                       └─ if ended:
                            _analytics.log_battle_result() ──→ battle.logs
```

### 2.4 打点事件分类与路由

| 事件 | 来源 | Kafka Topic | 路由方式 |
|------|------|-------------|----------|
| `battle_init` | daemon / VS Bot | `battle.logs` | `send_battle_log()` |
| `turn_executed` | daemon / VS Bot | `battle.logs` | `send_battle_log()` |
| `turn_damage` | VS Bot 前端 | `battle.logs` | `analytics_batch` 分流 |
| `turn_faint` | VS Bot 前端 | `battle.logs` | `analytics_batch` 分流 |
| `turn_switch` | VS Bot 前端 | `battle.logs` | `analytics_batch` 分流 |
| `battle_result` | daemon / VS Bot | `battle.logs` | `send_battle_log()` |
| `page_view` | 前端 router | `player.ui.events` | `send_ui_event()` |
| `ui_click` | 前端 navbar/btn | `player.ui.events` | `send_ui_event()` |
| `player_state` | 前端 | `player.ui.events` | `send_ui_event()` |
| `session_start` | 前端 | `player.ui.events` | `send_ui_event()` |
| `team_save` | 前端 | `player.ui.events` | `send_ui_event()` |
| `matchmaking_join` | 前端 | `player.ui.events` | `send_ui_event()` |

---

## 三、Kafka Topics

### 3.1 Topic 清单

| Topic | 分区 | Producer | Consumer | 说明 |
|-------|------|----------|----------|------|
| `battle.logs` | 3 | data_daemon.js, standalone_server | kafka_to_sqlite.py | 对战全生命周期 |
| `player.ui.events` | 3 | data_daemon.js, standalone_server | kafka_to_sqlite.py | 用户行为全量 |

### 3.2 消息时间戳

所有消息的时间戳统一使用 **UTC ISO 8601** 格式：

- `data_daemon.js`: `new Date().toISOString()` → 自动 UTC
- `track.js` / `analytics.js`: `new Date().toISOString()` → 自动 UTC
- `analytics_service.py`: `datetime.now(timezone.utc).strftime(...)` → 显式 UTC
- `kafka_producer.py`: 透传上游时间戳
- `kafka_to_sqlite.py` (bridge): 读取消息中的 `timestamp` 字段

---

## 四、VM 桥接层

### 4.1 kafka_to_sqlite.py (ADS Bridge)

**位置**: `~/kafka_to_sqlite.py` on myz (hadoop 用户)

**Consumer Group**: `ads-bridge-fresh`

**启动**:
```bash
ssh hadoop@myz "cd ~ && nohup python3 kafka_to_sqlite.py > logs/bridge.log 2>&1 &"
```

**功能**:
1. 消费 `battle.logs` → 提取 species/moves/items/abilities 使用量
2. 消费 `player.ui.events` → 记录页面访问、点击、玩家状态
3. 每 10 秒聚合更新百分比统计
4. 保留最近 500 条 `recent_events`

### 4.2 stats_server.py

**位置**: `~/stats_server.py` on myz

**端口**: `0.0.0.0:8080`

**主要端点**:

| 路径 | 用途 |
|------|------|
| `GET /api/v1/stats/snapshot` | 全量数据快照 |
| `GET /api/v1/stats/deep/summary` | 汇总统计 |
| `GET /api/v1/stats/deep/all` | 全部排行 |
| `GET /api/v1/stats/deep/meta` | 精灵排行 |
| `GET /api/v1/stats/deep/moves` | 招式排行 |
| `GET /api/v1/stats/deep/live` | 实时快照 |
| `GET /api/v1/stats/ui/recent` | 最近事件 |
| `GET /api/v1/stats/ui/players` | 玩家统计 |
| `GET /api/v1/stats/ui/clicks` | 点击排行 |
| `GET /api/v1/stats/ui/page_dwell` | 页面停留 |
| `GET /api/v1/stats/ui/favorites` | 热门功能 |

### 4.3 pokemon_stats.db 表结构

| 表名 | 类型 | 说明 |
|------|------|------|
| `meta_species` | 聚合 | 精灵出场次数/使用率 |
| `meta_moves` | 聚合 | 招式使用次数 |
| `meta_items` | 聚合 | 道具携带次数 |
| `meta_abilities` | 聚合 | 特性使用次数 |
| `battle_events` | 明细 | 对战事件（有 timestamp） |
| `ui_events` | 明细 | UI 事件（有 timestamp） |
| `battle_turns` | 明细 | 回合数据 |
| `recent_events` | 明细 | 最近 500 条事件（预格式化 detail） |
| `event_counts` | 聚合 | 事件类型计数 |
| `summary_stats` | 聚合 | K/V 全局统计 |
| `click_stats` | 聚合 | 按钮点击计数 |
| `player_stats` | 聚合 | 玩家活跃度 |
| `page_dwell` | 聚合 | 页面访问量 |
| `player_favorites` | 暂未使用 | — |

---

## 五、本地服务

### 5.1 standalone_server.py (:9000)

**启动**: `python api-server/standalone_server.py` (FastAPI + uvicorn)

**职责**:
- WebSocket 对战引擎桥接（Showdown daemon）
- REST API（队伍/对战/玩家查询）
- Smogon 数据查询（`/api/v1/smogon/*`）
- 埋点接收（`POST /api/v1/analytics`）→ Kafka 转发
- Stats 代理（`/api/v1/stats/*` → VM stats_server 透传）

**关键模块**:
| 模块 | 职责 |
|------|------|
| `services/analytics_service.py` | 事件收集、本地 JSONL 落盘 + Kafka 发送 |
| `services/kafka_producer.py` | Kafka 异步生产者（线程池） |
| `smogon_stats/queries.py` | Smogon SQLite 查询层 |

### 5.2 data_daemon.js

**启动**: `node engine-adapter/data_daemon.js`

**配置**:
- 8 个 Battle Worker（Showdown 模拟对战）
- 3 个 UI Worker（模拟用户行为）
- Smogon 加权随机队伍生成（来自 `data/smogon_meta.json`）

### 5.3 Vite 开发服务器 (:5173)

**代理规则**:
```js
proxy: {
  '/api/v1/stats': { target: 'http://100.107.105.99:8080' },  // VM stats
  '/api':          { target: 'http://localhost:9000' },         // 本地 API
  '/ws':           { target: 'http://localhost:9000', ws: true },
}
```

---

## 六、前端架构

### 6.1 路由

| 路径 | 组件 | 说明 |
|------|------|------|
| `/` | HomePage | 首页 |
| `/login` | LoginPage | 登录 |
| `/matchmaking` | MatchmakingPage | 匹配对战（VS Bot / PvP） |
| `/teams` | TeamBuilder | 组队器 + Smogon 一键构建 |
| `/realtime` | RealTimeStats | 实时对战数据看板 |
| `/analytics` | AnalyticsPage | 用户行为分析 |
| `/stats` | StatsDashboard | Smogon 大数据分析面板 |
| `/data` | DataExplorer | 数据浏览器 |

### 6.2 埋点工具

| 文件 | 用途 | 发送方式 |
|------|------|----------|
| `utils/track.js` | 页面浏览、导航点击 (新建) | HTTP POST → `/api/v1/analytics` |
| `utils/analytics.js` | 对战全生命周期 + UI 事件 (旧) | WebSocket → `analytics_batch` |

**排查脏数据**: `/test`, `/login`, `/realtime` 已被 `track.js` 过滤，不会发送。

### 6.3 关键状态管理

- 对战状态通过 WebSocket 双向同步
- 用户信息存储在 `localStorage`（`trainer_name`）
- Smogon 数据缓存于 `moveMetaCache`（避免重复请求）

---

## 七、Smogon 数据集成

### 7.1 数据库

**源文件**: `smogon_stats/gen91v1_stats.sqlite` (335MB, 15 张表)

**数据源**:
- `source=smogon`: 2025-07 至 2026-05 月粒度数据
- `source=simulator`: 2026-07-07 日粒度数据（95 只精灵，来自本系统 daemon）

**同步脚本**: `scripts/sync_smogon.sh`
```bash
bash scripts/sync_smogon.sh   # scp from VM → ATTACH MERGE → local
```

### 7.2 导出脚本

`scripts/py/export_smogon_meta.py` → `data/smogon_meta.json` (100 species × top 10 moves/abilities/items/spreads)

用于 data_daemon 的 Smogon 加权随机队伍生成。

---

## 八、运维指令速查

### 服务重启

```bash
# 本地服务
taskkill //f //im python.exe   # 停 API
taskkill //f //im node.exe     # 停 daemon + vite
cd E:/PokemonSImulator
nohup python api-server/standalone_server.py > logs/api-server.log 2>&1 &
cmd.exe /c "scripts\Windows\start_data_daemon.bat"
cd frontend && npx vite
```

### VM 监控

```bash
# Kafka 消费
ssh hadoop@myz "/home/hadoop/kafka/bin/kafka-console-consumer.sh --bootstrap-server localhost:9092 --topic player.ui.events --max-messages 5"

# VM DB 数据量
curl -s http://100.107.105.99:8080/api/v1/stats/deep/summary

# 桥状态
ssh hadoop@myz "ps aux | grep kafka_to_sqlite"
```

### 数据清理

```bash
# Kafka 按日期截断（保留 7月7日及之后）
python3 scripts/py/kafka_cleanup.py

# 仅清理 VM DB
ssh hadoop@myz "pkill -f kafka_to_sqlite && sqlite3 /opt/bigdata/pokemon_stats.db '...' && cd ~ && nohup python3 kafka_to_sqlite.py > logs/bridge.log 2>&1 &"
```

---

## 九、文件索引

| 路径 | 说明 |
|------|------|
| `engine-adapter/data_daemon.js` | 模拟数据生成器 |
| `engine-adapter/showdown_daemon.js` | Showdown 引擎桥接 |
| `api-server/standalone_server.py` | 主 API 服务器 |
| `api-server/services/analytics_service.py` | 埋点收集 + Kafka 发送 |
| `api-server/services/kafka_producer.py` | Kafka 异步生产者 |
| `api-server/config.py` | 全局配置 |
| `scripts/kafka_to_sqlite.py` | VM ADS 桥（本地副本） |
| `scripts/stats_server.py` | VM stats API（本地副本） |
| `scripts/sync_smogon.sh` | Smogon DB 同步脚本 |
| `scripts/py/export_smogon_meta.py` | Smogon 元数据导出 |
| `scripts/py/kafka_cleanup.py` | Kafka 按时间清理 |
| `frontend/src/utils/track.js` | 前端打点工具（页面/点击） |
| `frontend/src/utils/analytics.js` | 前端打点工具（战斗/状态） |
| `frontend/src/api/stats.js` | 统计 API 封装 |
| `frontend/src/api/smogon.js` | Smogon API 封装 |
| `smogon_stats/queries.py` | Smogon 查询层 |
| `smogon_stats/gen91v1_stats.sqlite` | Smogon 源数据库 |
| `data/smogon_meta.json` | Smogon 元数据 JSON |
| `docs/kafka-topics.md` | Kafka 消息格式规范 |
| `docs/system-overview.md` | 本文档 |
