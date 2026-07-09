# Kafka Topic 消息格式文档

## Topic 概览

| Topic | 生产者 | 消费者 | 描述 |
|---|---|---|---|
| `battle.logs` | data_daemon.js | kafka_to_sqlite.py | 对战事件流 |
| `player.ui.events` | data_daemon.js | kafka_to_sqlite.py | 用户行为事件流 |

---

## 一、battle.logs — 对战事件

### 通用顶层字段

所有 battle.logs 消息都包含这些顶层字段：

| 字段 | 类型 | 说明 |
|---|---|---|
| `event` | string | 事件类型 |
| `battle_id` | string | 对战唯一 ID，格式 `b_{workerId}_{timestamp}` |
| `session_id` | string | 会话 ID，格式 `sess_{玩家A}_{玩家B}_{timestamp}` |
| `timestamp` | string | ISO 8601 时间戳 |
| `data` | object | 事件负载（各类型不同，见下） |

部分事件还有 `turn`、`player_a`、`player_b`、`winner` 等字段。

---

### 1. battle_init

每场战斗开始时发送 1 次。

```json
{
  "event": "battle_init",
  "battle_id": "b_1_mra2abc1",
  "session_id": "sess_Ash_Serena_mra2abc1",
  "player_a": "Ash",
  "player_b": "Serena",
  "timestamp": "2026-07-07T03:10:00.000Z",
  "data": {
    "battle_id": "b_1_mra2abc1",
    "session_id": "sess_Ash_Serena_mra2abc1",
    "player_a": "Ash",
    "player_b": "Serena",
    "opponent_type": "bot",
    "side_a": [
      {
        "speciesID": 6,
        "moves": [53, 126, 44, 82],
        "item": 2,
        "ability": 66,
        "nature": 3,
        "level": 50
      }
    ],
    "side_b": [
      {
        "speciesID": 25,
        "moves": [85, 87, 72, 45],
        "item": 10,
        "ability": 9,
        "nature": 5,
        "level": 50
      }
    ]
  }
}
```

---

### 2. turn_executed

每个回合发送 1 次。HP 字段以玩家 ID 为 key。

```json
{
  "event": "turn_executed",
  "battle_id": "b_1_mra2abc1",
  "session_id": "sess_Ash_Serena_mra2abc1",
  "turn": 3,
  "player_a": "Ash",
  "player_b": "Serena",
  "timestamp": "2026-07-07T03:10:05.000Z",
  "data": {
    "battle_id": "b_1_mra2abc1",
    "session_id": "sess_Ash_Serena_mra2abc1",
    "turn": 3,
    "Ash": {
      "hp": 154,
      "maxhp": 197,
      "fainted": false
    },
    "Serena": {
      "hp": 124,
      "maxhp": 211,
      "fainted": false
    }
  }
}
```

---

### 3. turn_damage

每次造成伤害时发送。伤害值由 Showdown 引擎真实计算。

```json
{
  "event": "turn_damage",
  "battle_id": "b_1_mra2abc1",
  "session_id": "sess_Ash_Serena_mra2abc1",
  "turn": 3,
  "timestamp": "2026-07-07T03:10:05.100Z",
  "data": {
    "battle_id": "b_1_mra2abc1",
    "session_id": "sess_Ash_Serena_mra2abc1",
    "turn": 3,
    "target_player": "Serena",
    "damage": 87,
    "maxhp": 211,
    "curhp": 124,
    "fainted": false,
    "move_name": "Flamethrower"
  }
}
```

| 字段 | 说明 |
|---|---|
| `target_player` | 受伤玩家 ID |
| `damage` | 伤害量（maxhp - curhp） |
| `maxhp` | 最大 HP |
| `curhp` | 当前 HP |
| `fainted` | 是否濒死 |
| `move_name` | 造成伤害的招式名 |

---

### 4. turn_faint

宝可梦濒死时发送。

```json
{
  "event": "turn_faint",
  "battle_id": "b_1_mra2abc1",
  "session_id": "sess_Ash_Serena_mra2abc1",
  "turn": 8,
  "timestamp": "2026-07-07T03:10:15.000Z",
  "data": {
    "battle_id": "b_1_mra2abc1",
    "session_id": "sess_Ash_Serena_mra2abc1",
    "turn": 8,
    "player": "Ash"
  }
}
```

---

### 5. turn_switch

玩家换人时发送。

```json
{
  "event": "turn_switch",
  "battle_id": "b_1_mra2abc1",
  "session_id": "sess_Ash_Serena_mra2abc1",
  "turn": 9,
  "timestamp": "2026-07-07T03:10:18.000Z",
  "data": {
    "battle_id": "b_1_mra2abc1",
    "session_id": "sess_Ash_Serena_mra2abc1",
    "turn": 9,
    "player": "Ash",
    "reason": "forced"
  }
}
```

| reason | 说明 |
|---|---|
| `forced` | 濒死后强制换人 |
| `manual` | 主动换人 |

---

### 6. battle_result

战斗结束时发送。

```json
{
  "event": "battle_result",
  "battle_id": "b_1_mra2abc1",
  "session_id": "sess_Ash_Serena_mra2abc1",
  "player_a": "Ash",
  "player_b": "Serena",
  "winner": "Serena",
  "timestamp": "2026-07-07T03:10:30.000Z",
  "data": {
    "battle_id": "b_1_mra2abc1",
    "session_id": "sess_Ash_Serena_mra2abc1",
    "player_a": "Ash",
    "player_b": "Serena",
    "result": "completed",
    "winner": "Serena",
    "turns": 18,
    "side_a_remaining": 0,
    "side_b_remaining": 2
  }
}
```

| 字段 | 说明 |
|---|---|
| `result` | `completed` 或 `draw` |
| `winner` | 胜者玩家 ID，平局为 `null` |
| `turns` | 总回合数 |
| `side_a_remaining` | 玩家 A 剩余宝可梦数 |
| `side_b_remaining` | 玩家 B 剩余宝可梦数 |

---

## 二、player.ui.events — 用户行为事件

### 通用顶层字段

| 字段 | 类型 | 说明 |
|---|---|---|
| `event` | string | 事件类型 |
| `player_id` | string | 玩家 ID |
| `session_id` | string | UI 会话 ID，格式 `ui_sess_{玩家}_{workerId}_{timestamp}` |
| `timestamp` | string | ISO 8601 时间戳 |
| `data` | object | 事件负载 |

---

### page_view

```json
{
  "event": "page_view",
  "player_id": "Ash",
  "session_id": "ui_sess_Ash_1_mra2abc1",
  "timestamp": "2026-07-07T03:10:00.000Z",
  "data": { "page": "/stats" }
}
```

页面路径: `/` `/matchmaking` `/teams` `/stats` `/data`

---

### ui_click

```json
{
  "event": "ui_click",
  "player_id": "Ash",
  "session_id": "ui_sess_Ash_1_mra2abc1",
  "timestamp": "2026-07-07T03:10:02.000Z",
  "data": { "element": "btn_join_bot" }
}
```

元素列表:

| element | 含义 |
|---|---|
| `btn_confirm` | 确认 |
| `btn_switch` | 换人 |
| `btn_move_select` | 选招 |
| `btn_join_match` | PvP 匹配 |
| `btn_join_bot` | 对战 Bot |
| `btn_save_team` | 保存队伍 |
| `nav_matchmaking` | 导航-匹配 |
| `nav_teams` | 导航-组队 |
| `nav_stats` | 导航-统计 |
| `nav_data` | 导航-数据 |

---

### player_state

```json
{
  "event": "player_state",
  "player_id": "Ash",
  "session_id": "ui_sess_Ash_1_mra2abc1",
  "timestamp": "2026-07-07T03:10:10.000Z",
  "data": { "from": "idle", "to": "battling" }
}
```

状态: `idle` `matching` `battling` `teambuilding`

---

### matchmaking_join

```json
{
  "event": "matchmaking_join",
  "player_id": "Ash",
  "session_id": "ui_sess_Ash_1_mra2abc1",
  "timestamp": "2026-07-07T03:10:05.000Z",
  "data": { "opponent_type": "bot" }
}
```

---

### session_start

```json
{
  "event": "session_start",
  "player_id": "Ash",
  "session_id": "ui_sess_Ash_1_mra2abc1",
  "timestamp": "2026-07-07T03:10:00.000Z",
  "data": { "player_id": "Ash" }
}
```

---

### team_save

```json
{
  "event": "team_save",
  "player_id": "Ash",
  "session_id": "ui_sess_Ash_1_mra2abc1",
  "timestamp": "2026-07-07T03:10:20.000Z",
  "data": {
    "team_name": "晴天队",
    "pokemon_count": 5,
    "pokemon": [
      {
        "speciesID": 6,
        "moves": [],
        "item": 10,
        "ability": 66,
        "nature": 3
      }
    ]
  }
}
```

---

## 三、数据链路

```
data_daemon.js (Windows)
  │  8 battle workers + 3 UI workers
  │  Showdown Engine 真实模拟
  ├─ battle.logs ──→ Kafka (myz:9092) ──→ kafka_to_sqlite.py ──→ SQLite
  │                                              │                ├─ meta_species
  │                                              │                ├─ meta_moves
  │                                              │                ├─ meta_items
  │                                              │                ├─ meta_abilities
  │                                              │                ├─ event_counts
  │                                              │                └─ summary_stats
  │                                              │
  └─ player.ui.events ──→ Kafka (myz:9092) ──→ kafka_to_sqlite.py ──→ SQLite
                                                 │                ├─ page_dwell
                                                 │                ├─ click_stats
                                                 │                ├─ player_stats
                                                 │                └─ recent_events
                                                 │
                                            stats_server.py (:8080)
                                                 │
                                            standalone_server.py (:9000)
                                                 │
                                            前端 (:5173)
```
