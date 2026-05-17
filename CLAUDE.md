# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

C++17 backend for Pokemon Gen9 singles battle simulator (no Terastallization). Communicates with a frontend via JSON files in `cache/`.

See `AGENTS.md` for project guidelines, decision log, and workflow conventions.

## Build & Test Commands

```bash
# Build (Ninja)
cmake --build build

# Reconfigure + build
cmake -B build -S . -DBUILD_TESTING=ON
cmake --build build

# Run all GTest tests
ctest --test-dir build --output-on-failure

# Run the single GTest binary directly (supports --gtest_filter)
./build/PokemonSimulatorTests
./build/PokemonSimulatorTests --gtest_filter="TestName*"

# CLI-based move/item test runners (not GTest)
./build/PokemonSimulator --run-move-tests
./build/PokemonSimulator --run-item-tests

# Single-turn execution via JSON
./build/PokemonSimulator --run-turn-json <request.json>
./build/PokemonSimulator --run-turn-json-files <side_a.json> <side_b.json> <turn.json> [seed]

# Cache-based I/O modes
./build/PokemonSimulator --run-cache-input
./build/PokemonSimulator --daemon

# Data prefetch from PokeAPI
./build/PokemonSimulator --prefetch-moves [--refresh]
./build/PokemonSimulator --prefetch-abilities [--refresh]
./build/PokemonSimulator --prefetch-items [--refresh]
```

## Architecture

### Layered design

```
main.cpp  →  BattleSession (IO layer)  →  Battle (core engine)  →  GameRegistry (data)
```

**`BattleSession`** (`src/IO/BattleSession.cpp`) — External API. Creates battles from JSON, processes turns, returns JSON state. The entry point for all CLI modes.

**`Battle`** (`src/battle/Battle.cpp`) — Core turn loop (`processTurn`, `resolveNextAction`), damage calc, status effects, weather, field effects. Owns `Side`, `Field`, `Weather`, `BattleQueue`, `EventSystem`, `RuntimeMoveState`. Acts as the central hub for ability/item triggers.

**`BattleContext`** (`include/battle/BattleContext.h`) — Lightweight aggregator of subsystem references (`Weather`, `Field`, `Side`, `RuntimeMoveState`, `EventSystem`). Has a back-pointer to `Battle` via `setBattlePointer()`, so move rules and effect handlers can trigger battle operations without directly depending on `Battle`.

**`GameRegistry`** (`src/battle/GameRegistry.cpp`) — Singleton holding all registered abilities, items, and move rules. Initialized via `initializeCoreAbilities` / `initializeCoreItems` / `initializeCoreMoveRules`.

### Extension patterns

- **Moves** — Registered as `MoveRuleHandler` functions via `GameRegistry::registerMoveRule("name", handler)`. Each handler receives `(BattleContext&, Pokemon* user, Pokemon* target, const Move&)`. Status moves: 277/277 registered.
- **Abilities** — Built via `AbilityBuilder` (lambda configures type immunities, status immunities, triggers). ~246 engine entries covering all competitive abilities; the remaining ~121 are Tera (6) and Conquest (60) types not relevant to Gen9 singles.
- **Items** — Built via `ItemBuilder` (stat modifiers, damage modifiers, triggers). **Frozen** as of 2026-05-17: 145 competitively relevant items are registered; the remaining 152 in data are evolution/XP/PP/contest/EV/breeding items with no battle effect.

### Key subsystems

| Class | Purpose |
|-------|---------|
| `Side` | One side of the field (6 Pokemon team, active index, field effects) |
| `Pokemon` | Pokemon state (HP, status, stat stages, held item, moves) |
| `Field` | Field-wide effects (Trick Room, Magic Room, Wonder Room, Gravity, terrains) |
| `Weather` | Weather state (Sun, Rain, Sand, Snow, etc.) |
| `BattleQueue` | Speed-ordered action queue (accounts for priority, Trick Room, speed ties) |
| `EventSystem` | Sequential battle event log, serialized to output JSON as `state.battle_all_info` |
| `RuntimeMoveState` | Per-turn move execution state (hit count, damage dealt, flags) |
| `MoveEffectHandler` | Standard move secondary effects (status conditions, stat changes) |

### Data flow

1. Input: JSON files (`data/`, `cache/input/`) describing teams and turn actions
2. Engine processes the turn through `BattleSession::processTurn()` → `Battle::processTurn()`
3. Output: JSON written to `cache/output/` with full battle state and event log

### Key source locations

- Move rules: `src/battle/GameRegistry.cpp` (search `registerMoveRule`)
- Ability registration: `src/battle/GameRegistry.cpp` (search `initializeCoreAbilities`)
- Item registration: `src/battle/GameRegistry.cpp` (search `initializeCoreItems`)
- Turn resolution: `src/battle/Battle.cpp` (`processTurn`, `resolveNextAction`)
- Data definitions: `src/IO/MoveData.cpp`, `src/IO/AbilityData.cpp`, `src/IO/ItemData.cpp`
- Enums: `include/battle/` (search for `enum class`)
- GTest suite: `tests/battle_data_test.cpp` (192 tests)

## Important Constraints

- The item implementation is **frozen** — do not add new items. See AGENTS.md decision log.
- Do not guess move/ability/item mechanics. Consult Pokemon Showdown source code for exact behavior.
- Keep battle main loop clean: all move/ability/item logic belongs in registered handlers, not inline in `Battle::processTurn`.
- Never leave a TODO in a registered handler — implement the full logic.
- Deterministic RNG via optional seed parameter for reproducible battles.
