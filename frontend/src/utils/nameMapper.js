/**
 * Shared name mapper — loads Chinese names once, provides fast lookups.
 * Used by RealTimeStats, DataTable, and any component that needs CN names.
 */
import { ref } from 'vue'
import { request } from '../api/wsClient'

const cnSpecies = ref({})   // speciesId (number) → chineseName
const cnMoves = ref({})     // moveId (number) → chineseName
const cnItems = ref({})     // itemId (number) → chineseName
const cnAbilities = ref({}) // abilityId (number) → chineseName
const moveTypes = ref({})   // moveId (number) → type string
const loaded = ref(false)
const loading = ref(false)

export function useNameMapper() {
  async function load() {
    if (loaded.value || loading.value) return
    loading.value = true
    try {
      // Load species with Chinese names via WebSocket
      const list = await request('get_species', { search: '', limit: 1100 })
      const spMap = {}
      ;(list || []).forEach(s => {
        spMap[s.id] = s.chineseName || s.name
      })
      cnSpecies.value = spMap

      // Load moves/items/abilities (smaller lists, load on demand via get_items/get_moves/get_abilities)
      try {
        const moves = await request('get_moves', { search: '', limit: 1000 })
        const mvMap = {}
        const mvTypeMap = {}
        ;(moves || []).forEach(m => { mvMap[m.id] = m.chineseName || m.name; mvTypeMap[m.id] = m.type || '' })
        cnMoves.value = mvMap
        moveTypes.value = mvTypeMap
      } catch {}

      try {
        const items = await request('get_items', { search: '', limit: 1000 })
        const itMap = {}
        ;(items || []).forEach(i => { itMap[i.id] = i.chineseName || i.name })
        cnItems.value = itMap
      } catch {}

      try {
        const abilities = await request('get_abilities', { search: '' })
        const abMap = {}
        ;(abilities || []).forEach(a => { abMap[a.id] = a.chineseName || a.name })
        cnAbilities.value = abMap
      } catch {}

      loaded.value = true
    } catch (e) {
      console.warn('[nameMapper] load failed:', e.message)
      loading.value = false
    }
  }

  /** Get Chinese name for a species by numeric ID */
  function speciesCN(id) {
    const nid = typeof id === 'string' ? parseInt(id) : id
    return cnSpecies.value[nid] || ''
  }

  /** Get Chinese name for a move by numeric ID */
  function moveCN(id) {
    const nid = typeof id === 'string' ? parseInt(id) : id
    return cnMoves.value[nid] || ''
  }

  function itemCN(id) {
    const nid = typeof id === 'string' ? parseInt(id) : id
    return cnItems.value[nid] || ''
  }

  function abilityCN(id) {
    const nid = typeof id === 'string' ? parseInt(id) : id
    return cnAbilities.value[nid] || ''
  }

  return { cnSpecies, cnMoves, cnItems, cnAbilities, moveTypes, loaded, loading, load, speciesCN, moveCN, itemCN, abilityCN }
}
