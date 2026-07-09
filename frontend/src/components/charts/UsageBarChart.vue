<template>
  <div class="glass-card p-5">
    <h3 class="text-base font-bold text-gray-800 mb-4">🐾 宝可梦使用率 Top 15</h3>

    <!-- Loading skeleton -->
    <div v-if="loading" class="space-y-1.5 py-2">
      <div v-for="i in 10" :key="i" class="flex items-center gap-3">
        <div class="w-8 h-8 rounded-full bg-gradient-to-r from-gray-100 via-gray-200 to-gray-100 bg-[length:200%_100%] animate-shimmer shrink-0" />
        <div class="h-4 flex-1 rounded bg-gradient-to-r from-gray-100 via-gray-200 to-gray-100 bg-[length:200%_100%] animate-shimmer" />
      </div>
    </div>

    <!-- Empty -->
    <div v-else-if="!chartReady" class="text-center text-gray-400 py-16">
      <div class="text-4xl mb-3">📭</div>
      <p class="text-sm">暂无数据</p>
    </div>

    <!-- Custom bar chart -->
    <div v-else class="space-y-1">
      <div v-for="(row, i) in raw" :key="row.species_id"
        class="flex items-center gap-2.5 group hover:bg-blue-50/40 rounded-lg px-1 py-0.5 transition-colors cursor-pointer"
        :style="{ animationDelay: `${i * 40}ms` }"
        style="animation: fadeIn 0.4s ease-out both"
        @click="$emit('bar-click', row)">
        <!-- Rank -->
        <span class="w-5 text-right text-xs font-mono text-gray-400 shrink-0">{{ i + 1 }}</span>
        <!-- Sprite -->
        <img :src="row.sprite_url"
          class="w-8 h-8 rounded-full bg-gray-50 border border-gray-100 shrink-0 object-contain"
          style="image-rendering: pixelated"
          @error="$event.target.style.display='none'" />
        <!-- Name + Bar -->
        <div class="flex-1 min-w-0 flex items-center gap-2">
          <span class="text-sm font-medium text-gray-700 w-24 shrink-0 truncate">{{ row.species_name }}</span>
          <div class="flex-1 h-7 bg-gray-100 rounded-full overflow-visible relative">
            <div class="absolute inset-y-0 left-0 rounded-full bg-gradient-to-r from-indigo-400 to-blue-500 transition-all duration-700 ease-out flex items-center justify-end pr-1.5 overflow-visible"
              :style="{ width: barPct(row) + '%' }">
              <span class="text-xs text-white font-bold font-mono whitespace-nowrap">{{ row.appearances }}</span>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { statsAPI } from '../../api/stats'

const props = defineProps({ loading: { type: Boolean, default: false } })
const emit = defineEmits(['bar-click'])
const raw = ref([])
const chartReady = computed(() => !props.loading && raw.value.length > 0)

onMounted(async () => {
  try {
    const result = await statsAPI.deepMeta()
    const data = (result.data || result || [])
    // Sort: appearances desc, then species_name asc (A-Z) for ties
    data.sort((a, b) =>
      (b.appearances || 0) - (a.appearances || 0) ||
      String(a.species_name || '').localeCompare(String(b.species_name || ''))
    )
    raw.value = data.slice(0, 15)
  } catch { /* no data */ }
})

const barMax = computed(() => Math.max(...raw.value.map(r => r.appearances || 0), 1))

function barPct(row) {
  return Math.round((row.appearances / barMax.value) * 100)
}
</script>

<style scoped>
@keyframes fadeIn {
  from { opacity: 0; transform: translateX(-10px); }
  to { opacity: 1; transform: translateX(0); }
}
@keyframes shimmer { 0% { background-position: 200% 0; } 100% { background-position: -200% 0; } }
.animate-shimmer { animation: shimmer 1.5s infinite; }
</style>
