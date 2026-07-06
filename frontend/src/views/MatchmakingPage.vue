<template>
  <div>
    <h1 v-if="!activeBattle" class="text-2xl font-bold text-gray-800 mb-6">🔍 匹配对战</h1>
    <div v-if="!inQueue && !activeBattle" class="max-w-lg mx-auto space-y-6">
      <div class="glass-card p-6">
        <h3 class="text-lg font-bold text-gray-800 mb-4">选择队伍</h3>
        <!-- Saved teams -->
        <div v-if="savedTeams.length" class="mb-4">
          <div class="flex items-center justify-between mb-2">
            <span class="text-xs text-gray-400">已保存队伍</span>
            <div class="flex gap-1">
              <button @click="flipTeam(-1)" class="text-xs px-1.5 rounded hover:bg-gray-200 text-gray-500">◀</button>
              <span class="text-xs text-gray-400">{{ teamPage + 1 }}/{{ savedTeams.length }}</span>
              <button @click="flipTeam(1)" class="text-xs px-1.5 rounded hover:bg-gray-200 text-gray-500">▶</button>
            </div>
          </div>
          <div v-if="savedTeams[teamPage]" @click="selectTeam(savedTeams[teamPage])"
            class="rounded-xl p-3 cursor-pointer border-2 transition-all duration-200"
            :class="selectedTeam?.name===savedTeams[teamPage].name
              ? 'bg-pokedex-blue/10 border-pokedex-blue shadow-sm'
              : 'bg-white/50 border-gray-200 hover:border-gray-400 hover:shadow-sm'">
            <div class="text-sm text-gray-800 font-bold">{{ savedTeams[teamPage].name }}</div>
            <div class="text-xs text-gray-500 mt-1">{{ savedTeams[teamPage].pokemon?.length||0 }} 只宝可梦</div>
          </div>
        </div>
        <div v-else class="text-center text-gray-400 text-sm py-4">
          还没有保存的队伍，去 <router-link to="/teams" class="text-pokedex-blue hover:text-pokedex-light font-medium">组队页面</router-link> 创建
        </div>
        <button @click="joinQueue" :disabled="!canJoin"
          class="w-full mt-4 py-3 rounded-full font-bold text-lg transition-all duration-200 shadow-md"
          :class="canJoin ? 'bg-pokeball-red hover:bg-red-600 text-white hover:shadow-lg' : 'bg-gray-200 text-gray-400 cursor-not-allowed'">
          ⚡ 加入匹配队列
        </button>
        <button @click="joinVsBot" :disabled="!canJoin"
          class="w-full mt-3 py-3 rounded-full font-bold text-lg transition-all duration-200 shadow-md"
          :class="canJoin ? 'bg-pokedex-blue hover:bg-blue-600 text-white hover:shadow-lg' : 'bg-gray-200 text-gray-400 cursor-not-allowed'">
          🤖 对战 NPC
        </button>
        <div class="mt-3 text-xs font-medium" :class="wsConnected ? 'text-green-500' : 'text-red-400'">
          {{ wsConnected ? '🟢 已连接' : '🔴 未连接' }}
        </div>
      </div>
    </div>

    <!-- Queue waiting -->
    <div v-if="inQueue && !activeBattle" class="max-w-lg mx-auto text-center py-16">
      <div class="glass-card p-10">
        <div class="animate-spin rounded-full h-14 w-14 border-3 border-pokeball-red border-t-transparent mx-auto mb-5"></div>
        <h2 class="text-xl font-bold text-gray-800 mb-2">寻找对手中...</h2>
        <p class="text-gray-400 text-sm mb-6">正在为你匹配实力相当的训练家</p>
        <button @click="leaveQueue" class="px-6 py-2 bg-gray-200 hover:bg-gray-300 text-gray-600 rounded-full text-sm font-medium transition-colors">取消匹配</button>
      </div>
    </div>

    <!-- Active Battle -->
    <div v-if="activeBattle" class="flex flex-col" style="height:calc(100vh - 48px)">
      <!-- Battle field (takes remaining space) -->
      <div class="flex-1 flex items-center justify-center p-1 min-h-0">
        <BattleField
          :side-a="mySide==='a' ? battleSideA : battleSideB"
          :side-b="mySide==='b' ? battleSideA : battleSideB"
          :turn="turnNumber"
          :messages="events"
          :moves="moveButtons"
          :bench="playerBenchForArena"
          :submitting="submitting"
          :battle-status="activeBattle.status"
          :weather="battleState?._weather"
          :field="battleState?._field"
          :opp-side="mySide==='a' ? battleSideB : battleSideA"
          :force-switch="forceSwitchActive"
          @confirm="onArenaConfirm"
          @leave="onArenaLeave"
          @switch-pokemon="onArenaSwitch"
          @reset="reset"
        />
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onUnmounted, watch } from 'vue'
import BattleField from '../components/battle/BattleField.vue'
import { connect, send, on, getPlayerId, request } from '../api/wsClient'
import { getMove } from '../api/dataWs'
import { startSession, trackTurnAction, trackMatchmake, trackMatchFound, trackBattleInit, trackBattleResult, trackTurnExecuted, trackMatchCancel } from '../utils/analytics'

const inQueue = ref(false)
const wsConnected = ref(false)
const activeBattle = ref(null)
const mySide = ref('a')
const battleState = ref(null)
const loading = ref(false)
const submitting = ref(false)
const actionType = ref('attack')  // 'attack' | 'switch' | 'pass'
const moveInfo = ref({})
const savedTeams = ref([])
const teamPage = ref(0)
function flipTeam(dir) {
  const n = savedTeams.value.length
  if (!n) return
  teamPage.value = dir > 0 ? (teamPage.value + 1) % n : (teamPage.value > 0 ? teamPage.value - 1 : n - 1)
  selectTeam(savedTeams.value[teamPage.value])
}
const selectedTeam = ref(null)
const submittedMsg = ref('')
// Type effectiveness chart (attacker type -> defender type -> multiplier)
const TYPE_CHART = {
  Normal:{Rock:.5,Steel:.5,Ghost:0}, Fighting:{Normal:2,Rock:2,Steel:2,Ice:2,Dark:2,Flying:.5,Poison:.5,Bug:.5,Psychic:.5,Fairy:.5,Ghost:0},
  Flying:{Fighting:2,Bug:2,Grass:2,Rock:.5,Steel:.5,Electric:.5}, Poison:{Grass:2,Fairy:2,Poison:.5,Ground:.5,Rock:.5,Ghost:.5,Steel:0},
  Ground:{Poison:2,Rock:2,Steel:2,Fire:2,Electric:2,Bug:.5,Grass:.5,Flying:0}, Rock:{Flying:2,Bug:2,Fire:2,Ice:2,Fighting:.5,Ground:.5,Steel:.5},
  Bug:{Grass:2,Psychic:2,Dark:2,Fighting:.5,Flying:.5,Poison:.5,Ghost:.5,Steel:.5,Fire:.5,Fairy:.5},
  Ghost:{Psychic:2,Ghost:2,Dark:.5,Normal:0}, Steel:{Ice:2,Rock:2,Fairy:2,Steel:.5,Fire:.5,Water:.5,Electric:.5},
  Fire:{Grass:2,Ice:2,Bug:2,Steel:2,Fire:.5,Water:.5,Rock:.5,Dragon:.5}, Water:{Fire:2,Ground:2,Rock:2,Water:.5,Grass:.5,Dragon:.5},
  Grass:{Water:2,Ground:2,Rock:2,Flying:.5,Poison:.5,Bug:.5,Steel:.5,Fire:.5,Grass:.5,Dragon:.5},
  Electric:{Water:2,Flying:2,Ground:.5,Grass:.5,Electric:.5,Dragon:.5,Ghost:0}, Psychic:{Fighting:2,Poison:2,Psychic:.5,Steel:.5,Dark:0},
  Ice:{Grass:2,Ground:2,Flying:2,Dragon:2,Fire:.5,Water:.5,Ice:.5,Steel:.5}, Dragon:{Dragon:2,Steel:.5,Fairy:0},
  Dark:{Psychic:2,Ghost:2,Fighting:.5,Dark:.5,Fairy:.5}, Fairy:{Fighting:2,Dragon:2,Dark:2,Poison:.5,Steel:.5,Fire:.5},
}
function typeEffectiveness(moveType, defTypes) {
  if (!moveType || !defTypes?.length) return 1
  let m = 1
  for (const dt of defTypes) {
    const t = dt?.toLowerCase?.() || ''
    const cap = t.charAt(0).toUpperCase() + t.slice(1)
    const chart = TYPE_CHART[moveType] || {}
    m *= chart[cap] ?? 1
  }
  return m
}

const canJoin = computed(() => selectedTeam.value?.pokemon?.length > 0)
const turnNumber = computed(() => battleState.value?.turn ?? 0)
const forceSwitchActive = computed(() => {
  const sidesArr = sides.value
  const side = mySide.value === 'a' ? sidesArr[0] : sidesArr[1]
  return side?.need2switch || false
})
const sides = computed(() => battleState.value?.battle?.sides || [])
const battleSideA = computed(() => sides.value[0] || null)
const battleSideB = computed(() => sides.value[1] || null)
// Move buttons (type from daemon, effectiveness vs opponent)
const moveButtons = computed(() => {
  const sidesArr = sides.value
  const side = mySide.value === 'a' ? sidesArr[0] : sidesArr[1]
  const oppSide = mySide.value === 'a' ? sidesArr[1] : sidesArr[0]
  const p = side?.pokemons?.[side?.active||0]
  const opp = oppSide?.pokemons?.[oppSide?.active||0]
  const oppTypes = opp?.types || []
  const chargingId = p?._charging || null  // locked into charging move
  return (p?.moves||[]).map((m) => {
    const moveType = m._type || 'Normal'
    if (m.id && !moveInfo.value[m.id]) {
      getMove(m.id).then(data => {
        moveInfo.value[m.id] = { name: data?.name||'#'+m.id, type: moveType, category: data?.category||'' }
      })
    }
    const info = moveInfo.value[m.id] || { type: moveType }
    const eff = typeEffectiveness(info.type || moveType, oppTypes)
    const locked = chargingId && m.id !== chargingId  // disable non-charging moves
    return { ...m, _name: info.name || m._type || '#'+m.id, _type: info.type || moveType, _category: info.category || '', _disabled: m.pp <= 0 || locked, _eff: eff }
  })
})
// Bench data for arena switch mode
const playerBenchForArena = computed(() => {
  const sidesArr = sides.value
  const side = mySide.value === 'a' ? sidesArr[0] : sidesArr[1]
  if (!side) return []
  const activeIdx = side.active || 0
  return (side.pokemons || []).map((p, i) => ({
    ...p,
    _slot: p.slot ?? i,
    _isActive: i === activeIdx,
    _canSwitch: !p.fainted && i !== activeIdx,
    _hpPct: Math.round((p.hp||0)/(p.maxHp||1)*100),
  }))
})

function onArenaConfirm(action) {
  submitting.value = true
  trackTurnAction(action)
  send('submit_action', {
    battle_id: activeBattle.value.id,
    action: { side: mySide.value, ...action }
  })
}
function onArenaLeave() {
  // Quit battle and leave matchmaking pool
  send('quit_battle', { battle_id: activeBattle.value?.id })
  reset()
}
function onArenaSwitch(action) {
  submitting.value = true
  // Check if this is a forced switch (after faint)
  const sides = battleState.value?.battle?.sides || []
  const ourSide = mySide.value==='a' ? sides[0] : sides[1]
  if (ourSide?.need2switch) {
    // Forced switch — doesn't consume turn
    send('force_switch', {
      battle_id: activeBattle.value.id,
      side: mySide.value,
      switch_index: action.switch_index
    })
  } else {
    // Regular switch during turn
    send('submit_action', {
      battle_id: activeBattle.value.id,
      action: { side: mySide.value, type: 'switch', switch_index: action.switch_index }
    })
  }
}
const events = computed(() => battleState.value?.events||[])

// Battle messages for display (last 3 events, auto-fading)
const battleMessages = computed(() => {
  return (events.value||[]).slice(-3).map((ev, i) => ({
    text: ev.description || '',
    _key: (turnNumber.value||0) + '-' + i
  }))
})

const weatherLabel = computed(() => {
  const w = battleState.value?._weather; return w?.type ? `${w.label} ${w.duration}t` : ''
})
let unsubs = []
async function setupWS() {
  try { await connect('Player'); wsConnected.value=true } catch { wsConnected.value=false; setTimeout(setupWS,1500); return }
  const uid = getPlayerId()
  if (uid) {
    try { savedTeams.value = await request('get_user_teams',{user_id:uid}) } catch { savedTeams.value = [] }
  }
  unsubs.push(on('handshake_ok',()=>{wsConnected.value=true}))
  unsubs.push(on('user_teams',(d)=>{savedTeams.value = d}))
  unsubs.push(on('matchmaking_status',(d)=>{
    if(d.status==='waiting') inQueue.value=true
    if(d.status==='already_queued'){ inQueue.value=true; submittedMsg.value='已在队列中，请勿重复加入' }
  }))
  unsubs.push(on('matched',(d)=>{
    inQueue.value=false; activeBattle.value={id:d.battle_id,status:'active'}; mySide.value=d.side
    battleState.value=d.state; loading.value=false
    // Track full team init
    const sides = d.state?.battle?.sides || []
    const teamA = (sides[0]?.pokemons || []).map(p => ({ speciesID: p.speciesId, moves: (p.moves||[]).map(m=>m.id), item: p.item||p.itemId||0, ability: p.abilityId||0, nature: p.nature||3, level: p.level||50 }))
    const teamB = (sides[1]?.pokemons || []).map(p => ({ speciesID: p.speciesId, moves: (p.moves||[]).map(m=>m.id), item: p.item||p.itemId||0, ability: p.abilityId||0, nature: p.nature||3, level: p.level||50 }))
    const oppType = botBattles.value?.[d.battle_id] ? 'bot' : 'human'
    trackBattleInit(d.battle_id, d.side, teamA, teamB, oppType)
  }))
  unsubs.push(on('battle_state_update',(d)=>{
    battleState.value=d.state; loading.value=false
  }))
  unsubs.push(on('turn_processed',(d)=>{
    console.log('[WS] turn_processed received', {turn: d.turn, events: d.state?.events?.length, need2switch: d.state?.battle?.sides?.map(s=>s.need2switch)})
    battleState.value=d.state; submitting.value=false
    if(activeBattle.value)activeBattle.value.status=d.status
    const sides = d.state?.battle?.sides || []
    const ourSide = mySide.value==='a' ? sides[0] : sides[1]
    // Track battle result if completed
    if (d.status === 'completed' || d.game_over) {
      const ourPkm = ourSide?.pokemons || []
      const oppSide = mySide.value==='a' ? sides[1] : sides[0]
      const oppPkm = oppSide?.pokemons || []
      const ownAlive = ourPkm.filter(p => !p.fainted).length
      const oppAlive = oppPkm.filter(p => !p.fainted).length
      const winner = ownAlive > 0 && oppAlive === 0 ? 'win' : oppAlive > 0 && ownAlive === 0 ? 'loss' : 'draw'
      trackBattleResult(winner === 'win' ? 'completed_win' : winner === 'loss' ? 'completed_loss' : 'draw', d.turn, ownAlive, oppAlive, winner)
    }
    if (ourSide?.need2switch) {
      console.log('[WS] need2switch detected, switching to switch mode')
      actionType.value = 'switch'
    }
  }))
  unsubs.push(on('force_switch_done',(d)=>{
    console.log('[WS] force_switch_done received', {events: d.state?.events?.length})
    // State already broadcast via turn_processed — just reset action type
    actionType.value = 'attack'
  }))
  unsubs.push(on('action_submitted',()=>{ submittedMsg.value = '已提交, 等待对手...' }))
  // Reconnect: re-fetch battle state after handshake
  unsubs.push(on('handshake_ok', (d) => {
    const uid = d.player_id
    if (uid) send('get_current_battle', { player_id: uid })
  }))
  // Check if already in a battle (page re-entry)
  if (uid) {
    send('get_current_battle', { player_id: uid })
  }
  unsubs.push(on('match_found',(d)=>{
    inQueue.value = false; submittedMsg.value = '匹配成功，引擎启动中...'
  }))
  unsubs.push(on('match_cancelled',(d)=>{
    inQueue.value = false; submittedMsg.value = d.message || '匹配已取消'
    setTimeout(reset, 3000)
  }))
  unsubs.push(on('opponent_disconnected',(d)=>{
    trackBattleResult('disconnected', turnNumber.value, -1, -1, null)
    alert(d.message || '对手已断开连接')
    reset()
  }))
  unsubs.push(on('no_active_battle',()=>{
    // No battle found — just wait on queue/menu screen
  }))
}
setupWS()

function selectTeam(t) { selectedTeam.value = t }
async function joinQueue() {
  const teamJson = JSON.stringify({name:selectedTeam.value.name,pokemon:selectedTeam.value.pokemon})
  trackMatchmake('human')
  send('join_matchmaking',{player_id:getPlayerId(),team_json:teamJson,opponent_type:'human'})
}
async function joinVsBot() {
  const teamJson = JSON.stringify({name:selectedTeam.value.name,pokemon:selectedTeam.value.pokemon})
  trackMatchmake('bot')
  send('join_matchmaking',{player_id:getPlayerId(),team_json:teamJson,opponent_type:'bot'})
}
function leaveQueue(){ trackMatchCancel(); inQueue.value=false }
function reset(){
  trackBattleResult(activeBattle.value?.status === 'completed' ? 'completed' : 'abandoned', turnNumber.value, -1, -1, null)
  activeBattle.value=null;battleState.value=null;submitting.value=false;loading.value=false
  document.body.classList.remove('battle-mode')
}
// Dark background when in battle
watch(activeBattle, (val) => {
  if (val) document.body.classList.add('battle-mode')
  else document.body.classList.remove('battle-mode')
})
onUnmounted(()=>{unsubs.forEach(fn=>fn()); document.body.classList.remove('battle-mode')})
</script>
