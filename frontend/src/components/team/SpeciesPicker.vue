<template>
  <div class="space-y-4">
    <!-- Pokemon slots -->
    <div v-for="(slot, idx) in slots" :key="idx"
      class="bg-gray-700/50 rounded-lg p-3 border border-gray-600">
      <div class="flex items-center justify-between mb-3">
        <span class="text-xs font-bold text-gray-400">宝可梦 #{{ idx + 1 }}</span>
        <button v-if="slots.length > 1" @click="removeSlot(idx)"
          class="text-xs text-red-400 hover:text-red-300">移除</button>
      </div>

      <!-- Species search -->
      <div class="relative" :ref="el => dropdownRefs[idx] = el">
        <input v-model="slot.search" @focus="onFocus(idx)" @input="onSearch(idx)"
          placeholder="搜索宝可梦..."
          class="w-full bg-gray-900 border border-gray-700 rounded px-2 py-1.5 text-xs text-gray-300 cursor-pointer" />
        <div v-if="slot.showDropdown && slot.results.length"
          class="absolute z-30 w-full mt-1 bg-gray-800 border border-gray-600 rounded-lg shadow-xl max-h-48 overflow-y-auto">
          <div v-for="sp in slot.results" :key="sp.id" @mousedown.prevent="pickSpecies(idx, sp)"
            class="flex items-center gap-2 px-2 py-1.5 hover:bg-gray-700 cursor-pointer text-xs border-b border-gray-700/50">
            <img :src="'/ani/'+sp.id+'.gif'" class="w-8 h-8" @error="e=>e.target.style.display='none'" />
            <span class="text-white">#{{ sp.id }} {{ sp.name }}</span>
            <span v-for="t in (sp.types||[]).filter(Boolean)" :key="t"
              class="text-xs px-1 rounded text-white" :style="{background:typeColor(t)}">{{ typeLabel(t) }}</span>
          </div>
        </div>
      </div>

      <!-- Selected Pokemon details -->
      <div v-if="slot.selected" class="mt-3 space-y-2">
        <div class="flex items-center gap-2">
          <img :src="'/ani/'+slot.selected.id+'.gif'" class="w-12 h-12" />
          <div>
            <span class="text-white font-bold text-sm">#{{ slot.selected.id }} {{ slot.selected.name }}</span>
            <div class="flex gap-1 mt-0.5">
              <span v-for="t in (slot.selected.types||[]).filter(Boolean)" :key="t"
                class="text-xs px-1 rounded text-white" :style="{background:typeColor(t)}">{{ typeLabel(t) }}</span>
            </div>
          </div>
        </div>

        <!-- Species abilities (dropdown, not search) -->
        <div>
          <label class="text-xs text-gray-500">特性</label>
          <select v-model="slot.abilityId"
            class="w-full bg-gray-900 border border-gray-700 rounded px-2 py-1 text-xs text-gray-300 mt-1">
            <option :value="0">选择特性</option>
            <option v-for="aid in (slot.selected.abilities||[])" :key="aid" :value="aid">
              {{ getAbilityNameCached(aid) }}
            </option>
            <option v-if="slot.selected.hiddenAbilityID" :value="slot.selected.hiddenAbilityID">
              {{ getAbilityNameCached(slot.selected.hiddenAbilityID) }} (隐藏)
            </option>
          </select>
        </div>

        <!-- Learnable moves only -->
        <MovePicker v-model="slot.moves" :learnset="slot.selected.learnableMoves||[]" />

        <!-- Item -->
        <ItemPicker v-model="slot.item" />

        <!-- Level + Nature -->
        <div class="flex gap-3 text-xs">
          <span class="text-gray-500">Lv:</span>
          <input v-model.number="slot.level" type="number" min="1" max="100"
            class="bg-gray-900 border border-gray-700 rounded px-1 py-0.5 w-12 text-gray-300" />
          <span class="text-gray-500">性格:</span>
          <select v-model.number="slot.nature"
            class="bg-gray-900 border border-gray-700 rounded px-1 py-0.5 text-gray-300">
            <option v-for="n in NATURES" :key="n.value" :value="n.value">{{ n.name }}</option>
          </select>
        </div>
      </div>
    </div>

    <!-- Add slot button -->
    <button v-if="slots.length < maxSlots" @click="addSlot"
      class="w-full py-2 border border-dashed border-gray-600 rounded-lg text-xs text-gray-500 hover:border-gray-400 hover:text-gray-300 transition-colors">
      + 添加宝可梦 ({{ slots.length }}/{{ maxSlots }})
    </button>
  </div>
</template>

<script setup>
import { ref, watch, onMounted, onUnmounted } from 'vue'
import { TYPES } from '../../utils/enums'
import { searchSpecies, getAbilityName } from '../../api/dataWs'
import MovePicker from './MovePicker.vue'
import ItemPicker from './ItemPicker.vue'

const props = defineProps({ maxSlots: { type: Number, default: 6 }, modelValue: { type: Array, default: () => [] } })
const emit = defineEmits(['update:modelValue'])

const NATURES = [{value:3,name:'Adamant'},{value:10,name:'Timid'},{value:13,name:'Jolly'},{value:15,name:'Modest'},{value:23,name:'Careful'},{value:5,name:'Bold'},{value:8,name:'Impish'},{value:0,name:'Hardy'}]

const slots = ref([])
const dropdownRefs = ref({})
const abilityNameCache = ref({})
let timers = {}

function createSlot() {
  return { search: '', results: [], showDropdown: false, selected: null, abilityId: 0, moves: [], item: null, level: 50, nature: 3 }
}

// Initialize from modelValue
watch(() => props.modelValue, (val) => {
  if (val && val.length > 0 && slots.value.length === 0) {
    slots.value = val.map(p => ({ ...createSlot(), selected: { id: p.speciesID, name: '#'+p.speciesID, types: [], baseStats: [], abilities: [], learnableMoves: [] }, abilityId: p.ability||0, moves: (p.moves||[]).map(m=>({id:m,name:'#'+m,type:''})), item: p.item||null, level: p.level||50, nature: p.nature||3 }))
  } else if (slots.value.length === 0) {
    slots.value = [createSlot()]
  }
}, { immediate: true })

// Emit team config
function emitTeam() {
  const team = slots.value.filter(s => s.selected).map(s => ({
    speciesID: s.selected.id, level: s.level, ability: s.abilityId, nature: s.nature,
    moves: s.moves.map(m => m.id), item: s.item?.id || 0
  }))
  emit('update:modelValue', team)
}
watch(slots, () => emitTeam(), { deep: true })

// Click outside
function onClickOutside(e) {
  slots.value.forEach((s, i) => {
    const el = dropdownRefs.value[i]
    if (el && !el.contains(e.target)) s.showDropdown = false
  })
}
onMounted(() => document.addEventListener('click', onClickOutside))
onUnmounted(() => document.removeEventListener('click', onClickOutside))

async function onFocus(idx) {
  slots.value[idx].showDropdown = true
  if (slots.value[idx].results.length === 0) {
    try { slots.value[idx].results = await searchSpecies('') } catch { slots.value[idx].results = [] }
  }
}
async function onSearch(idx) {
  clearTimeout(timers[idx])
  timers[idx] = setTimeout(async () => {
    slots.value[idx].showDropdown = true
    try { slots.value[idx].results = await searchSpecies(slots.value[idx].search) } catch { slots.value[idx].results = [] }
  }, 150)
}
function pickSpecies(idx, sp) {
  const s = slots.value[idx]
  s.selected = sp; s.showDropdown = false; s.search = ''; s.results = []; s.moves = []; s.item = null; s.abilityId = 0
  // Preload ability names
  const allAbilities = [...(sp.abilities||[]), sp.hiddenAbilityID].filter(Boolean)
  allAbilities.forEach(aid => {
    if (!abilityNameCache.value[aid]) getAbilityName(aid).then(n => abilityNameCache.value[aid] = n)
  })
}
function addSlot() { if (slots.value.length < props.maxSlots) slots.value.push(createSlot()) }
function removeSlot(idx) { slots.value.splice(idx, 1); if (slots.value.length === 0) slots.value.push(createSlot()) }

function getAbilityNameCached(id) { return abilityNameCache.value[id] || `#${id}` }

function typeColor(name) { for (const t of Object.values(TYPES)) { if (t.name === name) return t.color }; return '#999' }
function typeLabel(name) { for (const t of Object.values(TYPES)) { if (t.name === name) return t.label }; return name }
</script>
