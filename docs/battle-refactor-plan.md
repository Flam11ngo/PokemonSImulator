# BattleField Refactor Plan

## Goal
Delete the event-driven animation system. Replace with Showdown-style state-driven rendering.
Layout stays 100% identical. Only the `<script>` section changes.

## What Gets Deleted

| File | Remove |
|------|--------|
| `BattleEvents.vue` | **Entire file** — event queue, click-through, `processedKeys`, `onAnimate` emit |
| `BattleField.vue` | `onAnimate()` function (50+ lines of event-type switches) |
| `BattleField.vue` | `_animHp`, `_animMaxHp`, `_displayBase`, `_spriteHidden` refs |
| `BattleField.vue` | `onEventsBusy()`, `syncDisplayFromState()` |
| `BattleField.vue` | `clearAnim()`, `logR()`, animation timer refs |
| `BattleField.vue` | `eventsBusy`, `_eventsActive` refs |
| `MatchmakingPage.vue` | `eventTrigger`, `events` computed, `eventsComplete` handler |

## What Gets Added

### BattleField.vue — State diff watcher

```js
// Previous state snapshot for diffing
const _prevA = ref(null)
const _prevB = ref(null)

// Auto-advancing text display (replaces BattleEvents)
const logQueue = ref([])
const currentLog = ref(null)
let logTimer = null

// Watch for state changes → trigger animations
watch(() => [player.value, opponent.value, props.turn], () => {
  const a = player.value; const b = opponent.value
  if (a) diffAndApply('a', _prevA.value, a)
  if (b) diffAndApply('b', _prevB.value, b)
  _prevA.value = a ? snapshot(a) : null
  _prevB.value = b ? snapshot(b) : null
}, { deep: true })
```

### diffAndApply(key, prev, curr)

```js
function diffAndApply(key, prev, curr) {
  const s = key === 'a' ? animPlayer : animOpp
  const hp = key === 'a' ? playerHpFlash : oppHpFlash
  const dmg = key === 'a' ? playerDmgNum : oppDmgNum
  const heal = key === 'a' ? playerHealNum : oppHealNum

  if (!prev || prev.speciesId !== curr.speciesId) {
    // Switch-in: grow animation
    s.value = 'grow'
    dmg.value = 0; heal.value = 0
    setTimeout(() => { s.value = '' }, 500)
  } else if (prev.hp > curr.hp) {
    // Damage
    const delta = prev.hp - curr.hp
    dmg.value = delta
    s.value = 'shake'
    hp.value = 'dmg-flash'
    setTimeout(() => { s.value = ''; hp.value = ''; dmg.value = 0 }, 600)
  } else if (prev.hp < curr.hp) {
    // Heal
    const delta = curr.hp - prev.hp
    heal.value = delta
    hp.value = 'heal-flash'
    s.value = 'flash-green'
    setTimeout(() => { s.value = ''; hp.value = ''; heal.value = 0 }, 800)
  }

  if (curr.hp <= 0 && (!prev || prev.hp > 0)) {
    // New faint: death animation, then hide
    s.value = 'death'
    spriteHidden.value[key] = true
    setTimeout(() => { s.value = '' }, 1200)
  }
}

function snapshot(p) {
  return { speciesId: p?.speciesId, hp: p?.hp, maxHp: p?.maxHp }
}
```

### Text log bar (replaces BattleEvents overlay)

```html
<!-- In template, replace <BattleEvents> tag with: -->
<div v-if="currentLog" class="event-overlay" @click="advanceLog">
  <div class="event-bar">
    <span class="event-text" :class="'ev-'+currentLog.event_type">
      {{ currentLog.description }}
    </span>
    <span class="hint">点击继续</span>
  </div>
</div>
```

```js
function loadLogs() {
  logQueue.value = [...(props.messages || [])]
  advanceLog()
}
function advanceLog() {
  clearTimeout(logTimer)
  currentLog.value = logQueue.value.shift() || null
}
```

### Template changes (minimal)

- Remove `<BattleEvents>` tag (line 188)
- Replace with inline event-overlay div
- `:class="animPlayer"` stays, `v-show="!_spriteHidden.a"` → `v-show="playerHpPct > 0 || !spriteHidden.a"`
- `displayPlayer`/`displayOpp` → read directly from `player`/`opponent` (remove resolveDisplayPoke)
- `playerHpPct` computed: `(player.value?.hp||0) / (player.value?.maxHp||1) * 100`
- Remove `eventsBusy` from all `:disabled` bindings

## Files Modified

1. **DELETE**: `frontend/src/components/battle/BattleEvents.vue`
2. **REWRITE (script)**: `frontend/src/components/battle/BattleField.vue` — template stays 95% same
3. **SIMPLIFY**: `frontend/src/views/MatchmakingPage.vue` — remove event trigger/dedup logic

## Layout Preservation

All template HTML/CSS/classes/animations/colors/positions stay EXACTLY as documented in `docs/battle-ui-layout.md`.
