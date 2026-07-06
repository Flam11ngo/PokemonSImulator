<template>
  <div>
    <h1 class="text-2xl font-bold text-gray-800 mb-4">📊 大数据分析看板</h1>
    <p class="text-xs text-gray-400 mb-6">Smogon Gen 9 1v1 · {{ timeBucketLabel }} · Rating {{ rating }}</p>

    <!-- Filter Bar -->
    <div class="flex gap-3 mb-6 flex-wrap items-center">
      <select v-model="selTimeBucket" @change="refreshAll"
        class="bg-white border border-gray-200 rounded-xl px-3 py-2 text-sm text-gray-600 focus:outline-none focus:ring-2 focus:ring-rose-400/30">
        <option value="">选择月份</option>
        <option v-for="tb in timeBuckets" :key="tb" :value="tb">{{ tb }}</option>
      </select>
      <select v-model="selRating" @change="refreshAll"
        class="bg-white border border-gray-200 rounded-xl px-3 py-2 text-sm text-gray-600 focus:outline-none focus:ring-2 focus:ring-rose-400/30">
        <option :value="null">选择分级</option>
        <option v-for="r in ratings" :key="r" :value="r">Rating {{ r }}</option>
      </select>
      <input v-model="searchQuery" @input="onSearch" placeholder="搜索宝可梦 (中/英文)..."
        class="flex-1 min-w-[180px] bg-white border border-gray-200 rounded-xl px-3 py-2 text-sm text-gray-700 focus:outline-none focus:ring-2 focus:ring-rose-400/30" />
      <button @click="refreshAll"
        class="px-4 py-2 bg-rose-400 hover:bg-rose-500 text-white rounded-xl text-xs font-bold transition-colors">
        🔄 刷新
      </button>
    </div>

    <!-- KPI Cards -->
    <div class="grid grid-cols-2 sm:grid-cols-4 gap-4 mb-6">
      <div class="bg-white rounded-xl border border-gray-200 p-4 text-center shadow-sm">
        <div class="text-2xl font-bold text-gray-800">{{ summary.total || '-' }}</div>
        <div class="text-xs text-gray-400 mt-1">总物种数</div>
      </div>
      <div class="bg-white rounded-xl border border-gray-200 p-4 text-center shadow-sm">
        <div class="text-2xl font-bold text-rose-500">{{ summary.avg_usage_pct || '-' }}%</div>
        <div class="text-xs text-gray-400 mt-1">平均使用率</div>
      </div>
      <div class="bg-white rounded-xl border border-gray-200 p-4 text-center shadow-sm">
        <div class="text-sm font-bold text-amber-600 truncate">{{ summary.top_item || '-' }}</div>
        <div class="text-xs text-gray-400 mt-1">最热门道具</div>
      </div>
      <div class="bg-white rounded-xl border border-gray-200 p-4 text-center shadow-sm">
        <div class="text-sm font-bold text-blue-600 truncate">{{ summary.top_ability || '-' }}</div>
        <div class="text-xs text-gray-400 mt-1">最热门特性</div>
      </div>
    </div>

    <!-- Main Content -->
    <div class="grid grid-cols-1 lg:grid-cols-3 gap-6">
      <!-- Ranking Table (2/3 width) -->
      <div class="lg:col-span-2">
        <PokemonRankTable ref="rankTable"
          :items="ranking"
          :loading="loadingRank"
          @select="openDetail" />
      </div>

      <!-- Sidebar: charts (1/3 width) -->
      <div class="space-y-4">
        <!-- Top Moves mini list -->
        <div class="bg-white rounded-xl border border-gray-200 shadow-sm p-4">
          <h4 class="text-xs font-bold text-gray-500 uppercase mb-3">全局热门招式</h4>
          <div class="space-y-2">
            <div v-for="(m, i) in topMoves.slice(0, 10)" :key="m.name"
              class="flex items-center gap-2 text-xs">
              <span class="text-gray-400 w-5 text-right font-mono shrink-0">{{ i + 1 }}</span>
              <span class="text-gray-600 flex-1 truncate">{{ m.name }}</span>
              <div class="w-16 h-1.5 bg-gray-100 rounded-full overflow-hidden shrink-0">
                <div class="h-full bg-rose-400 rounded-full" :style="{ width: moveBarPct(m.mon_count) + '%' }"></div>
              </div>
              <span class="text-gray-400 font-mono w-8 text-right shrink-0">{{ m.mon_count }}</span>
            </div>
            <div v-if="!topMoves.length" class="text-center py-4 text-gray-400 text-xs">加载中...</div>
          </div>
        </div>

        <!-- Top Items -->
        <div class="bg-white rounded-xl border border-gray-200 shadow-sm p-4">
          <h4 class="text-xs font-bold text-gray-500 uppercase mb-3">全局热门道具</h4>
          <div class="space-y-2">
            <div v-for="(it, i) in topItems.slice(0, 8)" :key="it.name"
              class="flex items-center gap-2 text-xs">
              <span class="text-gray-400 w-5 text-right font-mono shrink-0">{{ i + 1 }}</span>
              <span class="text-amber-700 flex-1 truncate">{{ it.name }}</span>
              <div class="w-16 h-1.5 bg-gray-100 rounded-full overflow-hidden shrink-0">
                <div class="h-full bg-amber-400 rounded-full" :style="{ width: itemBarPct(it.mon_count) + '%' }"></div>
              </div>
              <span class="text-gray-400 font-mono w-8 text-right shrink-0">{{ it.mon_count }}</span>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- Detail Modal -->
    <PokemonDetailModal
      :pokemon-name="selectedName"
      :visible="showDetail"
      :source="'smogon'"
      :time-bucket="selTimeBucket"
      :rating="selRating || 1760"
      @close="showDetail = false"
      @select-teammate="openDetail" />
  </div>
</template>

<script setup>
import { ref, reactive, computed, onMounted, watch } from 'vue'
import { smogonAPI } from '../api/smogon'
import { request } from '../api/wsClient'
import PokemonRankTable from '../components/stats/PokemonRankTable.vue'
import PokemonDetailModal from '../components/stats/PokemonDetailModal.vue'

// Filters
const timeBuckets = ref([])
const ratings = ref([0, 1500, 1630, 1760])
const selTimeBucket = ref('')
const selRating = ref(1760)
const searchQuery = ref('')
const rankTable = ref(null)  // template ref for PokemonRankTable

// Data
const ranking = ref([])
const loadingRank = ref(false)
const summary = reactive({})
const topMoves = ref([])
const topItems = ref([])

// Detail
const selectedName = ref('')
const showDetail = ref(false)

const timeBucketLabel = computed(() => {
  if (!selTimeBucket.value) return '最新数据'
  const m = selTimeBucket.value.match(/^(\d{4})-(\d{2})$/)
  if (m) return `${m[1]}年${m[2]}月`
  return selTimeBucket.value
})
const rating = computed(() => selRating.value || 1760)

let searchTimer = null

async function loadFilters() {
  try {
    const f = await smogonAPI.filters()
    timeBuckets.value = f.time_buckets || []
    if (!selTimeBucket.value && timeBuckets.value.length) {
      selTimeBucket.value = timeBuckets.value[0]
    }
  } catch (e) {
    console.error('Failed to load filters:', e)
  }
}

async function refreshAll() {
  if (!selTimeBucket.value) return
  const params = {
    source: 'smogon',
    time_bucket: selTimeBucket.value,
    rating: selRating.value || 1760,
    limit: 100,
  }
  if (searchQuery.value) params.search = searchQuery.value

  loadingRank.value = true
  try {
    ranking.value = await smogonAPI.ranking(params)
  } catch (e) {
    ranking.value = []
  } finally {
    loadingRank.value = false
  }

  // Load sidebar data in parallel
  smogonAPI.summary(params).then(s => Object.assign(summary, s)).catch(() => {})
  smogonAPI.moves({ ...params, limit: 20 }).then(d => topMoves.value = (d || []).filter(m => m.name && m.name.trim())).catch(() => {})
  smogonAPI.items({ ...params, limit: 10 }).then(d => topItems.value = (d.items || []).filter(i => i.name && i.name.trim())).catch(() => {})
}

function onSearch() {
  clearTimeout(searchTimer)
  searchTimer = setTimeout(refreshAll, 400)
}

function openDetail(name) {
  // name can be a string or the item object with .name
  selectedName.value = typeof name === 'string' ? name : (name?.name || name?.mate || '')
  showDetail.value = true
}

const maxMoveCount = computed(() => topMoves.value.length ? topMoves.value[0].mon_count || 1 : 1)
const maxItemCount = computed(() => topItems.value.length ? topItems.value[0].mon_count || 1 : 1)
function moveBarPct(v) { return Math.max(3, Math.round((v || 0) / maxMoveCount.value * 100)) }
function itemBarPct(v) { return Math.max(3, Math.round((v || 0) / maxItemCount.value * 100)) }

onMounted(async () => {
  await loadFilters()
  if (selTimeBucket.value) refreshAll()
  // Load species name→ID map for sprite icons (same pattern as TeamBuilder)
  console.log('[StatsDashboard] loading species map via WebSocket...')
  try {
    const list = await request('get_species', { search: '', limit: 1100 })
    console.log(`[StatsDashboard] received ${(list||[]).length} species, sample:`, (list||[]).slice(0,3).map(s=>`${s.name}=${s.id}`))
    const map = {}
    ;(list || []).forEach(s => { map[s.name.toLowerCase()] = s.id })
    console.log(`[StatsDashboard] built map with ${Object.keys(map).length} entries`)
    if (rankTable.value) {
      rankTable.value.setSpeciesMap(map)
      console.log('[StatsDashboard] setSpeciesMap called successfully')
    } else {
      console.warn('[StatsDashboard] rankTable ref is null — component not mounted yet')
    }
  } catch (e) {
    console.warn('[StatsDashboard] Failed to load species map:', e)
  }
})
</script>
