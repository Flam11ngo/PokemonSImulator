import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { send, on, connect, getPlayerId } from '../api/wsClient'

export const useBattleStore = defineStore('battle', () => {
  const currentBattle = ref(null)
  const battleState = ref(null)
  const formAliases = ref({})     // alias_name → {sid, spriteName}
  const loading = ref(false)
  const error = ref(null)
  const connected = ref(false)

  const isActive = computed(() =>
    currentBattle.value?.status === 'active' || currentBattle.value?.status === 'pending'
  )
  const turnNumber = computed(() => battleState.value?.turn ?? 0)

  // Setup WS listeners once
  let _inited = false
  function init() {
    if (_inited) return
    _inited = true

    on('handshake_ok', () => { connected.value = true })
    on('pong', () => { connected.value = true })

    on('battle_created', (data) => {
      loading.value = false
      currentBattle.value = data.battle
      battleState.value = data.state
      formAliases.value = data.formAliases || {}
    })

    on('turn_processed', (data) => {
      loading.value = false
      battleState.value = data.state
      if (currentBattle.value) {
        currentBattle.value.total_turns = data.turn
        currentBattle.value.status = data.status
      }
    })

    on('battle_data', (data) => {
      if (data && data.id) {
        currentBattle.value = data
        battleState.value = data.current_state
      }
    })

    on('error', (data) => {
      loading.value = false
      error.value = data.message
    })
  }

  async function ensureConnected() {
    init()
    if (!connected.value) {
      await connect('Player_' + Math.random().toString(36).slice(2,6))
    }
  }

  async function createBattle(teamAJson, teamBJson, seed = 0) {
    await ensureConnected()
    loading.value = true
    error.value = null
    send('create_battle', { team_a_json: teamAJson, team_b_json: teamBJson, seed })
  }

  async function processTurn(actions) {
    if (!currentBattle.value) return
    loading.value = true
    error.value = null
    send('process_turn', { battle_id: currentBattle.value.id, actions })
  }

  async function loadBattle(id) {
    await ensureConnected()
    loading.value = true
    send('get_battle', { battle_id: id })
  }

  function updateState(state) {
    battleState.value = state
    if (currentBattle.value) currentBattle.value.current_state = state
  }

  function reset() {
    currentBattle.value = null
    battleState.value = null
    error.value = null
  }

  return {
    currentBattle, battleState, formAliases, loading, error, connected,
    isActive, turnNumber,
    ensureConnected, createBattle, processTurn, loadBattle, updateState, reset,
  }
})
