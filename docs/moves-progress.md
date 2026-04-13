# Gen9 Move Progress Matrix (Singles 1v1 First)

Generated at: 2026-04-11 15:10:25Z

Scope:
Showdown Gen9 full move scope (including non-standard/historical), implementation staged in singles 1v1 first.

## Summary

- Total moves in data/moves.json: 937
- Implemented: 314
- Partial: 498
- Todo: 125

## Heuristic Rules Used

- `implemented`: explicit move name rule exists, or effect handler exists.
- `partial`: baseline damage path only, or multi-target semantics not fully verified in singles-first mode.
- `todo`: no dedicated handler detected.

## Current Next Batch (Top 80 Todo By ID)

| id | name | apiName | category | effect | reason |
| --- | --- | --- | --- | --- | --- |
| 18 | Whirlwind | whirlwind | status | None | no explicit mechanic handler |
| 46 | Roar | roar | status | None | no explicit mechanic handler |
| 48 | Supersonic | supersonic | status | None | no explicit mechanic handler |
| 102 | Mimic | mimic | status | None | implemented via move rule |
| 109 | Confuse Ray | confuse-ray | status | None | no explicit mechanic handler |
| 144 | Transform | transform | status | None | implemented via move rule |
| 166 | Sketch | sketch | status | None | implemented via move rule |
| 191 | Spikes | spikes | status | None | no explicit mechanic handler |
| 193 | Foresight | foresight | status | None | implemented via move rule |
| 197 | Detect | detect | status | None | no explicit mechanic handler |
| 201 | Sandstorm | sandstorm | status | None | no explicit mechanic handler |
| 240 | Rain Dance | rain-dance | status | None | no explicit mechanic handler |
| 241 | Sunny Day | sunny-day | status | None | no explicit mechanic handler |
| 256 | Swallow | swallow | status | None | no explicit mechanic handler |
| 258 | Hail | hail | status | None | no explicit mechanic handler |
| 266 | Follow Me | follow-me | status | None | no explicit mechanic handler |
| 267 | Nature Power | nature-power | status | None | no explicit mechanic handler |
| 270 | Helping Hand | helping-hand | status | None | no explicit mechanic handler |
| 271 | Trick | trick | status | None | implemented via move rule |
| 272 | Role Play | role-play | status | None | implemented via move rule |
| 274 | Assist | assist | status | None | no explicit mechanic handler |
| 277 | Magic Coat | magic-coat | status | None | no explicit mechanic handler |
| 278 | Recycle | recycle | status | None | no explicit mechanic handler |
| 285 | Skill Swap | skill-swap | status | None | implemented via move rule |
| 289 | Snatch | snatch | status | None | no explicit mechanic handler |
| 300 | Mud Sport | mud-sport | status | None | no explicit mechanic handler |
| 316 | Odor Sleuth | odor-sleuth | status | None | implemented via move rule |
| 346 | Water Sport | water-sport | status | None | no explicit mechanic handler |
| 356 | Gravity | gravity | status | None | no explicit mechanic handler |
| 357 | Miracle Eye | miracle-eye | status | None | implemented via move rule |
| 361 | Healing Wish | healing-wish | status | None | no explicit mechanic handler |

## Files

- Full matrix CSV: docs/moves-progress.csv
- Summary markdown: docs/moves-progress.md

## Regenerate

```bash
python3 scripts/generate_move_matrix.py
```
