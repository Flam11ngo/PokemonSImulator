# Battle Arena UI Layout

## Container

```
battle-arena (.battle-arena)
  aspect-ratio: 4/3, max-height: 100vh
  rounded-2xl, border-2 border-gray-800, shadow-2xl
  overflow: hidden, position: relative, w-full
────────────────────────────────────────────────
Background:
  .battle-bg-dark — radial-gradient(
    ellipse at 50% 60%,
    #1a1a2e 0%, #0d0d1a 50%, #050510 100%
  )
  Two decorative ellipse blurs (bg-white/5 blur-sm)
```

## Zones (position:absolute, z-index layered)

```
┌──────────────────────────────────────────────────┐
│  Z-30: Turn Counter (top-2 right-2)              │
│  ┌─────────────┐                                 │
│  │  Turn 3     │  WeatherField (top-left area)   │
│  └─────────────┘                                 │
│                                                   │
│         ┌──────────────────────┐                  │
│  Z-10   │   ENEMY POKEMON      │ top:5% right:6% │
│         │   Name                │ w:26%  h:38%    │
│         │   ████████████ HP bar │                  │
│         │   stages | status     │                  │
│         │   hp/maxHp counter    │                  │
│         │        [SPRITE]       │                  │
│         │   (front-facing gif)  │                  │
│         └──────────────────────┘                  │
│                                                   │
│  Z-20  ┌─┐                                        │
│  Bench ⇅│  ┌─────────────────────────┐            │
│  balls ┌─┐ │   PLAYER POKEMON         │            │
│  (vert  ┌─┐│   bottom:26% left:5%     │            │
│   list) ┌─┐│   w:33.33% h:48%        │            │
│         ┌─┐│   Name                   │            │
│         │💀││   ████████████ HP bar    │            │
│         └─┘│   status | hp/maxHp      │            │
│            │   stat stages            │            │
│            │        [SPRITE]          │            │
│            │   (back-facing gif)      │            │
│            └─────────────────────────┘            │
│                                                   │
│ Z-25  EVENT OVERLAY (BattleEvents)                │
│       full area, align-items:flex-end             │
│       ┌─────────────────────────────────────────┐ │
│       │  "Pikachu used Thunderbolt! -120 HP 💀" │ │
│       │                              [点击继续]  │ │
│       └─────────────────────────────────────────┘ │
│       height:24%, bg:#12121a, border-top:#2a2a35 │
│                                                   │
│ Z-10  BOTTOM BAR  height:24% bg:#111118           │
│  ┌────────┬────────┬────────┬────────┐ ┌───────┐ │
│  │ Move 1 │ Move 2 │ Move 3 │ Move 4 │ │ 确认  │ │
│  │ type   │ type   │ type   │ type   │ │       │ │
│  │ name   │ name   │ name   │ name   │ │ 离开  │ │
│  │ PP N/M │ PP N/M │ PP N/M │ PP N/M │ │       │ │
│  └────────┴────────┴────────┴────────┘ └───────┘ │
│                                                   │
│  OR (switch mode):                                │
│  ┌────────┬────────┬────────┬────────┐ ┌───────┐ │
│  │Bench 1 │Bench 2 │Bench 3 │Bench 4 │ │ 确认  │ │
│  │sprite  │sprite  │💀      │sprite  │ │       │ │
│  │hp bar  │hp bar  │hp bar  │hp bar  │ │ 离开  │ │
│  │name    │name    │name    │name    │ │       │ │
│  └────────┴────────┴────────┴────────┘ └───────┘ │
└──────────────────────────────────────────────────┘
```

## Z-Index Stack

| Layer | Element | Position |
|-------|---------|----------|
| Z-0 | Background + decorative blurs | absolute inset-0 |
| Z-10 | Enemy pokemon (top-right) | top:5% right:6% w:26% h:38% |
| Z-10 | Player pokemon (bottom-left) | bottom:26% left:5% w:33% h:48% |
| Z-10 | Bottom action bar | bottom:0 h:24% |
| Z-20 | Bench balls | bottom:29% left:1.5% |
| Z-25 | Event overlay (click-through) | absolute inset-0 |
| Z-30 | Turn counter | top:2 right:2 |

## Color Scheme

- Background: dark gradient (#1a1a2e → #050510)
- HP bar: green (>50%) / yellow (>20%) / red (≤20%)
- HP bar flash: dmg-flash (bright red glow), heal-flash (bright green glow)
- Move buttons: gray (#374151) default, blue (#1e40af) selected, disabled (darker)
- Switch buttons: gray default, blue selected, red for fainted, green for active
- Confirm button: red (#dc2626) active, gray disabled
- Event bar: dark (#12121a) with gray border (#2a2a35)
- Text: white/light gray (#e5e7eb/#d1d5db)
- Event colors: damage/faint=red, heal=green, switch_in=blue, stat_raise=purple, status=yellow

## CSS Animations

| Animation | Class | Duration | Effect |
|-----------|-------|----------|--------|
| Sprite shake | .shake | 0.4s | translateX oscillation |
| Damage flash | .flash-red | — | red glow filter |
| Heal flash | .flash-green | — | green glow filter |
| DOT flash | .flash-purple | — | purple glow filter |
| Switch-in | .grow | 0.5s | opacity 0→1 + brightness pulse |
| Death | .death | 1s | fade out + translateY(40px) + scale(0.7) |
| Damage number | @keyframes dmgPop | 0.8s | float up + fade out |
| Heal number | @keyframes healPop | 0.8s | float up + fade out |
| Heal arrow | @keyframes healFloat | 0.7s | float up + fade out |
| HP bar width | .hp-bar transition | 0.6s | smooth width transition |

## Component Tree

```
MatchmakingPage.vue
  └─ BattleField.vue
       ├─ IconSprite (pokemon sprites)
       ├─ BattleEvents.vue (event overlay — TO BE REMOVED)
       └─ WeatherField.vue (weather/field indicators)
```

## Props (BattleField)

- `sideA`, `sideB` — full side objects (pokemons[], active, need2switch, sideEffects)
- `turn` — current turn number
- `trigger` — event trigger counter (TO BE REMOVED)
- `messages` — event array (TO BE REMOVED)
- `moves` — move button data with computed _type, _name, _eff, _disabled
- `bench` — bench Pokemon with computed _slot, _isActive, _canSwitch, _hpPct
- `submitting` — boolean, disable confirm button
- `battleStatus` — 'active' | 'completed'
- `weather` — {type, label, duration}
- `field` — {type, label, _terrain, duration}
- `forceSwitch` — boolean, auto-open bench panel

## Emits (BattleField)

- `confirm` — {type:'attack', move_index} or {type:'switch', switch_index}
- `leave` — quit battle
- `switchPokemon` — {switch_index}
- `reset` — exit completed battle
- `eventsComplete` — (TO BE REMOVED)
