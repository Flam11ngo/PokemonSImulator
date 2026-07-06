/**
 * Shared species data — loaded once at app startup, consumed by all pages.
 */
import { ref } from 'vue'
import { request } from '../api/wsClient'

const speciesList = ref([])
const speciesMap = ref({})
const loaded = ref(false)
const loading = ref(false)

export function useSpeciesStore() {
  async function load() {
    if (loaded.value || loading.value) return
    loading.value = true
    try {
      const list = await request('get_species', { search: '', limit: 1100 })
      speciesList.value = list || []
      const map = {}
      ;(list || []).forEach(s => {
        map[s.name.toLowerCase()] = s.id
        map[s.name.toLowerCase().replace(/[-'. \s]/g, '')] = s.id
      })
      speciesMap.value = map
      loaded.value = true
      console.log('[speciesStore] loaded', Object.keys(map).length, 'entries (WS)')
    } catch (e) {
      console.warn('[speciesStore] load failed, retrying in 1s:', e.message)
      loading.value = false
      setTimeout(load, 1000)
    }
  }

  return { speciesList, speciesMap, loaded, loading, load }
}
