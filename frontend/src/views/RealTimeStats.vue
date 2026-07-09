<template>
  <div class="stats-page">
    <div class="topbar">
      <div class="topbar-left">
        <h1 class="topbar-title">📊 实时对战数据</h1>
        <span class="topbar-badge" :class="polling ? 'live' : 'off'"><span class="badge-dot" /> {{ polling ? 'LIVE' : '离线' }}</span>
        <span class="topbar-time">更新于 {{ lastUpdated }}</span>
      </div>
      <div class="topbar-right">
        <span class="topbar-metric">⚔️ {{ animatedBattles.toLocaleString() }} 场</span>
        <span class="topbar-metric">🐾 {{ summary.species || 0 }} 种</span>
      </div>
    </div>
    <div class="nav-row">
      <button v-for="tab in tabs" :key="tab.key" @click="activeTab = tab.key" class="nav-btn" :class="{ 'nav-active': activeTab === tab.key }">{{ tab.label }}</button>
    </div>

    <div v-if="activeTab === 'species'" class="tab-body">
      <DataTable title="🐾 宝可梦使用率排行" :columns="speciesCols" :rows="speciesRows" :loading="loading" id-key="species_id" defaultSort="appearance_count" :stableHeight="true" :stableHeightPx="600" search-placeholder="搜索精灵..." />
    </div>
    <div v-if="activeTab === 'moves'" class="tab-body">
      <DataTable title="⚡ 招式使用排行" :columns="moveCols" :rows="moveRows" :loading="loading" id-key="move_id" defaultSort="usage_pct" :stableHeight="true" :stableHeightPx="600" search-placeholder="搜索招式..." />
    </div>
    <div v-if="activeTab === 'items'" class="tab-body">
      <DataTable title="🎒 道具携带排行" :columns="itemCols" :rows="itemRows" :loading="loading" id-key="item_id" defaultSort="usage_pct" :stableHeight="true" :stableHeightPx="600" search-placeholder="搜索道具..." />
    </div>
    <div v-if="activeTab === 'abilities'" class="tab-body">
      <DataTable title="🌟 特性使用排行" :columns="abilityCols" :rows="abilityRows" :loading="loading" id-key="ability_id" defaultSort="usage_pct" :stableHeight="true" :stableHeightPx="600" search-placeholder="搜索特性..." />
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted } from 'vue'
import { statsAPI } from '../api/stats'
import { useNameMapper } from '../utils/nameMapper'
import DataTable from '../components/charts/DataTable.vue'

const { speciesCN, moveCN, itemCN, abilityCN, moveTypes, load: loadNames } = useNameMapper()
const POLL = 30

const loading = ref(true), polling = ref(false), activeTab = ref('species'), lastUpdated = ref('--')
const summary = ref({}), speciesRows = ref([]), moveRows = ref([]), itemRows = ref([]), abilityRows = ref([])
const animatedBattles = ref(0)

function animateNumber(r, t) {
  if (t === r.value) return
  const s = r.value, d = t - s, st = performance.now()
  function tick(n) { const p = Math.min((n - st) / 500, 1); r.value = Math.round(s + d * (1 - Math.pow(1 - p, 3))); if (p < 1) requestAnimationFrame(tick) }
  requestAnimationFrame(tick)
}
function iconCell(sid) {
  const ICON = 32, COLS = 16, n = (sid || 0) - 1
  if (n < 0) return ''
  const c = n % COLS, r = Math.floor(n / COLS), bw = COLS * ICON
  return `<div style="width:${ICON}px;height:${ICON}px;background-image:url(/icons-sheet.png);background-position:-${c*ICON}px -${r*ICON}px;background-size:${bw}px auto;background-repeat:no-repeat;image-rendering:pixelated;border-radius:3px"></div>`
}
function enrich(list, lookup) { return (list || []).map(r => ({ ...r, _cn: lookup(r.name) || r.species_name || r.move_name || r.item_name || r.ability_name || '' })) }

const tabs = [
  { key: 'species', label: '🐾 宝可梦' }, { key: 'moves', label: '⚡ 招式' },
  { key: 'items', label: '🎒 道具' }, { key: 'abilities', label: '🌟 特性' },
]

const speciesCols = [
  { key: 'species_id', label: '', sortable: false, format: (v, row) => iconCell(parseInt(row.species_id) || parseInt(row.name) || 0), class: 'dt-col-icon' },
  { key: '_cn', label: '精灵', sortable: true },
  { key: 'appearance_count', label: '出场', sortable: true, spark: true, class: 'text-right' },
]
const moveCols = [
  { key: 'move_id', label: '', sortable: false, format: (v, row) => { const t = moveTypes.value[parseInt(row.name)||0]; return t ? '<span class="type-label type-' + t.toLowerCase() + '">' + t + '</span>' : '' }, class: 'dt-col-type' },
  { key: '_cn', label: '招式', sortable: true },
  { key: 'usage_pct', label: '使用率%', sortable: true, spark: true, class: 'text-right', format: v => (v||0).toFixed(1) }
]
const itemCols = [
  { key: 'item_id', label: '', sortable: false, format: (v, row) => iconCell(parseInt(row.item_id) || parseInt(row.name) || 0), class: 'dt-col-icon' },
  { key: '_cn', label: '道具', sortable: true },
  { key: 'usage_pct', label: '使用率%', sortable: true, spark: true, class: 'text-right', format: v => (v||0).toFixed(1) }
]
const abilityCols = [{ key: '_cn', label: '特性', sortable: true }, { key: 'usage_pct', label: '使用率%', sortable: true, spark: true, class: 'text-right', format: v => (v||0).toFixed(1) }]

let _poll = null, _seq = 0
async function pollData() {
  const seq = ++_seq; polling.value = true
  try {
    const res = await statsAPI.snapshot().catch(() => null)
    if (seq !== _seq || !res) return
    const d = (res.data || res) || {}
    summary.value = d.summary || {}
    animateNumber(animatedBattles, d.summary?.battles || 0)
    speciesRows.value = enrich(d.species || [], speciesCN).sort((a, b) => (b.appearance_count || 0) - (a.appearance_count || 0))
    moveRows.value = enrich(d.moves || [], moveCN)
    itemRows.value = enrich(d.items || [], itemCN)
    abilityRows.value = enrich(d.abilities || [], abilityCN)
    lastUpdated.value = new Date().toLocaleTimeString()
  } catch {} finally { polling.value = false; loading.value = false }
}

onMounted(async () => { await loadNames(); await pollData(); _poll = setInterval(pollData, POLL * 1000) })
onUnmounted(() => { if (_poll) clearInterval(_poll) })
</script>

<style scoped>
.stats-page { max-width: 1200px; margin: 0 auto; padding: 16px 16px 40px; }
.topbar { display: flex; align-items: center; justify-content: space-between; flex-wrap: wrap; gap: 10px; padding: 12px 18px; margin-bottom: 14px; background: rgba(255,255,255,0.75); backdrop-filter: blur(12px); border: 1px solid rgba(255,255,255,0.5); border-radius: 14px; }
.topbar-left { display: flex; align-items: center; gap: 10px; }
.topbar-title { font-size: 1.05rem; font-weight: 700; color: #1e293b; margin: 0; }
.topbar-badge { display: inline-flex; align-items: center; gap: 5px; font-size: 0.58rem; font-weight: 700; letter-spacing: 0.08em; text-transform: uppercase; padding: 3px 10px; border-radius: 99px; }
.topbar-badge.live { background: #ecfdf5; color: #059669; }
.badge-dot { width: 6px; height: 6px; border-radius: 50%; }
.live .badge-dot { background: #10b981; animation: dot-pulse 1.5s ease-in-out infinite; }
@keyframes dot-pulse { 0%,100% { opacity: 1; } 50% { opacity: 0.3; } }
.topbar-time { font-size: 0.7rem; color: #94a3b8; }
.topbar-right { display: flex; align-items: center; gap: 10px; }
.topbar-metric { font-size: 0.76rem; font-weight: 600; color: #475569; }
.nav-row { display: flex; gap: 6px; margin-bottom: 14px; }
.nav-btn { padding: 9px 22px; border-radius: 12px; font-size: 0.84rem; font-weight: 600; border: none; background: rgba(255,255,255,0.65); color: #64748b; cursor: pointer; transition: all 0.2s; }
.nav-btn:hover { background: rgba(255,255,255,0.9); color: #334155; }
.nav-active { background: #fff; color: #1e293b; box-shadow: 0 1px 6px rgba(0,0,0,0.1); }
.type-label { display:inline-block; padding:1px 6px; border-radius:4px; font-size:0.65rem; font-weight:600; color:#fff; min-width:36px; text-align:center; }
.type-normal{background:#A8A878} .type-fire{background:#F08030} .type-water{background:#6890F0}
.type-grass{background:#78C850} .type-electric{background:#F8D030} .type-ice{background:#98D8D8}
.type-fighting{background:#C03028} .type-poison{background:#A040A0} .type-ground{background:#E0C068}
.type-flying{background:#A890F0} .type-psychic{background:#F85888} .type-bug{background:#A8B820}
.type-rock{background:#B8A038} .type-ghost{background:#705898} .type-dark{background:#705848}
.type-dragon{background:#7038F8} .type-steel{background:#B8B8D0} .type-fairy{background:#F0B6BC}
.tab-body { display: flex; flex-direction: column; }
</style>
