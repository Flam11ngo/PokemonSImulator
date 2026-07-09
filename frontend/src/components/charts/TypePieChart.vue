<template>
  <div class="glass-card p-5">
    <h3 class="text-base font-bold text-gray-800 mb-4">🔥 属性分布</h3>
    <div v-if="loading" class="flex items-center justify-center py-8">
      <div class="w-48 h-48 rounded-full bg-gradient-to-r from-gray-100 via-gray-50 to-gray-100 bg-[length:200%_100%] animate-shimmer" />
    </div>
    <Doughnut v-else-if="chartReady" :data="chartData" :options="chartOptions" />
    <div v-else class="text-center text-gray-400 py-16">
      <div class="text-4xl mb-3">📭</div>
      <p class="text-sm">暂无数据</p>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { Doughnut } from 'vue-chartjs'
import { Chart as ChartJS, ArcElement, Tooltip, Legend } from 'chart.js'
import { statsAPI } from '../../api/stats'

ChartJS.register(ArcElement, Tooltip, Legend)

const props = defineProps({ loading: { type: Boolean, default: false } })

const TYPE_COLORS = {
  'Normal': '#A8A878', 'Fire': '#F08030', 'Water': '#6890F0', 'Electric': '#F8D030',
  'Grass': '#78C850', 'Ice': '#98D8D8', 'Fighting': '#C03028', 'Poison': '#A040A0',
  'Ground': '#E0C068', 'Flying': '#A890F0', 'Psychic': '#F85888', 'Bug': '#A8B820',
  'Rock': '#B8A038', 'Ghost': '#705898', 'Dragon': '#7038F8', 'Dark': '#705848',
  'Steel': '#B8B8D0', 'Fairy': '#EE99AC',
}

const raw = ref([])
const chartReady = computed(() => !props.loading && raw.value.length > 0)

onMounted(async () => {
  try {
    const result = await statsAPI.deepTypes()
    raw.value = (result.data || result || []).filter(t => t.appearances > 0)
  } catch { /* no data */ }
})

const chartData = computed(() => ({
  labels: raw.value.map(r => r.type_name || r.type_id),
  datasets: [{
    data: raw.value.map(r => r.appearances),
    backgroundColor: raw.value.map(r => {
      const key = Object.keys(TYPE_COLORS).find(
        k => k.toLowerCase() === (r.type_id || '').toLowerCase()
      )
      return TYPE_COLORS[key] || '#e2e8f0'
    }),
    borderWidth: 3,
    borderColor: '#fff',
    hoverBorderWidth: 4,
  }]
}))

const chartOptions = {
  responsive: true,
  maintainAspectRatio: false,
  animation: { animateRotate: true, duration: 1000, easing: 'easeOutQuart' },
  plugins: {
    legend: { position: 'right', labels: { boxWidth: 10, padding: 12, font: { size: 11 }, usePointStyle: true, pointStyleWidth: 8 } },
    tooltip: { backgroundColor: '#1e293b', cornerRadius: 8, padding: 10 },
  },
}
</script>

<style scoped>
canvas { max-height: 300px; }
@keyframes shimmer { 0% { background-position: 200% 0; } 100% { background-position: -200% 0; } }
.animate-shimmer { animation: shimmer 1.5s infinite; }
</style>
