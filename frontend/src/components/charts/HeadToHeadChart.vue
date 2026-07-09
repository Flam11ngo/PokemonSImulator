<template>
  <div class="glass-card p-5">
    <h3 class="text-base font-bold text-gray-800 mb-4">⚔️ 对位胜率</h3>

    <div class="flex items-center justify-center gap-3 mb-4">
      <input v-model="search1" @input="filterSpecies1" placeholder="精灵A..."
        class="text-sm px-4 py-2 rounded-full bg-gray-50 border border-gray-200 w-40 focus:border-blue-400 focus:outline-none" />
      <span class="text-lg font-black text-gray-300">VS</span>
      <input v-model="search2" @input="filterSpecies2" placeholder="精灵B..."
        class="text-sm px-4 py-2 rounded-full bg-gray-50 border border-gray-200 w-40 focus:border-blue-400 focus:outline-none" />
      <button @click="query" :disabled="!s1 || !s2"
        class="text-sm px-5 py-2 rounded-full bg-blue-500 text-white font-bold disabled:bg-gray-300 hover:bg-blue-600 transition-colors">查询</button>
    </div>

    <div v-if="result && result.total > 0" class="rounded-xl bg-gradient-to-r from-blue-50 to-red-50 border border-gray-200 p-6">
      <div class="flex items-center justify-between">
        <!-- Left Pokemon -->
        <div class="flex-1 flex flex-col items-center">
          <img v-if="result.s1_sprite" :src="result.s1_sprite" class="w-20 h-20" />
          <p class="text-lg font-bold mt-2">{{ result.s1_name }}</p>
          <p class="text-xs text-gray-500">{{ result.s1_types || '?' }}</p>
          <p class="text-3xl font-extrabold mt-1" :class="(result.s1_rate||0) >= 50 ? 'text-green-500' : 'text-red-500'">{{ result.s1_rate }}%</p>
          <p class="text-xs text-gray-500">{{ result.s1_wins || 0 }}胜</p>
          <div class="w-full max-w-[120px] h-2 rounded-full bg-gray-200 mt-1 overflow-hidden">
            <div class="h-full rounded-full bg-green-400" :style="{width: result.s1_rate+'%'}"></div>
          </div>
        </div>

        <!-- VS center -->
        <div class="flex flex-col items-center px-4">
          <span class="text-4xl font-black text-gray-300 tracking-widest">VS</span>
          <span class="text-xs text-gray-400 mt-1">{{ result.total }}场</span>
        </div>

        <!-- Right Pokemon -->
        <div class="flex-1 flex flex-col items-center">
          <img v-if="result.s2_sprite" :src="result.s2_sprite" class="w-20 h-20" />
          <p class="text-lg font-bold mt-2">{{ result.s2_name }}</p>
          <p class="text-xs text-gray-500">{{ result.s2_types || '?' }}</p>
          <p class="text-3xl font-extrabold mt-1" :class="(result.s2_rate||0) >= 50 ? 'text-green-500' : 'text-red-500'">{{ result.s2_rate }}%</p>
          <p class="text-xs text-gray-500">{{ result.s2_wins || 0 }}胜</p>
          <div class="w-full max-w-[120px] h-2 rounded-full bg-gray-200 mt-1 overflow-hidden">
            <div class="h-full rounded-full bg-blue-400" :style="{width: result.s2_rate+'%'}"></div>
          </div>
        </div>
      </div>
    </div>
    <div v-else-if="searched && result && result.total === 0" class="flex flex-col items-center justify-center text-gray-400 py-16">
      <div class="text-4xl mb-3">📭</div>
      <p class="text-sm">无对战数据</p>
    </div>
    <div v-else class="flex flex-col items-center justify-center text-gray-400 py-16">
      <div class="text-4xl mb-3">⚔️</div>
      <p class="text-sm">选择两个精灵查询对位胜率</p>
    </div>
  </div>
</template>

<script setup>
import { ref } from 'vue'
import { statsAPI } from '../../api/stats'

const props = defineProps({ loading: { type: Boolean, default: false } })

const search1 = ref('')
const search2 = ref('')
const s1 = ref(null)
const s2 = ref(null)
const result = ref(null)
const searched = ref(false)

async function filterSpecies1() {
  try {
    const resp = await statsAPI.deepMeta()
    const list = (resp.data || resp || [])
    const q = search1.value.toLowerCase().trim()
    const match = q ? list.find(r => (r.species_name || '').toLowerCase().includes(q)) : null
    s1.value = match ? match.species_id : null
  } catch {}
}
async function filterSpecies2() {
  try {
    const resp = await statsAPI.deepMeta()
    const list = (resp.data || resp || [])
    const q = search2.value.toLowerCase().trim()
    const match = q ? list.find(r => (r.species_name || '').toLowerCase().includes(q)) : null
    s2.value = match ? match.species_id : null
  } catch {}
}

async function query() {
  if (!s1.value || !s2.value) return
  searched.value = true
  try {
    const resp = await statsAPI.deepHeadToHead(s1.value, s2.value)
    result.value = resp.data || resp || { total: 0 }
  } catch { result.value = { total: 0 } }
}
</script>
