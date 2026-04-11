# Gen9 Moves Implementation Plan (Singles 1v1 First)

## Scope Freeze
- Baseline: Showdown Gen9 full move scope (including non-standard/historical).
- Execution mode: singles 1v1 first; doubles-specific target semantics are tracked as deferred when needed.

## Current Position
- Phase 0 complete; Phase 2 (mechanic batches) started.
- Completed:
  - Scope and delivery strategy locked.
  - Matrix generator script added: scripts/generate_move_matrix.py.
  - Initial move progress matrix generated from data/moves.json.
  - First mechanic batch landed: Recover/Soft-Boiled/Milk Drink healing + Haze stat reset.
  - Regression coverage added for the new mechanics.
- Latest matrix snapshot:
  - implemented: 260
  - partial: 496
  - todo: 181

## Milestones
1. Phase 0: build and maintain move matrix (implemented/partial/todo/deferred).
2. Phase 1: expand move data model and effect routing to reduce name-hardcoding.
3. Phase 2: implement moves in batches (40-80 per batch) by mechanic group.
4. Phase 3: strengthen regression suites and showdown-aligned replay checks.
5. Phase 4: converge matrix to zero todo under singles scope.

## Progress Tracking Rules
- Every implementation batch must update docs/moves-progress.csv and docs/moves-progress.md.
- Every batch requires build + ctest pass before merge.
- Keep deferred list explicit for doubles-only semantics to avoid hidden scope drift.

## Commands
```bash
python3 scripts/generate_move_matrix.py
cmake --build /home/qzz/PokemonSimulator/build --target PokemonSimulatorTests -j
ctest --test-dir /home/qzz/PokemonSimulator/build --output-on-failure
```
