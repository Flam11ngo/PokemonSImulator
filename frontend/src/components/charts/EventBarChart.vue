<template>
  <div class="glass-card p-5">
    <h3 class="text-base font-bold text-gray-800 mb-4">📋 对战事件分布</h3>
    <div v-if="loading" class="space-y-2 py-4">
      <div v-for="i in 6" :key="i" class="h-8 rounded-lg bg-gradient-to-r from-gray-100 via-gray-50 to-gray-100 bg-[length:200%_100%] animate-shimmer" />
    </div>
    <Bar v-else-if="chartReady" :data="chartData" :options="chartOptions" />
    <div v-else class="text-center text-gray-400 py-16">
      <div class="text-4xl mb-3">📭</div>
      <p class="text-sm">暂无数据</p>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { Bar } from 'vue-chartjs'
import { Chart as ChartJS, CategoryScale, LinearScale, BarElement, Title, Tooltip, Legend } from 'chart.js'
import { statsAPI } from '../../api/stats'

ChartJS.register(CategoryScale, LinearScale, BarElement, Title, Tooltip, Legend)

const props = defineProps({ loading: { type: Boolean, default: false }, battleId: { type: String, default: 'all' } })

const EVENT_LABELS = {
  'switch_in': '上场', 'switch_out': '下场', 'move_used': '使用招式',
  'damage': '伤害', 'heal': '恢复', 'faint': '濒死',
  'ability_trigger': '特性触发', 'item_trigger': '道具触发',
  'status_apply': '状态附加', 'stat_change': '能力变化',
  'weather': '天气', 'other': '其他',
}

const EVENT_COLORS = [
  'rgba(99, 102, 241, 0.85)', 'rgba(236, 72, 153, 0.85)',
  'rgba(34, 197, 94, 0.85)', 'rgba(245, 158, 11, 0.85)',
  'rgba(139, 92, 246, 0.85)', 'rgba(6, 182, 212, 0.85)',
  'rgba(239, 68, 68, 0.85)',
]

const raw = ref([])
const chartReady = computed(() => !props.loading && raw.value.length > 0)

onMounted(async () => {
  try {
    const result = await statsAPI.deepEvents()
    raw.value = (result.data || result || [])
  } catch { /* no data */ }
})

const chartData = computed(() => ({
  labels: raw.value.map(r => EVENT_LABELS[r.event_type] || r.event_type),
  datasets: [{
    label: '次数',
    data: raw.value.map(r => r.count),
    backgroundColor: raw.value.map((_, i) => EVENT_COLORS[i % EVENT_COLORS.length]),
    borderWidth: 0,
    borderRadius: 4,
  }]
}))

const chartOptions = {
  responsive: true,
  maintainAspectRatio: false,
  animation: { duration: 800, easing: 'easeOutQuart' },
  plugins: {
    legend: { display: false },
    tooltip: { backgroundColor: '#1e293b', cornerRadius: 8, padding: 10 },
  },
  scales: {
    x: { grid: { display: false }, ticks: { color: '#64748b' } },
    y: { beginAtZero: true, ticks: { stepSize: 1, color: '#94a3b8' }, grid: { color: '#f1f5f9' } },
  },
}
</script>

<style scoped>
canvas { max-height: 280px; }
@keyframes shimmer { 0% { background-position: 200% 0; } 100% { background-position: -200% 0; } }
.animate-shimmer { animation: shimmer 1.5s infinite; }
</style>
