<template>
  <div class="glass-card p-5">
    <h3 class="text-base font-bold text-gray-700 mb-3">{{ title }}</h3>
    <div v-if="rows.length > 0" class="overflow-x-auto">
      <table class="w-full text-sm">
        <thead>
          <tr class="text-left text-gray-400 text-xs border-b border-gray-100">
            <th class="pb-2 w-8">#</th>
            <th class="pb-2">名称</th>
            <th class="pb-2 text-right">{{ colLabel }}</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="(row, i) in rows.slice(0, 10)" :key="i"
            class="border-b border-gray-50 hover:bg-gray-50 transition-colors">
            <td class="py-2 text-gray-400 font-mono text-xs">{{ i + 1 }}</td>
            <td class="py-2 text-gray-700 font-medium">{{ row.label }}</td>
            <td class="py-2 text-right text-gray-600">
              <span class="bg-blue-50 text-blue-700 px-2 py-0.5 rounded-full text-xs">{{ row.value }}</span>
            </td>
          </tr>
        </tbody>
      </table>
    </div>
    <div v-else class="text-center text-gray-400 py-12">暂无数据</div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { statsAPI } from '../../api/stats'

const props = defineProps({
  title: { type: String, default: '排行榜' },
  source: { type: String, default: 'moves' },  // moves | items | abilities
  colLabel: { type: String, default: '使用次数' },
})

const rows = ref([])

const labelMap = {
  moves: (r) => r.move_name || `#${r.move_id}`,
  items: (r) => r.item_name || `#${r.item_id}`,
  abilities: (r) => r.ability_name || `#${r.ability_id}`,
}

onMounted(async () => {
  try {
    let result
    if (props.source === 'moves') result = await statsAPI.deepMoves()
    else if (props.source === 'items') result = await statsAPI.deepItems()
    else if (props.source === 'abilities') result = await statsAPI.deepAbilities()
    const data = (result.data || result || [])
    const fn = labelMap[props.source] || ((r) => '')
    rows.value = data.map(r => ({
      label: fn(r),
      value: r.times_seen || r.uses || '?',
    }))
  } catch { /* no data */ }
})
</script>
