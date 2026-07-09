<template>
  <div v-if="visible" class="fixed inset-0 z-50 flex items-center justify-center bg-black bg-opacity-50" @click.self="$emit('close')">
    <div class="bg-white rounded-lg shadow-xl w-full max-w-2xl max-h-[80vh] flex flex-col">
      <!-- Header -->
      <div class="flex items-center justify-between p-4 border-b">
        <h2 class="text-xl font-bold text-gray-800">
          🐾 {{ speciesName }} 详细数据
        </h2>
        <button @click="$emit('close')" class="text-gray-400 hover:text-gray-600 text-2xl leading-none">&times;</button>
      </div>

      <!-- Tab bar -->
      <div class="flex border-b px-4">
        <button
          v-for="tab in tabs" :key="tab.key"
          @click="activeTab = tab.key"
          :class="[
            'px-4 py-2.5 text-sm font-medium transition-colors border-b-2',
            activeTab === tab.key
              ? 'border-blue-500 text-blue-600'
              : 'border-transparent text-gray-500 hover:text-gray-700'
          ]"
        >{{ tab.label }}</button>
      </div>

      <!-- Tab content -->
      <div class="p-4 flex-1 overflow-y-auto">
        <DataTable
          :title="activeTabLabel"
          :columns="currentColumns"
          :rows="currentRows"
          :loading="loading"
          :emptyText="`暂无${activeTabLabel}数据`"
          :searchPlaceholder="`搜索${activeTabLabel}...`"
          idKey="id"
          defaultSort="usage_count"
        />
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, watch } from 'vue'
import DataTable from '../charts/DataTable.vue'
import { statsAPI } from '../../api/stats'

const props = defineProps({
  visible: { type: Boolean, default: false },
  speciesId: { type: Number, default: null },
  speciesName: { type: String, default: '' },
})
const emit = defineEmits(['close'])

const loading = ref(false)
const moves = ref([])
const items = ref([])
const abilities = ref([])
const activeTab = ref('moves')

const tabs = [
  { key: 'moves', label: '⚡ 招式' },
  { key: 'items', label: '🎒 道具' },
  { key: 'abilities', label: '🌟 特性' },
]

const activeTabLabel = computed(() => {
  const t = tabs.find(t => t.key === activeTab.value)
  return t ? t.label : ''
})

const moveCols = [
  { key: 'move_name', label: '招式名称', sortable: true },
  { key: 'usage_count', label: '使用次数', sortable: true, spark: true, class: 'text-right' },
]

const itemCols = [
  { key: 'item_name', label: '道具名称', sortable: true },
  { key: 'usage_count', label: '携带次数', sortable: true, spark: true, class: 'text-right' },
]

const abilityCols = [
  { key: 'ability_name', label: '特性名称', sortable: true },
  { key: 'usage_count', label: '携带次数', sortable: true, spark: true, class: 'text-right' },
]

const columnMap = { moves: moveCols, items: itemCols, abilities: abilityCols }
const currentColumns = computed(() => columnMap[activeTab.value] || moveCols)
const currentRows = computed(() => {
  const m = { moves, items, abilities }
  return m[activeTab.value].value || []
})

async function fetchDetail(id) {
  if (!id) return
  loading.value = true
  try {
    const resp = await statsAPI.pokemonDetail(id)
    const detail = resp.data || resp || {}
    moves.value = detail.moves || []
    items.value = detail.items || []
    abilities.value = detail.abilities || []
  } catch (e) {
    console.error('Failed to fetch species detail:', e)
    moves.value = []
    items.value = []
    abilities.value = []
  } finally {
    loading.value = false
  }
}

watch(() => props.speciesId, (id) => {
  if (id) {
    activeTab.value = 'moves'
    fetchDetail(id)
  }
})
</script>
