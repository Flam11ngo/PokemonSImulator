/**
 * WebSocket client for PokemonSimulator.
 * Singleton connection with message routing via type-based callbacks.
 */
const WS_URL = `ws://${location.hostname}:${location.port}/ws`

let ws = null
let playerId = null
let reconnectTimer = null
let pingTimer = null
let missedState = null
const handlers = new Map()
const pending = new Map()
const sendQueue = []  // queue messages during disconnect
let msgId = 0

function startHeartbeat() {
  stopHeartbeat()
  pingTimer = setInterval(() => {
    if (ws && ws.readyState === WebSocket.OPEN) {
      sendRaw({ type: 'ping', data: {} })
    }
  }, 20000) // every 20s, before 30s timeout
}

function stopHeartbeat() {
  if (pingTimer) { clearInterval(pingTimer); pingTimer = null }
}

export function getPlayerId() { return playerId }

export function connect(playerName = 'Trainer') {
  const expectedId = localStorage.getItem('trainer_name') || playerName.replace(/\s+/g,'_')
  // If WS open but playerId doesn't match expected, reconnect
  if (ws && ws.readyState === WebSocket.OPEN) {
    if (playerId === expectedId) return Promise.resolve(playerId)
    // Name changed - close old and reconnect
    ws.close()
    ws = null
  }

  // If already connecting, wait for it instead of creating a duplicate
  if (ws && ws.readyState === WebSocket.CONNECTING) {
    return new Promise((resolve, reject) => {
      const check = setInterval(() => {
        if (ws.readyState === WebSocket.OPEN && playerId) {
          clearInterval(check); resolve(playerId)
        }
      }, 100)
      setTimeout(() => { clearInterval(check); reject(new Error('connect timeout')) }, 10000)
    })
  }

  return new Promise((resolve, reject) => {
    ws = new WebSocket(WS_URL)

    ws.onopen = () => {
      startHeartbeat()
      playerId = localStorage.getItem('trainer_name') || playerName.replace(/\s+/g,'_')
      sendRaw({ type: 'handshake', data: { player_id: playerId } })
      flushQueue()
      if (missedState) {
        console.log('[WS] replaying buffered state from reconnect')
        const st = missedState; missedState = null
        if (handlers.has('turn_processed')) {
          handlers.get('turn_processed').forEach(fn => fn(st))
        }
      }
    }

    ws.onmessage = (event) => {
      try {
        const msg = JSON.parse(event.data)
        const { type, data, id } = msg

        // Resolve pending request
        if (id && pending.has(id)) {
          pending.get(id)(data)
          pending.delete(id)
          return
        }

        // Dispatch to type handlers
        if (handlers.has(type)) {
          handlers.get(type).forEach(fn => fn(data))
        }
        // Also dispatch to wildcard
        if (handlers.has('*')) {
          handlers.get('*').forEach(fn => fn(type, data))
        }
      } catch (e) {
        console.error('[WS] parse error:', e)
      }
    }

    ws.onclose = () => {
      console.log('[WS] disconnected, reconnecting in 1s...');
      stopHeartbeat()
      reconnectTimer = setTimeout(() => connect(playerName), 1000)
    }

    ws.onerror = (e) => {
      console.error('[WS] error:', e)
      reject(e)
    }

    // Wait for handshake
    const check = setInterval(() => {
      if (playerId) {
        clearInterval(check)
        resolve(playerId)
      }
    }, 100)
    setTimeout(() => { clearInterval(check); reject(new Error('handshake timeout')) }, 5000)
  })
}

function sendRaw(msg) {
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify(msg))
  } else {
    // Queue for reconnect (max 3 to avoid flooding)
    if (sendQueue.length < 3) sendQueue.push(msg)
  }
}

function flushQueue() {
  while (sendQueue.length && ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify(sendQueue.shift()))
  }
}

/** Send a message and get a Promise for the response (request/response pattern) */
export function request(type, data = {}) {
  const id = String(++msgId)
  return new Promise((resolve, reject) => {
    pending.set(id, resolve)
    sendRaw({ type, data, id })
    setTimeout(() => {
      if (pending.has(id)) {
        pending.delete(id)
        reject(new Error(`Request timeout: ${type}`))
      }
    }, 15000)
  })
}

/** Send a fire-and-forget message */
export function send(type, data = {}) {
  sendRaw({ type, data })
}

/** Subscribe to a message type */
export function on(type, callback) {
  if (!handlers.has(type)) handlers.set(type, new Set())
  handlers.get(type).add(callback)
  return () => handlers.get(type)?.delete(callback)  // return unsubscribe
}

/** One-time subscription */
export function once(type, callback) {
  const unsub = on(type, (data) => {
    callback(data)
    unsub()
  })
}

export function disconnect() {
  clearTimeout(reconnectTimer)
  stopHeartbeat()
  if (ws) ws.close()
  ws = null
  playerId = null
}
