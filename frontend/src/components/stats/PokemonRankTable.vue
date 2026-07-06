<template>
  <div class="bg-white rounded-2xl border border-gray-200 shadow-sm overflow-hidden">
    <div class="px-4 py-3 border-b border-gray-100 flex items-center justify-between">
      <h3 class="text-sm font-bold text-gray-700">宝可梦排名</h3>
      <span class="text-xs text-gray-400">{{ items.length }} 只</span>
    </div>

    <div v-if="loading" class="flex items-center justify-center py-16">
      <div class="animate-spin w-6 h-6 border-2 border-rose-400 border-t-transparent rounded-full mr-2"></div>
      <span class="text-sm text-gray-400">加载中...</span>
    </div>

    <div v-else class="overflow-x-auto">
      <table class="w-full text-sm">
        <thead>
          <tr class="border-b border-gray-100 text-xs text-gray-400 uppercase tracking-wider">
            <th class="py-2 px-3 text-center w-10 cursor-pointer hover:text-gray-600" @click="toggleSort('rank')">#</th>
            <th class="py-2 px-3 text-left cursor-pointer hover:text-gray-600" @click="toggleSort('name')">宝可梦</th>
            <th class="py-2 px-3 text-right w-20 cursor-pointer hover:text-gray-600" @click="toggleSort('usage')">Pick率</th>
            <th class="py-2 px-3 text-right w-16 cursor-pointer hover:text-gray-600" @click="toggleSort('vc')">VC</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="(item, i) in sortedItems" :key="item.name"
            @click="$emit('select', item)"
            class="border-b border-gray-50 hover:bg-rose-50/60 cursor-pointer transition-colors">
            <td class="py-2 px-3 text-center">
              <span class="inline-flex items-center justify-center w-6 h-6 rounded-full text-xs font-bold"
                :class="rankClass(i + 1)">{{ i + 1 }}</span>
            </td>
            <td class="py-2 px-3">
              <div class="flex items-center gap-2">
                <div class="shrink-0">
                  <IconSprite v-if="speciesId(item.name)" :species-id="speciesId(item.name)" size="sm" />
                  <span v-else class="text-gray-400 text-xs">?</span>
                </div>
                <div class="min-w-0">
                  <div class="text-gray-800 font-medium truncate">{{ item.chinese_name || item.name }}</div>
                  <div class="text-gray-400 text-xs truncate">{{ item.name }}</div>
                </div>
              </div>
            </td>
            <td class="py-2 px-3 text-right">
              <div class="flex items-center justify-end gap-2">
                <div class="w-16 h-1.5 bg-gray-100 rounded-full overflow-hidden">
                  <div class="h-full rounded-full bg-gradient-to-r from-rose-400 to-rose-500"
                    :style="{ width: usagePercent(item.usage) + '%' }"></div>
                </div>
                <span class="text-gray-700 font-mono font-medium text-xs w-14 text-right">
                  {{ (item.usage * 100).toFixed(2) }}%
                </span>
              </div>
            </td>
            <td class="py-2 px-3 text-right">
              <span class="font-mono text-xs font-medium" :class="vcColor(item.viability_ceiling)">
                {{ item.viability_ceiling?.toFixed(1) || '-' }}
              </span>
            </td>
          </tr>
        </tbody>
      </table>
      <div v-if="!sortedItems.length" class="text-center py-16 text-gray-400 text-sm">暂无数据</div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed } from 'vue'
import IconSprite from '../shared/IconSprite.vue'

const props = defineProps({
  items: { type: Array, default: () => [] },
  loading: { type: Boolean, default: false },
  speciesMap: { type: Object, default: () => ({}) },
})
defineEmits(['select'])

const sortKey = ref('usage')
const sortAsc = ref(false)

const sortedItems = computed(() => {
  const arr = [...props.items]
  arr.sort((a, b) => {
    let va, vb
    if (sortKey.value === 'rank') { va = arr.indexOf(a); vb = arr.indexOf(b) }
    else if (sortKey.value === 'name') { va = a.name || ''; vb = b.name || '' }
    else if (sortKey.value === 'vc') { va = a.viability_ceiling || 0; vb = b.viability_ceiling || 0 }
    else { va = a.usage || 0; vb = b.usage || 0 }
    if (typeof va === 'string') return sortAsc.value ? va.localeCompare(vb) : vb.localeCompare(va)
    return sortAsc.value ? va - vb : vb - va
  })
  return arr
})

function norm(s) { return s.toLowerCase().replace(/[-'. \s]/g, '') }

function speciesId(name) {
  if (!name) return 0
  // 1. Exact match
  const lower = name.toLowerCase()
  if (props.speciesMap[lower]) return props.speciesMap[lower]
  // 2. Normalized match (strip hyphens, spaces, apostrophes, periods)
  const n = norm(name)
  if (props.speciesMap[n]) return props.speciesMap[n]
  // 3. Extract base species from form name (e.g. "Urshifu-Rapid-Strike" → "Urshifu")
  const base = name.split('-')[0]
  if (base !== name && props.speciesMap[base.toLowerCase()]) return props.speciesMap[base.toLowerCase()]
  if (name) console.warn(`[PokemonRankTable] speciesId not found: "${name}"`)
  return 0
}

function toggleSort(key) {
  if (sortKey.value === key) { sortAsc.value = !sortAsc.value; return }
  sortKey.value = key; sortAsc.value = key !== 'rank'
}
function rankClass(rank) {
  if (rank === 1) return 'bg-amber-400 text-white'
  if (rank === 2) return 'bg-gray-300 text-white'
  if (rank === 3) return 'bg-amber-600 text-white'
  return 'text-gray-400'
}
function usagePercent(v) { return Math.min(100, (v || 0) * 100) }
function vcColor(v) {
  if (!v) return 'text-gray-400'
  if (v >= 80) return 'text-green-600'
  if (v >= 70) return 'text-amber-600'
  return 'text-gray-500'
}
</script>
