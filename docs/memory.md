# PokemonSimulator 项目记忆（当前快照）

## 项目目标（来自 ToDo / docs）
- 目标是实现 **GEN9 单打（无钛晶化）** 的后端对战引擎，优先保证单打正确性与可复现性。
- 当前基线（ToDo.json）：
  - moves: 937（implemented 314 / partial 498 / todo 125）
  - abilities: 数据 367，当前引擎枚举 57
  - items: JSON battle items 272，其中未支持 152
- 当前总计划按 P0~P5 分阶段推进：先稳定回合系统和状态生命周期，再扩展状态招式、特性、道具与边界机制。

## 代码结构与职责
- `BattleSession`：对外 JSON 回合接口（创建战斗、解析 action、enqueue、返回 state）。
- `Battle`：核心回合计算器（行动排序、伤害结算、状态/天气/场地、切换、触发特性/道具）。
- `BuildFromJson`：把前端/文件 JSON 转成 `Species/Pokemon/Move/Item/Ability`。
- `BattleToJson`：把战斗状态序列化给前端。
- `EventSystem` + `cache/event.json`：保存战斗过程中按顺序触发的运行时事件（含 ability/item/status/heal/switch 等）。

## 当前进度判断（结合 docs + 头文件 + ToDo）
- 对战主循环、队列优先级、状态/天气/场地、部分高频机制已经可运行，且有较大规模 gtest 覆盖。
- `ToDo.json` 显示目前主线仍处于 **P0/P1/P2/P3/P4 并行推进**，并记录了近几次对保护系、Heal Block、Taunt/Torment/Disable 过期行为、Nightmare 等修复。
- `docs/moves-progress.md` 显示 move 覆盖已达可用但仍有明显缺口（尤其 todo 与 partial）。
- 当前测试基线仍存在既有失败（非本次改动引入）：  
  `KinesisSweetScentAndPainSplitApplyExpectedEffects`、`SafeguardPsychUpHealBellAndAromatherapyApplyExpectedEffects`、`HazeResetsBothActivePokemonStatStages`。

## JSON I/O 现状与本次已对齐项
- 旧接口已支持 `actions[]` 风格，但前端常见“分 side 两份 action JSON”风格兼容不足。
- 输出里已有 `state`，但“索引化 HP/PP + 时间线 + 描述”不完整。
- 本次已补齐：
  1. `BattleSession::processTurn` 兼容多种输入：
     - `actions[]`
     - `side_a/side_b`、`a/b`、`player_a/player_b`
     - `side_a_action/side_b_action`、`action_a/action_b`
     - `type: "move"` 作为 `attack` 别名
  2. 新增索引化输出 `battle_all_info`（并写入 `state.battle_all_info`）：
     - 双方队伍按 `side_index/pokemon_index`
     - HP、PP、move id、ability id、item id、status id 等整数索引化字段
     - 从 `cache/event.json` 提取并输出按顺序时间线（含事件索引与描述）
  3. 新增第 0 回合上场事件记录：
     - 在 `Battle` 构造时写入双方 `switch_in`（`reason=initial_send_out`）
     - 随后继续触发双方上场特性/道具逻辑

## 关键文件（本次理解与改动聚焦）
- `src/Battle/BattleSession.cpp`
- `src/Battle/BattleToJson.cpp`
- `src/Battle/Battle.cpp`
- `include/Battle/BattleToJson.h`
- 参考文档：
  - `docs/prompt.md`
  - `docs/moves-progress.md`
  - `ToDo.json`
