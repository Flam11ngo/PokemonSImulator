/**
 * Analytics / telemetry for PokemonSimulator.
 * Tracks: battle init (full teams), each turn, damage/faint/switch,
 *         player state transitions, UI clicks, team saves.
 * Batches events → WebSocket → server → logs/analytics/*.jsonl
 */
import { getPlayerId, send } from '../api/wsClient'

const BATCH_SIZE = 8
const FLUSH_INTERVAL = 5000

let sessionId = genId()
let eventQueue = []
let flushTimer = null
let battleCtx = null   // { battle_id, side }
let playerState = 'idle'
let stateTimestamp = null

function genId() { return Date.now().toString(36) + '_' + Math.random().toString(36).slice(2, 8) }
function ts() { return new Date().toISOString() }

// ═══════════════════════════════════════════
// Session & player state
// ═══════════════════════════════════════════

export function startSession() {
  sessionId = genId()
  track('session_start', { player_id: getPlayerId() || 'guest' })
  startFlushTimer()
}

export function endSession() {
  track('session_end', { player_id: getPlayerId() || 'guest' })
  flush()
  stopFlushTimer()
}

/** Track player state transitions: idle | matching | teambuilding | battling */
export function setPlayerState(state) {
  if (state === playerState) return
  const prev = playerState
  const prevTs = stateTimestamp
  playerState = state
  stateTimestamp = Date.now()
  track('player_state', {
    from: prev,
    to: state,
    duration_ms: prevTs ? Date.now() - prevTs : 0,
    timestamp: ts(),
  })
}

// ═══════════════════════════════════════════
// Battle context
// ═══════════════════════════════════════════

/**
 * Call when battle is created. teamPokemon must be the full pokemon[] array
 * from side_a / side_b, each with: speciesID, moves[], item, ability, nature.
 */
export function trackBattleInit(battleId, side, teamA, teamB, opponentType) {
  battleCtx = { battle_id: battleId, side, _turn: 0 }
  track('battle_init', {
    battle_id: battleId,
    side,
    opponent_type: opponentType || 'human',
    side_a: teamA.map(p => ({
      speciesID: p.speciesID,
      moves: p.moves || [],
      item: p.item || 0,
      ability: p.ability || 0,
      nature: p.nature ?? 3,
      level: p.level || 50,
    })),
    side_b: teamB.map(p => ({
      speciesID: p.speciesID,
      moves: p.moves || [],
      item: p.item || 0,
      ability: p.ability || 0,
      nature: p.nature ?? 3,
      level: p.level || 50,
    })),
  })
  setPlayerState('battling')
}

/** Call when battle ends. result: 'win'|'loss'|'abandoned'|'disconnected' */
export function trackBattleResult(result, turnCount, remainingOwn, remainingOpp, winner) {
  if (!battleCtx) return
  track('battle_result', {
    battle_id: battleCtx.battle_id,
    side: battleCtx.side,
    result,
    winner,
    turns: turnCount || battleCtx._turn || 0,
    own_remaining: remainingOwn ?? -1,
    opp_remaining: remainingOpp ?? -1,
  })
  battleCtx = null
  setPlayerState('idle')
}

/** Call when a turn is processed. actions: { side_a: {type, move_id?, switch_to?}, side_b: {...} } */
export function trackTurnExecuted(turn, actions) {
  if (!battleCtx) return
  battleCtx._turn = turn
  track('turn_executed', {
    battle_id: battleCtx.battle_id,
    turn,
    side_a: actions?.side_a || null,
    side_b: actions?.side_b || null,
  })
}

// ═══════════════════════════════════════════
// In-turn events (fine-grained, from state-diff)
// ═══════════════════════════════════════════

export function trackDamage(targetSide, speciesId, moveName, damage, fainted) {
  track('turn_damage', {
    battle_id: battleCtx?.battle_id || '',
    turn: battleCtx?._turn || 0,
    target_side: targetSide,
    target_species: speciesId,
    move: moveName || 'unknown',
    damage,
    fainted: !!fainted,
  })
}

export function trackFaint(speciesId, side) {
  track('turn_faint', {
    battle_id: battleCtx?.battle_id || '',
    turn: battleCtx?._turn || 0,
    species: speciesId,
    side,
  })
}

export function trackSwitch(speciesId, side, reason) {
  track('turn_switch', {
    battle_id: battleCtx?.battle_id || '',
    turn: battleCtx?._turn || 0,
    species: speciesId,
    side,
    reason: reason || 'manual',
  })
}

export function trackHeal(targetSide, speciesId, healAmount) {
  track('turn_heal', {
    battle_id: battleCtx?.battle_id || '',
    turn: battleCtx?._turn || 0,
    target_side: targetSide,
    target_species: speciesId,
    heal: healAmount,
  })
}

export function trackAbility(speciesId, side, abilityName) {
  track('turn_ability', {
    battle_id: battleCtx?.battle_id || '',
    turn: battleCtx?._turn || 0,
    species: speciesId,
    side,
    ability: abilityName || 'unknown',
  })
}

export function setTurn(turn) {
  if (battleCtx) battleCtx._turn = turn
}

// ═══════════════════════════════════════════
// UI / navigation
// ═══════════════════════════════════════════

export function trackPageView(page) {
  track('page_view', { page })
}

/** Track specific UI clicks: element name + optional context */
export function trackClick(element, extra) {
  track('ui_click', { element, timestamp: ts(), ...(extra || {}) })
}

export function trackTeamSave(teamName, pokemon) {
  track('team_save', {
    team_name: teamName,
    pokemon_count: (pokemon || []).length,
    pokemon: (pokemon || []).map(p => ({
      speciesID: p.speciesID,
      moves: p.moves || [],
      item: p.item || 0,
      ability: p.ability || 0,
      nature: p.nature ?? 3,
    })),
  })
}

export function trackMatchmake(opponentType) {
  setPlayerState('matching')
  track('matchmaking_join', { opponent_type: opponentType || 'human' })
}

export function trackMatchFound(battleId, side) {
  track('match_found', { battle_id: battleId, side })
}

export function trackMatchCancel() {
  setPlayerState('idle')
  track('matchmaking_cancel', {})
}

// ═══════════════════════════════════════════
// Internal
// ═══════════════════════════════════════════

function track(event, data) {
  const entry = {
    event,
    data,
    session_id: sessionId,
    player_id: getPlayerId() || 'guest',
    timestamp: ts(),
  }
  eventQueue.push(entry)
  console.log(`[analytics] ${event}`, JSON.stringify(data).slice(0, 120))
  if (eventQueue.length >= BATCH_SIZE) flush()
}

function flush() {
  if (!eventQueue.length) return
  const batch = eventQueue.splice(0)
  try { send('analytics_batch', { events: batch }) } catch (e) { /* best-effort */ }
}

function startFlushTimer() {
  stopFlushTimer()
  flushTimer = setInterval(flush, FLUSH_INTERVAL)
}

function stopFlushTimer() {
  if (flushTimer) { clearInterval(flushTimer); flushTimer = null }
}

// sendBeacon on page close
if (typeof window !== 'undefined') {
  window.addEventListener('beforeunload', () => {
    if (eventQueue.length) {
      try {
        const blob = new Blob([JSON.stringify({ type: 'analytics_batch', data: { events: eventQueue } })], { type: 'application/json' })
        navigator.sendBeacon('/api/v1/analytics', blob)
      } catch (e) { /* ignore */ }
    }
  })
}
