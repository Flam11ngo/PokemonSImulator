/**
 * Data query helpers using WebSocket request/response pattern.
 * Caches results for instant re-access.
 */
import { request, connect, getPlayerId } from './wsClient'

let _ready = false
async function ensure() {
  if (!_ready) {
    await connect('DataClient')
    _ready = true
  }
}

const STORE_KEY = 'pokemon_data_cache'

function loadStore() {
  try { return JSON.parse(localStorage.getItem(STORE_KEY)) || {} }
  catch { return {} }
}
function saveStore(s) {
  try { localStorage.setItem(STORE_KEY, JSON.stringify(s)) } catch {}
}

const cache = { species: {}, moves: {}, abilities: {}, items: {}, ...loadStore() }

// Persist to localStorage after every cache update (debounced)
let _saveTimer = null
function scheduleSave() {
  clearTimeout(_saveTimer)
  _saveTimer = setTimeout(() => saveStore(cache), 1000)
}

export async function searchSpecies(query = '') {
  await ensure()
  const data = await request('get_species', { search: query || '', limit: 15 })
  data.forEach(s => { cache.species[s.id] = s })
  scheduleSave()
  return data
}

export async function searchMoves(query = '', learnset = null) {
  await ensure()
  const params = { search: query || '', limit: 15 }
  if (learnset && learnset.length > 0) params.learnset = learnset
  const data = await request('get_moves', params)
  data.forEach(m => { cache.moves[m.id] = m })
  scheduleSave()
  return data
}

export async function getMove(id) {
  if (cache.moves[id]) return cache.moves[id]
  await ensure()
  const data = await request('get_move', { id })
  if (data) { cache.moves[id] = data; scheduleSave() }
  return data
}

export async function getMoveName(id) {
  const m = await getMove(id)
  return m?.name || `#${id}`
}

export async function getAbility(id) {
  if (!id) return { name: '无' }
  if (cache.abilities[id]) return cache.abilities[id]
  await ensure()
  const data = await request('get_ability', { id })
  if (data) { cache.abilities[id] = data; scheduleSave() }
  return data
}

export async function getAbilityName(id) {
  const a = await getAbility(id)
  return a?.name || `#${id}`
}

export async function searchAbilities(query = '') {
  await ensure()
  const data = await request('get_abilities', { search: query || '' })
  data.forEach(a => { cache.abilities[a.id] = a })
  return data
}

export async function searchItems(query = '') {
  await ensure()
  const data = await request('get_items', { search: query || '', limit: 15 })
  data.forEach(it => { cache.items[it.id] = it })
  scheduleSave()
  return data
}

// Sprite URL cache (fetched from server)
const spriteUrlCache = {}
export async function getSpriteUrl(speciesId) {
  if (!speciesId) return ''
  if (spriteUrlCache[speciesId]) return spriteUrlCache[speciesId]
  await ensure()
  try {
    const data = await request('get_sprite_url', { id: speciesId })
    spriteUrlCache[speciesId] = data.url || `/icons/${speciesId}.png`
  } catch {
    spriteUrlCache[speciesId] = `/icons/${speciesId}.png`
  }
  return spriteUrlCache[speciesId]
}
