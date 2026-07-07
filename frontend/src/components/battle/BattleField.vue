<template>
  <div class="battle-arena relative w-full overflow-hidden rounded-2xl border-2 border-gray-800 shadow-2xl"
       style="aspect-ratio:4/3;max-height:100%">
    <div class="absolute inset-0 battle-bg-dark"></div>
    <div class="absolute bottom-[32%] left-[20%] w-[35%] h-[10%] rounded-[50%] bg-white/5 -translate-x-1/2 blur-sm z-0"></div>
    <div class="absolute top-[15%] right-[15%] w-[26%] h-[7%] rounded-[50%] bg-white/5 blur-sm z-0"></div>

    <!-- Enemy Pokemon -->
    <div v-if="opponent" class="absolute z-10 flex flex-col items-center"
         style="top:5%; right:6%; width:26%; height:38%">
      <div class="w-full mb-1">
        <div class="text-[1.8vw] text-gray-300 mb-0 px-2 leading-tight font-semibold truncate drop-shadow">
          {{ opponent._speciesName || '#'+opponent.speciesId }}
        </div>
        <div class="w-full bg-gray-800 rounded-full overflow-hidden border border-gray-700 relative"
             style="height:clamp(7px,1.5vw,12px)">
          <div class="h-full rounded-full hp-bar" :class="oppHpFlash"
               :style="{width: opponentHpPct+'%', backgroundColor: hpBarColor(opponentHpPct)}"></div>
          <span v-if="oppDmgNum" class="absolute -right-1 -top-1 text-red-400 font-bold drop-shadow"
                style="font-size:clamp(9px,1.3vw,14px);animation: dmgPop .8s ease-out forwards;pointer-events:none">-{{ oppDmgNum }}</span>
          <span v-if="oppHealNum" class="absolute -left-1 -top-1 text-green-400 font-bold drop-shadow"
                style="font-size:clamp(9px,1.3vw,14px);animation: healPop .8s ease-out forwards;pointer-events:none">+{{ oppHealNum }}</span>
        </div>
        <div class="flex justify-between items-center mt-0.5">
          <div v-if="fmtStages(opponent?.statStages).length" class="flex gap-0.5 flex-wrap">
            <span v-for="s in fmtStages(opponent?.statStages)" :key="'opp'+s.name"
                  class="px-1 rounded text-[1.1vw] leading-tight"
                  :class="s.val > 0 ? 'bg-green-700/80 text-green-200' : 'bg-red-700/80 text-red-200'">
              {{ s.name }}{{ s.val > 0 ? '+' : '' }}{{ s.val }}
            </span>
          </div>
          <div v-if="fmtStatus(opponent?.inBattleStatus)" class="text-[1.2vw] text-orange-400">
            {{ fmtStatus(opponent?.inBattleStatus) }}
          </div>
          <div class="text-right text-[2vw] text-gray-400 font-mono leading-tight">
            {{ opponent.hp||0 }}/{{ opponent.maxHp||1 }}
          </div>
        </div>
      </div>
      <div class="sprite-wrap" :class="animOpp" v-show="!spriteHidden.b">
        <img v-if="displayOpponent.speciesId && !oppGifFailed" :src="'/ani/'+formSpriteId(displayOpponent)+'.gif'"
          class="object-contain drop-shadow-xl" style="width:80%;height:75%;image-rendering:pixelated"
          :class="{ 'fainted-overlay': opponentHpPct <= 0 }"
          @error="onOppGifError" />
        <IconSprite v-if="displayOpponent.speciesId && oppGifFailed" :species-id="displayOpponent.speciesId" size="xl"
          :class="{ 'fainted-overlay': opponentHpPct <= 0 }" />
      </div>
    </div>

    <!-- Player Pokemon -->
    <div v-if="player" class="absolute z-10 flex flex-col items-center"
         style="bottom:26%; left:5%; width:33.33%; height:48%">
      <div class="w-[85%] mb-1">
        <div class="text-[2vw] text-gray-300 mb-0 px-2 leading-tight font-semibold truncate drop-shadow">
          {{ player._speciesName || '#'+player.speciesId }}
        </div>
        <div class="w-full bg-gray-800 rounded-full overflow-hidden border border-gray-700 relative"
             style="height:clamp(8px,1.8vw,14px)">
          <div class="h-full rounded-full hp-bar" :class="playerHpFlash"
               :style="{width: playerHpPct+'%', backgroundColor: hpBarColor(playerHpPct)}"></div>
          <span v-if="playerDmgNum" class="absolute -right-1 -top-1 text-red-400 font-bold drop-shadow"
                style="font-size:clamp(10px,1.4vw,15px);animation: dmgPop .8s ease-out forwards;pointer-events:none">-{{ playerDmgNum }}</span>
          <span v-if="playerHealNum" class="absolute -left-1 -top-1 text-green-400 font-bold drop-shadow"
                style="font-size:clamp(10px,1.4vw,15px);animation: healPop .8s ease-out forwards;pointer-events:none">+{{ playerHealNum }}</span>
        </div>
        <div class="flex justify-between items-center mt-0.5">
          <div class="text-left flex items-center gap-1">
            <span v-if="fmtStatus(player?.inBattleStatus)" class="text-[1.4vw] text-orange-400">
              {{ fmtStatus(player?.inBattleStatus) }}
            </span>
          </div>
          <div class="text-right text-[2.2vw] text-gray-400 font-mono leading-tight">
            {{ player.hp||0 }}/{{ player.maxHp||1 }}
          </div>
        </div>
        <div v-if="fmtStages(player?.statStages).length" class="flex gap-0.5 mt-0.5 flex-wrap justify-center">
          <span v-for="s in fmtStages(player?.statStages)" :key="'pl'+s.name"
                class="px-1.5 py-0.5 rounded-full text-[1.3vw] leading-tight font-bold"
                :class="s.val > 0 ? 'bg-green-600/80 text-green-200' : 'bg-red-600/80 text-red-200'">
            {{ s.name }}{{ s.val > 0 ? '+' : '' }}{{ s.val }}
          </span>
        </div>
      </div>
      <div class="sprite-wrap" :class="animPlayer" v-show="!spriteHidden.a">
        <img v-if="displayPlayer.speciesId && !playerGifFailed" :src="playerBackSrc"
          class="object-contain drop-shadow-xl" style="width:90%;height:78%;image-rendering:pixelated"
          :class="{ 'fainted-overlay': playerHpPct <= 0 }"
          @error="onPlayerBackError" />
        <IconSprite v-if="displayPlayer.speciesId && playerGifFailed" :species-id="displayPlayer.speciesId" size="xl" back
          :class="{ 'fainted-overlay': playerHpPct <= 0 }" />
      </div>
    </div>

    <!-- Bench + Switch button -->
    <div class="absolute z-20 flex flex-col items-center gap-1.5" style="bottom:29%; left:1.5%">
      <button @click="localShowSwitch = !localShowSwitch"
              class="rounded-full border-2 flex items-center justify-center transition-all font-bold shadow-lg"
              :class="localShowSwitch ? 'border-amber-400 bg-amber-500/40 text-amber-200' : 'border-gray-600 bg-gray-900/80 text-gray-400 hover:border-gray-400'"
              style="width:clamp(34px,5.4vw,53px);height:clamp(34px,5.4vw,53px);font-size:clamp(14px,2.2vw,24px)"
              title="切换宝可梦">↔</button>
      <div v-for="p in playerBench" :key="'pb'+p._slot"
           class="rounded-full flex items-center justify-center shrink-0 overflow-hidden cursor-pointer hover:scale-110 transition-transform shadow-md"
           :class="p.fainted ? 'border-red-500 bg-red-900/60' : 'border-green-500'"
           :style="benchStyle(p)"
           style="width:clamp(34px,5.4vw,53px);height:clamp(34px,5.4vw,53px);background:rgba(0,0,0,0.6)"
           @click="localShowSwitch = true; localSwitchTarget = p._slot">
        <IconSprite v-if="!p.fainted" :species-id="p.speciesId" size="md" />
        <span v-else style="font-size:clamp(10px,1.5vw,16px)">💀</span>
      </div>
    </div>

    <!-- Bottom Bar: Moves / Switch / Confirm -->
    <div class="absolute bottom-0 left-0 right-0 z-10 flex"
         style="height:24%; background:#111118; border-top:1px solid #2a2a35">
      <div class="flex-1 flex gap-2 p-2.5 h-full items-stretch min-w-0">
        <template v-if="!localShowSwitch">
          <button v-for="(m,i) in moves" :key="'mv'+i"
                  @click="selectedMoveIdx = i"
                  :disabled="m._disabled || submitting"
                  class="flex-1 rounded-lg border-2 transition-all duration-150 flex flex-col min-w-0 relative"
                  :class="(m._disabled||submitting)
                    ? 'bg-gray-800 border-gray-700 text-gray-600 cursor-not-allowed'
                    : selectedMoveIdx === i
                      ? 'bg-blue-900/60 border-blue-400 text-white shadow-md'
                      : 'bg-gray-800 border-gray-600 text-gray-200 hover:border-gray-400 hover:bg-gray-750'">
            <div class="flex justify-between px-1.5 pt-1">
              <img v-if="m._type" :src="iconUrl('types', m._type)" style="height:clamp(10px,1.7vw,20px);width:auto" />
              <img v-if="m._category" :src="iconUrl('categories', m._category)" style="height:clamp(10px,1.7vw,20px);width:auto" />
            </div>
            <div class="flex-1 flex items-center justify-center px-1">
              <div class="font-bold truncate text-center" style="font-size:clamp(12px,1.8vw,18px)">
                {{ m._name || '#'+m.id }}
                <span v-if="m._disabled && m.pp > 0" class="text-yellow-400" style="font-size:clamp(8px,1vw,11px)">{{ m.disabled === 'locked' ? '🔒' : m.disabled ? '🚫' : '🔒充能中' }}</span>
              </div>
            </div>
            <div class="flex justify-between items-end px-1.5 pb-1">
              <span v-if="m._eff >= 2" class="font-bold text-green-400" style="font-size:clamp(7px,1vw,11px)">
                {{ m._eff === 4 ? '效果绝佳' : '效果拔群' }}</span>
              <span v-else-if="m._eff > 0 && m._eff < 1" class="font-bold text-orange-400" style="font-size:clamp(7px,1vw,11px)">
                {{ m._eff === 0.5 ? '不理想' : '很不理想' }}</span>
              <span v-else-if="m._eff === 0" class="font-bold text-gray-500" style="font-size:clamp(7px,1vw,11px)">无效</span>
              <span v-else></span>
              <span class="font-mono" :class="(m._disabled||submitting)?'text-gray-600':'text-gray-400'" style="font-size:clamp(7px,1vw,11px)">PP {{ m.pp }}/{{ m.maxPp }}</span>
            </div>
          </button>
        </template>
        <template v-else>
          <button v-for="(p,i) in bench" :key="'sw'+p._slot"
                  @click="p._canSwitch ? localSwitchTarget = p._slot : null"
                  :disabled="!p._canSwitch || submitting"
                  class="flex-1 rounded-lg border-2 transition-all duration-150 flex flex-col justify-center items-center px-2 min-w-0"
                  :class="p._isActive
                    ? 'bg-green-900/50 border-green-500 text-green-300 cursor-default'
                    : p.fainted
                      ? 'bg-red-950/60 border-red-700 text-red-300/60 cursor-not-allowed'
                      : localSwitchTarget === p._slot
                        ? 'bg-blue-900/60 border-blue-400 text-white shadow-md'
                        : 'bg-gray-800 border-gray-600 text-gray-200 hover:border-gray-400 cursor-pointer'">
            <IconSprite v-if="!p.fainted" :species-id="p.speciesId" size="md" />
            <span v-else style="font-size:clamp(12px,1.8vw,20px)">💀</span>
            <div class="w-[80%] bg-gray-800 rounded-full overflow-hidden mt-1" style="height:clamp(4px,0.7vw,8px)">
              <div class="h-full rounded-full" :style="{width:p._hpPct+'%', backgroundColor: hpBarColor(p._hpPct)}"></div>
            </div>
            <div class="text-gray-500 font-mono mt-0.5" style="font-size:clamp(7px,1vw,10px)">{{ p.hp||0 }}/{{ p.maxHp||1 }}</div>
            <div class="font-bold truncate mt-0.5" style="font-size:clamp(9px,1.2vw,13px)">{{ p._speciesName || '#'+p.speciesId }}</div>
          </button>
        </template>
      </div>
      <div class="shrink-0 flex flex-col gap-1.5 justify-center px-2" style="width:clamp(56px,9vw,100px)">
        <button v-if="battleStatus==='active'" @click="doConfirm"
                :disabled="submitting || (!localShowSwitch && selectedMoveIdx < 0) || (localShowSwitch && localSwitchTarget < 0)"
                class="flex-1 rounded-xl font-bold text-white transition-all duration-200 shadow-md text-center"
                :class="submitting || (!localShowSwitch && selectedMoveIdx < 0) || (localShowSwitch && localSwitchTarget < 0)
                  ? 'bg-gray-700 cursor-not-allowed text-gray-500'
                  : 'bg-red-600 hover:bg-red-500'"
                style="font-size:clamp(10px,1.5vw,16px)">{{ submitting ? '⏳' : '确认' }}</button>
        <button v-if="battleStatus==='active'" @click="$emit('leave')"
                class="flex-1 rounded-lg font-medium text-red-400 bg-gray-800 hover:bg-red-900/30 border border-red-800/50 transition-all text-center"
                style="font-size:clamp(8px,1.1vw,12px)">离开</button>
        <button v-if="battleStatus==='completed'" @click="$emit('reset')"
                class="flex-1 rounded-xl font-bold text-white bg-amber-600 hover:bg-amber-500 transition-all text-center"
                style="font-size:clamp(10px,1.5vw,16px)">退出</button>
      </div>
    </div>

    <!-- Event text bar (replaces BattleEvents — state-driven, no animation coupling) -->
    <div v-if="currentLog" class="event-overlay" @click="advanceLog">
      <div class="event-bar">
        <span class="event-text" :class="'ev-'+currentLog.event_type">{{ currentLog.description }}</span>
        <span class="hint">点击继续</span>
      </div>
    </div>

    <WeatherField :weather="weather" :field="field" />
    <div class="absolute top-2 right-2 z-30">
      <span class="px-3 py-1 rounded-full font-mono font-bold bg-black/80 backdrop-blur shadow transition-all duration-150"
            :class="turnBump ? 'text-amber-300 scale-110' : 'text-gray-400'"
            style="font-size:clamp(10px,1.3vw,15px)">Turn {{ turn }}</span>
    </div>
  </div>
</template>

<script setup>
import { computed, ref, watch } from 'vue'
import IconSprite from '../shared/IconSprite.vue'
import WeatherField from './WeatherField.vue'
import { frontSprite, backSprite, cdnBackSprite } from '../../utils/spriteUrl.js'
import { trackDamage, trackFaint, trackSwitch, setTurn } from '../../utils/analytics'

function formSpriteId(p) {
  if (p?._formSpriteId) return p._formSpriteId
  return p?.speciesId
}

// ═══════════ Props ═══════════
const props = defineProps({
  sideA: Object, sideB: Object, turn: Number, messages: Array, moves: Array,
  bench: Array, submitting: Boolean, battleStatus: String, weather: Object,
  field: Object, oppSide: Object, forceSwitch: { type: Boolean, default: false },
})

const emit = defineEmits(['confirm', 'leave', 'switchPokemon', 'reset'])

// ═══════════ Core state (direct reads — no animation shadow layer) ═══════════
const playerSide = computed(() => props.sideA)
const opponentSide = computed(() => props.sideB)
const player = computed(() => { const s = playerSide.value; return s ? (s.pokemons||[])[s.active||0] : null })
const opponent = computed(() => { const s = opponentSide.value; return s ? (s.pokemons||[])[s.active||0] : null })

// Frozen sprite: during death→switch animation, show the OLD Pokemon's sprite
// so death animation plays on the correct (old) Pokemon. null = use current state.
const _frozenSprite = ref({ a: null, b: null })
const displayPlayer = computed(() => _frozenSprite.value.a || player.value)
const displayOpponent = computed(() => _frozenSprite.value.b || opponent.value)

const playerHpPct = computed(() => { const p = player.value; return Math.round((p?.hp||0)/(p?.maxHp||1)*100) })
const opponentHpPct = computed(() => { const p = opponent.value; return Math.round((p?.hp||0)/(p?.maxHp||1)*100) })

const playerBench = computed(() => { const s = playerSide.value; return s ? (s.pokemons||[]).map((p,i)=>({...p,_slot:i})).filter(p=>p._slot!==(s.active||0)) : [] })

// ═══════════ CSS animation triggers (transient, set by state-diff watcher) ═══════════
const animPlayer = ref(''), animOpp = ref('')
const playerHpFlash = ref(''), oppHpFlash = ref('')
const playerDmgNum = ref(0), oppDmgNum = ref(0)
const playerHealNum = ref(0), oppHealNum = ref(0)
const spriteHidden = ref({ a: false, b: false })
const turnBump = ref(false)  // brief flash when turn advances without HP change
const animTimer = { a: null, b: null }

// ═══════════ State-diff: detect changes, trigger CSS animations ═══════════
const _prevA = ref(null), _prevB = ref(null)
const _lastTurn = ref(0)  // track last processed turn to detect true duplicates

function snapshot(p) {
  if (!p) return null
  return { speciesId: p.speciesId, _speciesName: p._speciesName, hp: p.hp, maxHp: p.maxHp || 1 }
}

let _baselineEstablished = false

watch(() => [player.value, opponent.value, props.turn], () => {
  const a = player.value; const b = opponent.value
  const snapA = snapshot(a); const snapB = snapshot(b)
  const curTurn = props.turn || 0
  console.log(`[diff] turn=${curTurn} player=${snapA?.speciesId||'?'}:${snapA?.hp||0}/${snapA?.maxHp||1} opp=${snapB?.speciesId||'?'}:${snapB?.hp||0}/${snapB?.maxHp||1}`,
    `\n  prevA=${_prevA.value?.speciesId||'?'}:${_prevA.value?.hp||0} prevB=${_prevB.value?.speciesId||'?'}:${_prevB.value?.hp||0} lastTurn=${_lastTurn.value} baseline=${_baselineEstablished}`)

  if (!_baselineEstablished && snapA && snapB) {
    // First time we have real data — set baseline, skip animations
    _prevA.value = snapA; _prevB.value = snapB; _lastTurn.value = curTurn; _baselineEstablished = true
    console.log(`[diff] baseline set: #${snapA.speciesId}:${snapA.hp} / #${snapB.speciesId}:${snapB.hp}`)
    return
  }

  const sameSpecies = snapA && _prevA.value &&
    snapA.speciesId === _prevA.value.speciesId && snapA.hp === _prevA.value.hp &&
    snapB && _prevB.value &&
    snapB.speciesId === _prevB.value.speciesId && snapB.hp === _prevB.value.hp

  // Only skip as duplicate if BOTH turn and state are identical
  if (sameSpecies && curTurn === _lastTurn.value && curTurn > 0) {
    console.log(`[diff] true duplicate (same turn=${curTurn}, same state) — skipping`)
    return
  }

  // State changed OR turn advanced — always process
  if (sameSpecies && curTurn !== _lastTurn.value) {
    console.log(`[diff] turn advanced (${_lastTurn.value}→${curTurn}) but state unchanged — updating baseline only`)
    // No HP/species change to animate, but flash the turn counter for feedback
    turnBump.value = true
    setTimeout(() => { turnBump.value = false }, 400)
    _prevA.value = snapA; _prevB.value = snapB; _lastTurn.value = curTurn
    return
  }

  if (a && snapA) applyDiff('a', _prevA.value, snapA)
  if (b && snapB) applyDiff('b', _prevB.value, snapB)
  // Only snapshot non-null state — preserves previous state across intermediate nulls
  if (snapA) _prevA.value = snapA
  if (snapB) _prevB.value = snapB
  _lastTurn.value = curTurn
}, { immediate: true })

function applyDiff(key, prev, curr) {
  if (!curr) return
  const s = key === 'a' ? animPlayer : animOpp
  const hpFlash = key === 'a' ? playerHpFlash : oppHpFlash
  const dmgNum = key === 'a' ? playerDmgNum : oppDmgNum
  const healNum = key === 'a' ? playerHealNum : oppHealNum
  const sideName = key === 'a' ? 'player' : 'opp'
  clearTimeout(animTimer[key])

  // Inferred faint: prev had a different Pokemon alive, now replaced = old one fainted.
  // Freeze old sprite so death animation plays on the correct (departed) Pokemon.
  if (prev && prev.speciesId && prev.speciesId !== curr.speciesId) {
    trackFaint(prev.speciesId, key); trackSwitch(curr.speciesId, key, "faint_replace");
    const switchInDmg = (curr.maxHp || 1) - curr.hp  // damage taken in same frame as switch-in
    console.log(`[anim] ${sideName} INFERRED-FAINT #${prev.speciesId} (${prev.hp}→0) → death → #${curr.speciesId} switch-in` +
      (switchInDmg > 0 ? ` (took ${switchInDmg} dmg on entry: ${curr.hp}/${curr.maxHp})` : ''))
    _frozenSprite.value[key] = { speciesId: prev.speciesId, _speciesName: prev._speciesName || '', hp: 0, maxHp: prev.maxHp }
    s.value = 'death'; hpFlash.value = ''; dmgNum.value = 0; healNum.value = 0
    animTimer[key] = setTimeout(() => {
      _frozenSprite.value[key] = null  // unfreeze: show new Pokemon
      spriteHidden.value[key] = false
      s.value = 'grow'
      console.log(`[anim] ${sideName} SWITCH_IN #${curr.speciesId} (after faint) hp=${curr.hp}/${curr.maxHp}`)
      animTimer[key] = setTimeout(() => {
        s.value = ''
        // If the new Pokemon took damage in the same frame (entry hazard, faster opponent),
        // play damage animation after the grow settles
        if (switchInDmg > 0) {
          dmgNum.value = switchInDmg
          s.value = 'shake'; hpFlash.value = 'dmg-flash'
          console.log(`[anim] ${sideName} ENTRY-DMG -${switchInDmg} (${curr.hp}/${curr.maxHp})`)
          animTimer[key] = setTimeout(() => { s.value = ''; hpFlash.value = ''; dmgNum.value = 0 }, 600)
        }
      }, 500)
    }, 800)
    return
  }

  if (!prev || prev.speciesId !== curr.speciesId) {
    // New Pokemon (first time or direct switch without faint)
    trackSwitch(curr.speciesId, key, "switch");
    console.log(`[anim] ${sideName} SWITCH_IN #${prev?.speciesId||'?'}→#${curr.speciesId} hp=${curr.hp}/${curr.maxHp} spriteHidden=${spriteHidden.value[key]}`)
    spriteHidden.value[key] = false
    s.value = 'grow'; hpFlash.value = ''; dmgNum.value = 0; healNum.value = 0
    animTimer[key] = setTimeout(() => { s.value = '' }, 500)
  } else if (prev.hp > curr.hp) {
    // Damage taken
    const dmg = prev.hp - curr.hp
    console.log(`[anim] ${sideName} DAMAGE -${dmg} (${prev.hp}→${curr.hp})`)
    dmgNum.value = dmg
    s.value = 'shake'; hpFlash.value = 'dmg-flash'
    animTimer[key] = setTimeout(() => { s.value = ''; hpFlash.value = ''; dmgNum.value = 0 }, 600)
  } else if (prev.hp < curr.hp) {
    // HP restored
    const heal = curr.hp - prev.hp
    console.log(`[anim] ${sideName} HEAL +${heal} (${prev.hp}→${curr.hp})`)
    healNum.value = heal
    s.value = 'flash-green'; hpFlash.value = 'heal-flash'
    animTimer[key] = setTimeout(() => { s.value = ''; hpFlash.value = ''; healNum.value = 0 }, 800)
  } else {
    console.log(`[anim] ${sideName} NO-DIFF #${curr.speciesId} hp=${curr.hp} — state unchanged, skipping animation`)
  }

  if (curr.hp <= 0 && (!prev || prev.hp > 0)) {
        trackFaint(curr.speciesId, key);
    // Direct faint (HP reached 0 in the diff — no switch after)
    console.log(`[anim] ${sideName} FAINT — death animation, hide after 1.2s`)
    s.value = 'death'
    animTimer[key] = setTimeout(() => { s.value = ''; spriteHidden.value[key] = true; console.log(`[anim] ${sideName} sprite hidden after faint`) }, 1200)
  }
}

// ═══════════ Text log (driven by messages prop, not events) ═══════════
const currentLog = ref(null)
const logQueue = ref([])

watch(() => props.turn, () => { loadLogs() })

function loadLogs() {
  logQueue.value = [...(props.messages || [])]
  currentLog.value = null
  console.log(`[log] turn ${props.turn}: ${logQueue.value.length} messages loaded:`,
    logQueue.value.map(e => `${e.event_type}(${e.side}):${e.description?.slice(0,30)}`))
  setTimeout(advanceLog, 200)  // small delay so HP animation starts first
}
function advanceLog() {
  currentLog.value = logQueue.value.shift() || null
  if (currentLog.value) console.log(`[log] show: ${currentLog.value.event_type} "${currentLog.value.description}"`)
}

// ═══════════ Selection state ═══════════
const selectedMoveIdx = ref(-1)
const localShowSwitch = ref(false)
const localSwitchTarget = ref(-1)

watch(() => props.moves, () => { selectedMoveIdx.value = -1 })
watch(() => props.turn, (t, old) => { setTurn(t);
  console.log(`[turn] ${old||0}→${t} | msgs=${(props.messages||[]).length} | moves=${(props.moves||[]).length} | fainted=${player.value?.hp<=0||opponent.value?.hp<=0} | force=${props.forceSwitch}`)
  selectedMoveIdx.value = -1; localShowSwitch.value = false; localSwitchTarget.value = -1
})
watch(() => props.forceSwitch, (v) => { if (v) { console.log(`[ui] forceSwitch=true → auto-open bench`); localShowSwitch.value = true } })

function doConfirm() {
  console.log(`[ui] doConfirm: submitting=${props.submitting} switchMode=${localShowSwitch.value} switchTarget=${localSwitchTarget.value} selectedMove=${selectedMoveIdx.value} moves=${props.moves?.map(m=>`#${m.id} pp${m.pp} ${m._disabled?'DISABLED':''}`).join(',')}`)
  if (props.submitting) { console.log('[ui] doConfirm BLOCKED: submitting'); return }
  if (localShowSwitch.value && localSwitchTarget.value >= 0) {
    emit('switchPokemon', { switch_index: localSwitchTarget.value }); localShowSwitch.value = false
    console.log('[ui] doConfirm: emitted switchPokemon')
  } else if (selectedMoveIdx.value >= 0) {
    const mv = props.moves?.[selectedMoveIdx.value]
    if (mv?._disabled) { console.log(`[ui] doConfirm BLOCKED: move ${selectedMoveIdx.value} is disabled`); return }
    emit('confirm', { type: 'attack', move_index: selectedMoveIdx.value })
    console.log('[ui] doConfirm: emitted confirm')
  } else {
    console.log('[ui] doConfirm: nothing selected')
  }
}

// ═══════════ Display helpers ═══════════
const STAT_NAMES = ['物攻','物防','特攻','特防','速度']
const STATUS_NAMES = { brn:'🔥烧伤', frz:'❄️冻结', par:'⚡麻痹', psn:'☠️中毒', tox:'☠️剧毒', slp:'💤睡眠' }
function fmtStages(s) { return (s||[]).slice(0,5).map((v,i)=>({name:STAT_NAMES[i],val:v})).filter(x=>x.val!==0) }
function fmtStatus(l) { return (l||[]).map(s=>STATUS_NAMES[s.name]||s.name).join(' ') }
function hpBarColor(pct) { if (pct > 50) return '#4ade80'; if (pct > 20) return '#facc15'; return '#ef4444' }
function iconUrl(type, name) { if (!name) return ''; const n = name.toLowerCase(); return `/${type}/${n.charAt(0).toUpperCase()+n.slice(1)}.png` }
function benchStyle(p) { const pct = p ? Math.round((p.hp||0)/(p.maxHp||1)*100) : 0; if (pct <= 0) return { borderColor: '#7f1d1d', borderWidth: '2px' }; return { borderWidth: Math.max(2, Math.round(pct/40))+'px' } }

// ═══════════ Sprite fallback ═══════════
const CDN_GEN5 = 'https://play.pokemonshowdown.com/gen5'
const oppGifFailed = ref(false), oppImgStep = ref(0)
watch(() => displayOpponent.value?.speciesId, () => { oppGifFailed.value = false; oppImgStep.value = 0 })
function onOppGifError(e) {
  oppImgStep.value++
  const el = e.target
  if (oppImgStep.value === 1) { el.src = el.src.replace('.gif','.png'); return }
  if (oppImgStep.value === 2) { el.src = `${CDN_GEN5}/${formSpriteId(displayOpponent.value)}.png`; return }
  oppGifFailed.value = true
}

const playerGifFailed = ref(false), playerBackStep = ref(0)
const playerBackSrc = computed(() => {
  const p = displayPlayer.value; if (!p?.speciesId) return ''
  const sid = formSpriteId(p)
  if (playerBackStep.value === 0) return backSprite(sid)
  if (playerBackStep.value === 1) return backSprite(sid).replace('.gif','.png')
  return cdnBackSprite(sid, p._speciesName)
})
function onPlayerBackError(e) {
  playerBackStep.value++
  if (playerBackStep.value >= 3) playerGifFailed.value = true
}
watch(() => displayPlayer.value?.speciesId, () => { playerGifFailed.value = false; playerBackStep.value = 0 })
</script>

<style scoped>
.battle-bg-dark { background: radial-gradient(ellipse at 50% 60%, #1a1a2e 0%, #0d0d1a 50%, #050510 100%); }
.sprite-wrap { position: relative; width: 100%; height: 100%; display: flex; align-items: center; justify-content: center; }
.shake { animation: pokeShake .4s ease-in-out; }
@keyframes pokeShake { 0%,100% { transform: translateX(0); } 10%,50%,90% { transform: translateX(-4px); } 30%,70% { transform: translateX(4px); } }
.flash-red { filter: brightness(1.4) drop-shadow(0 0 8px rgba(255,60,60,.7)); }
.flash-red img, .flash-red svg { filter: hue-rotate(-10deg) saturate(1.5) brightness(1.2); }
.flash-green { filter: brightness(1.3) drop-shadow(0 0 10px rgba(60,255,100,.7)); }
.flash-green img, .flash-green svg { filter: hue-rotate(30deg) saturate(1.3) brightness(1.15); }
.grow { animation: pokeGrow .5s ease-out; }
@keyframes pokeGrow { from { opacity: 0; filter: brightness(1.5); } to { opacity: 1; filter: brightness(1); } }
.death { animation: pokeDeath 1s ease-in forwards; }
@keyframes pokeDeath { 0% { opacity: 1; transform: translateY(0); filter: brightness(1); } 30% { opacity: .6; filter: brightness(2) saturate(.3); } 100% { opacity: 0; transform: translateY(40px) scale(.7); filter: brightness(.3) saturate(0); } }
@keyframes dmgPop { 0% { transform: translateY(0); opacity:1; } 100% { transform: translateY(-16px); opacity:0; } }
@keyframes healPop { 0% { transform: translateY(0); opacity:1; } 100% { transform: translateY(-18px); opacity:0; } }
.hp-bar { transition: width .6s ease-out; }
.hp-bar.dmg-flash { filter: brightness(1.5) drop-shadow(0 0 6px rgba(255,60,60,.8)); }
.hp-bar.heal-flash { filter: brightness(1.5) drop-shadow(0 0 8px rgba(60,255,100,.8)); }
.fainted-overlay { filter: brightness(0.6) saturate(0.3); opacity: 0.65; }
.event-overlay {
  position: absolute; top: 0; left: 0; right: 0; bottom: 0; z-index: 25;
  display: flex; align-items: flex-end; cursor: pointer; user-select: none;
}
.event-bar {
  width: 100%; height: 24%; display: flex; align-items: center; justify-content: center; gap: 12px;
  background: #12121a; border-top: 1px solid #2a2a35;
  font-size: clamp(14px, 1.8vw, 20px); font-weight: 700; color: #e5e7eb;
}
.event-text { white-space: nowrap; }
.ev-damage, .ev-faint { color: #f87171; }
.ev-heal { color: #4ade80; }
.ev-stat_raise { color: #c4b5fd; }
.ev-stat_drop { color: #fca5a5; }
.ev-switch_in { color: #60a5fa; }
.ev-status_apply { color: #fbbf24; }
.ev-ability_trigger { color: #f0abfc; }
.ev-weather { color: #67e8f9; }
.hint { font-size: .55em; color: #555; font-weight: 400; }
</style>
