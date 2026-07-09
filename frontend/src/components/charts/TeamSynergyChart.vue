<template>
  <div class="glass-card p-5">
    <div class="flex items-center justify-between mb-4">
      <h3 class="text-base font-bold text-gray-800">🤝 同队搭配分析</h3>
      <input v-model="search" @input="applyFilter" placeholder="搜索精灵..."
        class="text-xs px-3 py-1.5 rounded-full bg-gray-50 border border-gray-200 focus:border-blue-400 focus:outline-none focus:ring-1 focus:ring-blue-300 w-36 transition-all" />
    </div>
    <div v-if="loading" class="space-y-1.5 py-2">
      <div v-for="i in 8" :key="i" class="h-6 rounded bg-gradient-to-r from-gray-100 via-gray-200 to-gray-100 bg-[length:200%_100%] animate-shimmer" />
    </div>
    <div v-else-if="!hasSearch" class="text-center text-gray-400 py-16">
      <div class="text-4xl mb-3">🔍</div>
      <p class="text-sm">搜索精灵名称查看搭配</p>
    </div>
    <div v-else-if="filtered.length === 0" class="text-center text-gray-400 py-16">
      <div class="text-4xl mb-3">📭</div>
      <p class="text-sm">该精灵暂无搭配数据</p>
    </div>
    <div v-else ref="chartContainer" class="relative" style="height: 400px">
      <canvas ref="canvasRef"></canvas>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted, watch, nextTick } from 'vue'
import { Chart, BarController, CategoryScale, LinearScale, BarElement, Title, Tooltip } from 'chart.js'
import { statsAPI } from '../../api/stats'

Chart.register(BarController, CategoryScale, LinearScale, BarElement, Title, Tooltip)

const props = defineProps({ loading: { type: Boolean, default: false } })
const allPairs = ref([])
const search = ref('')
const filtered = ref([])
const canvasRef = ref(null)
let chartInstance = null

const hasSearch = computed(() => search.value.trim().length > 0)

onMounted(async () => {
  try {
    const resp = await statsAPI.deepTeamSynergy()
    allPairs.value = (resp.data || resp || [])
  } catch {}
})

function applyFilter() {
  const q = search.value.toLowerCase().trim()
  if (!q) { filtered.value = []; return }
  filtered.value = allPairs.value
    .filter(p => (p.s1_name || '').toLowerCase().includes(q) || (p.s2_name || '').toLowerCase().includes(q))
    .slice(0, 15)
}

watch(filtered, async () => {
  await nextTick()
  if (chartInstance) { chartInstance.destroy(); chartInstance = null }
  if (!canvasRef.value || filtered.value.length === 0) return
  chartInstance = new Chart(canvasRef.value, {
    type: 'bar',
    data: {
      labels: filtered.value.map(p => `${p.s1_name} + ${p.s2_name}`),
      datasets: [{
        label: '配对次数',
        data: filtered.value.map(p => p.times),
        backgroundColor: 'rgba(99, 102, 241, 0.6)',
        borderRadius: 4,
      }],
    },
    options: {
      indexAxis: 'y',
      responsive: true,
      maintainAspectRatio: false,
      animation: false,
      plugins: { legend: { display: false } },
      scales: { x: { ticks: { font: { size: 11 } } }, y: { ticks: { font: { size: 10 } } } },
    },
  })
}, { deep: true })
</script>
