---
description: "根据招式机制分类并给出可实现的代码改动方案（含 Effect Handler 解耦建议）"
name: "Implement Move By Mechanic"
argument-hint: "输入招式名、id、或一组招式（如: Encore, Dig, Round）"
agent: "agent"
---
请作为 Pokemon 对战引擎实现助手，完成“按机制分类实现招式”的单任务工作。

输入参数：
- `$ARGUMENTS`：一个或多个招式名/ID，或一句实现需求。

你必须遵循以下流程，并严格按“输出格式”返回：

1. 读取并校验数据源
- 读取 [moves.json](../../data/moves.json)。
- 从参数中解析目标招式；若参数为空，先给出可选候选并请求用户补充。
- 对每个目标招式提取：`id`、`name`、`apiName`、`type`、`category`、`power`、`accuracy`、`pp`、`effect`。

2. 机制分类（必须二选一或多选）
- 将每个招式映射到以下机制类别（可多标签）：
  - 一类：基于条件判定的伤害/命中计算
  - 二类：改变行动顺序与回合逻辑
  - 三类：交互与状态反射逻辑
  - 四类：天气、场地与空间效果
  - 五类：特定分组与机制协同
  - 六类：双打对战特殊机制
- 若无法确定，明确写出“证据不足”与最小可行实现假设。

3. 生成实现方案（面向当前仓库）
- 优先复用现有战斗管线；避免无关重构。
- 给出最小可行实现（MVP）与可选的“高保真扩展”。
- 必须把通用伤害计算与特殊副效果解耦：
  - 说明哪些逻辑应进入通用 `Damage Calculation`
  - 说明哪些逻辑应进入独立 `Effect Handler / Secondary Effect Logic`
- 明确需要修改的文件与职责（例如：`Moves.h`、`MoveData.cpp`、`Battle.cpp`、测试文件）。

4. 测试与验收标准
- 为每个目标招式至少给出 1 条行为测试建议（触发条件 + 预期结果）。
- 标明潜在回归风险（如回合顺序、天气场地共享状态、双打目标选择）。

输出格式（严格遵守）：

## 1) Move Snapshot
- 逐个招式列出关键字段（来自 moves.json）。

## 2) Mechanic Classification
- `招式 -> 类别标签`（可多标签）
- 每个标签后附 1 句判定依据。

## 3) Implementation Plan (MVP First)
- MVP：
  - 文件级改动清单
  - 关键伪代码/规则
- 扩展（可选）：
  - 更贴近对战规则的增强点

## 4) Decoupling Design
- Damage Calculation：
  - 仅列出应保留在通用伤害管线的逻辑
- Secondary Effect Logic / Effect Handler：
  - 仅列出应放入效果处理器的逻辑

## 5) Tests
- 每个招式的测试用例建议（输入场景、断言、边界条件）。

## 6) Risks & Assumptions
- 列出假设与未覆盖边界。

约束：
- 不编造 moves.json 中不存在的字段。
- 优先给“可落地到当前代码结构”的改动，而不是抽象理论。
- 如果参数包含多招式，输出中先按类别分组，再在组内按招式名排序。
