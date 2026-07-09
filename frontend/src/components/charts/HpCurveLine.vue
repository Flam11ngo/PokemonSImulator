<template>
  <div class="glass-card p-5">
    <h3 class="text-base font-bold text-gray-800 mb-4">
      📈 宝可梦 HP 变化曲线
      <span v-if="battleId !== 'all'" class="text-xs text-gray-400 font-normal ml-2">对战 #{{ battleId }}</span>
    </h3>
    <div v-if="loading" class="h-64 rounded-lg bg-gradient-to-r from-gray-100 via-gray-50 to-gray-100 bg-[length:200%_100%] animate-shimmer" />
    <div v-else-if="battleId === 'all'" class="text-center text-gray-400 py-16">
      <div class="text-4xl mb-3">🔍</div>
      <p class="text-sm">请选择具体对战查看 HP 变化曲线</p>
      <p class="text-xs text-gray-300 mt-1">在上方下拉菜单中选择一场对战</p>
    </div>
    <Line v-else-if="chartReady" :data="chartData" :options="chartOptions" />
    <div v-else class="text-center text-gray-400 py-16">
      <div class="text-4xl mb-3">📭</div>
      <p class="text-sm">暂无数据</p>
    </div>
    <!-- Legend for fainted marks -->
    <div v-if="chartReady && battleId !== 'all'" class="text-xs text-gray-400 mt-2 flex items-center gap-2">
      <span>💀 标记 = 濒死</span>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted, watch } from 'vue'
import { Line } from 'vue-chartjs'
import { Chart as ChartJS, CategoryScale, LinearScale, PointElement, LineElement, Title, Tooltip, Legend, Filler } from 'chart.js'
import { statsAPI } from '../../api/stats'

ChartJS.register(CategoryScale, LinearScale, PointElement, LineElement, Title, Tooltip, Legend, Filler)

const props = defineProps({ loading: { type: Boolean, default: false }, battleId: { type: String, default: 'all' } })
const battleId = computed(() => props.battleId)
const raw = ref([])
const chartReady = computed(() => !props.loading && raw.value.length > 0 && props.battleId !== 'all')

// Per-pokemon colors
const POKE_COLORS = [
  { bg: 'rgba(239, 68, 68, 0.12)', border: '#ef4444' },
  { bg: 'rgba(59, 130, 246, 0.12)', border: '#3b82f6' },
  { bg: 'rgba(34, 197, 94, 0.12)', border: '#22c55e' },
  { bg: 'rgba(245, 158, 11, 0.12)', border: '#f59e0b' },
  { bg: 'rgba(139, 92, 246, 0.12)', border: '#8b5cf6' },
  { bg: 'rgba(236, 72, 153, 0.12)', border: '#ec4899' },
  { bg: 'rgba(6, 182, 212, 0.12)', border: '#06b6d4' },
  { bg: 'rgba(168, 85, 247, 0.12)', border: '#a855f7' },
  { bg: 'rgba(34, 211, 238, 0.12)', border: '#22d3ee' },
  { bg: 'rgba(251, 146, 60, 0.12)', border: '#fb923c' },
]

async function loadData() {
  try {
    const result = await statsAPI.deepPokemonHp()
    raw.value = (result.data || result || [])
  } catch { raw.value = [] }
}

onMounted(loadData)
watch(() => props.battleId, loadData)

const chartData = computed(() => {
  const data = props.battleId === 'all' ? raw.value
    : raw.value.filter(r => String(r.battle_id) === props.battleId)

  // Group by unique pokemon (species + side + pokemon_index)
  const pokemonMap = new Map()
  for (const r of data) {
    const key = `${r.side_index}-${r.pokemon_index}`
    if (!pokemonMap.has(key)) {
      pokemonMap.set(key, {
        label: `${r.species_name || '#' + r.species_id} [${r.side_index === 0 ? 'A' : 'B'}${r.pokemon_index}]`,
        side: r.side_index,
        points: [],
      })
    }
    pokemonMap.get(key).points.push({ turn: r.turn, hp: r.hp_pct, fainted: r.fainted })
  }

  const pokemons = [...pokemonMap.values()]
  // Sort by side then pokemon_index
  pokemons.sort((a, b) => a.side - b.side)

  // Collect all turns
  const allTurns = [...new Set(data.map(r => r.turn))].sort((a, b) => a - b)

  return {
    labels: allTurns.map(t => `T${t}`),
    datasets: pokemons.map((p, i) => {
      const color = POKE_COLORS[i % POKE_COLORS.length]
      const points = p.points
      return {
        label: p.label,
        data: allTurns.map(t => {
          const pt = points.find(pp => pp.turn === t)
          return pt ? pt.hp : null
        }),
        borderColor: color.border,
        backgroundColor: color.bg,
        tension: 0.3, fill: false,
        pointRadius: 3, pointHoverRadius: 5,
        pointBackgroundColor: color.border,
        borderWidth: 2,
        spanGaps: false,
        // Mark fainted points with X-style
        pointStyle: allTurns.map(t => {
          const pt = points.find(pp => pp.turn === t)
          return pt && pt.fainted ? 'cross' : 'circle'
        }),
        pointBorderColor: allTurns.map(t => {
          const pt = points.find(pp => pp.turn === t)
          return pt && pt.fainted ? '#000' : color.border
        }),
      }
    }),
  }
})

const chartOptions = {
  responsive: true,
  maintainAspectRatio: false,
  animation: { duration: 800, easing: 'easeOutQuart' },
  interaction: { intersect: false, mode: 'index' },
  plugins: {
    legend: { position: 'bottom', labels: { usePointStyle: true, boxWidth: 8, padding: 12, font: { size: 10 } } },
    tooltip: {
      backgroundColor: '#1e293b', cornerRadius: 8, padding: 10,
      callbacks: {
        label: (ctx) => `${ctx.dataset.label}: ${ctx.parsed.y?.toFixed(1) || '?'}% HP`
      }
    },
  },
  scales: {
    y: { beginAtZero: true, max: 105, title: { display: true, text: 'HP %', color: '#94a3b8' }, grid: { color: '#f1f5f9' }, ticks: { color: '#94a3b8' } },
    x: { grid: { display: false }, ticks: { color: '#94a3b8', maxTicksLimit: 20 } },
  },
}
</script>

<style scoped>
canvas { max-height: 380px; }
@keyframes shimmer { 0% { background-position: 200% 0; } 100% { background-position: -200% 0; } }
.animate-shimmer { animation: shimmer 1.5s infinite; }
</style>
