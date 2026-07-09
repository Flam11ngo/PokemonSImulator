/**
 * Frontend data instrumentation — sends real user events to Kafka via API.
 * Follows kafka-topics.md format for player.ui.events.
 */
import { ref } from 'vue'

const playerId = ref(localStorage.getItem('trainer_name') || 'anonymous')
const sessionId = ref(`sess_${playerId.value}_${Date.now().toString(36)}`)

// Refresh player ID periodically (user might log in/out)
setInterval(() => {
  const n = localStorage.getItem('trainer_name') || 'anonymous'
  if (n !== playerId.value) {
    playerId.value = n
    sessionId.value = `sess_${n}_${Date.now().toString(36)}`
  }
}, 3000)

const buffer = []
let flushTimer = null

function sendBeacon(events) {
  try {
    const body = JSON.stringify({ events })
    if (navigator.sendBeacon) {
      navigator.sendBeacon('/api/v1/analytics', body)
    } else {
      fetch('/api/v1/analytics', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body,
      }).catch(() => {})
    }
  } catch {}
}

function flush() {
  if (!buffer.length) return
  sendBeacon([...buffer])
  buffer.length = 0
}

function enqueue(eventType, data) {
  buffer.push({
    event: eventType,
    player_id: playerId.value,
    data: { ...data, player_id: playerId.value },
    session_id: sessionId.value,
    timestamp: new Date().toISOString(),
  })
  if (buffer.length >= 10) flush()
  // Also set a timer for partial flushes
  if (!flushTimer) {
    flushTimer = setTimeout(() => { flush(); flushTimer = null }, 5000)
  }
}

// ── Public API ──

// Pages we don't track (dev/login/test)
const SKIP_PAGES = ['/test', '/login', '/realtime']

export function trackPageView(page) {
  if (SKIP_PAGES.includes(page)) return
  enqueue('page_view', { page })
}

export function trackClick(element) {
  enqueue('ui_click', { element })
}

export function trackState(from, to) {
  enqueue('player_state', { from, to })
}

export function trackMatchmaking(opponentType) {
  enqueue('matchmaking_join', { opponent_type: opponentType })
}

export function trackTeamSave(teamName, pokemonCount) {
  enqueue('team_save', { team_name: teamName, pokemon_count: pokemonCount })
}

export function trackSessionStart() {
  enqueue('session_start', { player_id: playerId.value })
}

// Flush on page unload
if (typeof window !== 'undefined') {
  window.addEventListener('beforeunload', () => sendBeacon(buffer.length ? [...buffer] : []))
}
