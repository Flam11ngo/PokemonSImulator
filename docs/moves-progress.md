# Gen9 Move Progress Matrix (Singles 1v1 First)

Generated at: 2026-04-10 14:33:12Z

Scope:
Showdown Gen9 full move scope (including non-standard/historical), implementation staged in singles 1v1 first.

## Summary

- Total moves in data/moves.json: 937
- Implemented: 260
- Partial: 496
- Todo: 181

## Heuristic Rules Used

- `implemented`: explicit move name rule exists, or effect handler exists.
- `partial`: baseline damage path only, or multi-target semantics not fully verified in singles-first mode.
- `todo`: no dedicated handler detected.

## Current Next Batch (Top 80 Todo By ID)

| id | name | apiName | category | effect | reason |
| --- | --- | --- | --- | --- | --- |
| 18 | Whirlwind | whirlwind | status | None | no explicit mechanic handler |
| 28 | Sand Attack | sand-attack | status | None | no explicit mechanic handler |
| 46 | Roar | roar | status | None | no explicit mechanic handler |
| 48 | Supersonic | supersonic | status | None | no explicit mechanic handler |
| 50 | Disable | disable | status | None | no explicit mechanic handler |
| 54 | Mist | mist | status | None | no explicit mechanic handler |
| 73 | Leech Seed | leech-seed | status | None | no explicit mechanic handler |
| 100 | Teleport | teleport | status | None | no explicit mechanic handler |
| 102 | Mimic | mimic | status | None | no explicit mechanic handler |
| 104 | Double Team | double-team | status | None | no explicit mechanic handler |
| 107 | Minimize | minimize | status | None | no explicit mechanic handler |
| 108 | Smokescreen | smokescreen | status | None | no explicit mechanic handler |
| 109 | Confuse Ray | confuse-ray | status | None | no explicit mechanic handler |
| 113 | Light Screen | light-screen | status | None | no explicit mechanic handler |
| 115 | Reflect | reflect | status | None | no explicit mechanic handler |
| 116 | Focus Energy | focus-energy | status | None | no explicit mechanic handler |
| 118 | Metronome | metronome | status | None | no explicit mechanic handler |
| 119 | Mirror Move | mirror-move | status | None | no explicit mechanic handler |
| 134 | Kinesis | kinesis | status | None | no explicit mechanic handler |
| 144 | Transform | transform | status | None | no explicit mechanic handler |
| 148 | Flash | flash | status | None | no explicit mechanic handler |
| 150 | Splash | splash | status | None | no explicit mechanic handler |
| 156 | Rest | rest | status | None | no explicit mechanic handler |
| 160 | Conversion | conversion | status | None | no explicit mechanic handler |
| 164 | Substitute | substitute | status | None | no explicit mechanic handler |
| 166 | Sketch | sketch | status | None | no explicit mechanic handler |
| 169 | Spider Web | spider-web | status | None | no explicit mechanic handler |
| 170 | Mind Reader | mind-reader | status | None | no explicit mechanic handler |
| 171 | Nightmare | nightmare | status | None | no explicit mechanic handler |
| 174 | Curse | curse | status | None | no explicit mechanic handler |
| 176 | Conversion 2 | conversion-2 | status | None | no explicit mechanic handler |
| 180 | Spite | spite | status | None | no explicit mechanic handler |
| 186 | Sweet Kiss | sweet-kiss | status | None | no explicit mechanic handler |
| 187 | Belly Drum | belly-drum | status | None | no explicit mechanic handler |
| 191 | Spikes | spikes | status | None | no explicit mechanic handler |
| 193 | Foresight | foresight | status | None | no explicit mechanic handler |
| 194 | Destiny Bond | destiny-bond | status | None | no explicit mechanic handler |
| 195 | Perish Song | perish-song | status | None | no explicit mechanic handler |
| 197 | Detect | detect | status | None | no explicit mechanic handler |
| 199 | Lock On | lock-on | status | None | no explicit mechanic handler |
| 201 | Sandstorm | sandstorm | status | None | no explicit mechanic handler |
| 203 | Endure | endure | status | None | no explicit mechanic handler |
| 212 | Mean Look | mean-look | status | None | no explicit mechanic handler |
| 213 | Attract | attract | status | None | no explicit mechanic handler |
| 214 | Sleep Talk | sleep-talk | status | None | no explicit mechanic handler |
| 215 | Heal Bell | heal-bell | status | None | no explicit mechanic handler |
| 219 | Safeguard | safeguard | status | None | no explicit mechanic handler |
| 220 | Pain Split | pain-split | status | None | no explicit mechanic handler |
| 230 | Sweet Scent | sweet-scent | status | None | no explicit mechanic handler |
| 234 | Morning Sun | morning-sun | status | None | no explicit mechanic handler |
| 235 | Synthesis | synthesis | status | None | no explicit mechanic handler |
| 236 | Moonlight | moonlight | status | None | no explicit mechanic handler |
| 240 | Rain Dance | rain-dance | status | None | no explicit mechanic handler |
| 241 | Sunny Day | sunny-day | status | None | no explicit mechanic handler |
| 244 | Psych Up | psych-up | status | None | no explicit mechanic handler |
| 256 | Swallow | swallow | status | None | no explicit mechanic handler |
| 258 | Hail | hail | status | None | no explicit mechanic handler |
| 259 | Torment | torment | status | None | no explicit mechanic handler |
| 266 | Follow Me | follow-me | status | None | no explicit mechanic handler |
| 267 | Nature Power | nature-power | status | None | no explicit mechanic handler |
| 269 | Taunt | taunt | status | None | no explicit mechanic handler |
| 270 | Helping Hand | helping-hand | status | None | no explicit mechanic handler |
| 271 | Trick | trick | status | None | no explicit mechanic handler |
| 272 | Role Play | role-play | status | None | no explicit mechanic handler |
| 273 | Wish | wish | status | None | no explicit mechanic handler |
| 274 | Assist | assist | status | None | no explicit mechanic handler |
| 275 | Ingrain | ingrain | status | None | no explicit mechanic handler |
| 277 | Magic Coat | magic-coat | status | None | no explicit mechanic handler |
| 278 | Recycle | recycle | status | None | no explicit mechanic handler |
| 281 | Yawn | yawn | status | None | no explicit mechanic handler |
| 285 | Skill Swap | skill-swap | status | None | no explicit mechanic handler |
| 286 | Imprison | imprison | status | None | no explicit mechanic handler |
| 287 | Refresh | refresh | status | None | no explicit mechanic handler |
| 288 | Grudge | grudge | status | None | no explicit mechanic handler |
| 289 | Snatch | snatch | status | None | no explicit mechanic handler |
| 293 | Camouflage | camouflage | status | None | no explicit mechanic handler |
| 298 | Teeter Dance | teeter-dance | status | None | no explicit mechanic handler |
| 300 | Mud Sport | mud-sport | status | None | no explicit mechanic handler |
| 303 | Slack Off | slack-off | status | None | no explicit mechanic handler |
| 312 | Aromatherapy | aromatherapy | status | None | no explicit mechanic handler |

## Files

- Full matrix CSV: docs/moves-progress.csv
- Summary markdown: docs/moves-progress.md

## Regenerate

```bash
python3 scripts/generate_move_matrix.py
```
