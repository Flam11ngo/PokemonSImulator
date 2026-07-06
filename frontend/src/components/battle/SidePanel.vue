<template>
  <div class="bg-gray-800 rounded-lg p-4 border border-gray-700">
    <!-- Side name + active pokemon -->
    <div class="flex items-center justify-between mb-3">
      <h3 class="text-sm font-bold text-gray-300 uppercase tracking-wide">{{ side?.name || 'Side' }}</h3>
      <div class="flex gap-1">
        <StatusBadge v-for="s in activeStatuses" :key="s.id" :status="s" />
      </div>
    </div>

    <!-- Active Pokemon detail -->
    <div v-if="active" class="space-y-3">
      <div class="flex items-center gap-3">
        <!-- Pokemon animated sprite (Pokemon Showdown / PokeAPI) -->
        <PokemonSprite :species-id="active.speciesId" size="lg" :animated="true" />
        <div class="flex-1 min-w-0">
          <div class="flex items-center gap-2 flex-wrap">
            <span class="text-lg font-bold text-white truncate">
              #{{ active.speciesId }}
            </span>
            <TypeBadge v-if="active.types?.[0]" :type-id="active.types[0]" />
            <TypeBadge v-if="active.types?.[1]" :type-id="active.types[1]" />
          </div>
          <div class="flex items-center gap-2 mt-1">
            <span class="text-sm" :class="active.fainted ? 'text-red-400' : 'text-green-400'">
              {{ active.fainted ? '濒死' : `HP: ${active.hp}/${active.maxHp}` }}
            </span>
          </div>
        </div>
      </div>

      <!-- HP Bar -->
      <HPBar :current="active.hp || 0" :max="active.maxHp || 1" />

      <!-- Stat stages (if non-zero) -->
      <div v-if="hasStatChanges" class="grid grid-cols-4 gap-1 text-xs">
        <span v-for="(stage, i) in statsToShow" :key="i"
          class="px-1 py-0.5 rounded text-center"
          :class="stageClass(stage)"
        >
          {{ statNames[i] }} {{ formatStatStage(stage) }}
        </span>
      </div>

      <!-- Moves with type/category icons -->
      <div class="grid grid-cols-2 gap-1">
        <div v-for="(move, i) in (active.moves || [])" :key="i"
          class="text-xs px-2 py-1 rounded bg-gray-700"
        >
          <div class="flex items-center gap-1 mb-0.5">
            <img v-if="moveInfo[move.id]?.type" :src="'/types/'+capitalize(moveInfo[move.id].type)+'.png'" class="h-3 w-auto" />
            <img v-if="moveInfo[move.id]?.category" :src="'/categories/'+moveInfo[move.id].category+'.png'" class="h-3 w-auto" />
            <span class="text-gray-500 ml-auto font-mono text-[10px]">{{ move.pp }}/{{ move.maxPp }}</span>
          </div>
          <div class="text-gray-300 font-medium truncate text-[11px]">{{ moveInfo[move.id]?.name || '#'+move.id }}</div>
        </div>
      </div>
    </div>

    <!-- Empty state -->
    <div v-else class="text-center text-gray-500 py-4 text-sm">
      无活跃宝可梦
    </div>

    <!-- Bench preview (small sprites) -->
    <div v-if="bench.length > 0" class="mt-4 flex gap-2 justify-center">
      <div v-for="(p, i) in bench" :key="i"
        class="w-10 h-10 rounded-full border-2 flex items-center justify-center overflow-hidden"
        :class="p.fainted ? 'border-red-800 bg-red-900/50 opacity-50' : 'border-gray-600 bg-gray-700'"
      >
        <PokemonSprite v-if="!p.fainted" :species-id="p.speciesId" size="sm" :animated="false" />
        <span v-else class="text-xs">💀</span>
      </div>
    </div>
  </div>
</template>

<script setup>
import { computed, ref, watch } from 'vue'
import HPBar from './HPBar.vue'
import StatusBadge from './StatusBadge.vue'
import TypeBadge from '../shared/TypeBadge.vue'
import PokemonSprite from '../shared/PokemonSprite.vue'
import { formatStatStage, statStageClass } from '../../utils/formatters'
import { STAT_NAMES } from '../../utils/enums'
import { getMove } from '../../api/dataWs'

const props = defineProps({
  side: { type: Object, default: null },
  showBench: { type: Boolean, default: true },
})

const active = computed(() => {
  if (!props.side) return null
  const pokemons = props.side.pokemons || []
  const activeIdx = props.side.active || 0
  return pokemons[activeIdx] || pokemons[0] || null
})

const bench = computed(() => {
  if (!props.side || !props.showBench) return []
  const pokemons = props.side.pokemons || []
  const activeIdx = props.side.active || 0
  return pokemons.filter((_, i) => i !== activeIdx)
})

const activeStatuses = computed(() => {
  if (!active.value) return []
  return (active.value.inBattleStatus || []).filter(s => s.id !== 0)
})

const hasStatChanges = computed(() => {
  if (!active.value?.statStages) return false
  return active.value.statStages.some(s => s !== 0)
})

const statsToShow = computed(() => active.value?.statStages?.slice(0, 5) || [])
const statNames = STAT_NAMES

// Move info cache
const moveInfo = ref({})
function capitalize(s) { return s ? s.charAt(0).toUpperCase()+s.slice(1).toLowerCase() : '' }

watch(active, async (pkm) => {
  if (!pkm?.moves) return
  for (const m of pkm.moves) {
    if (m.id && !moveInfo.value[m.id]) {
      const data = await getMove(m.id)
      moveInfo.value[m.id] = {
        name: data?.name || '#'+m.id,
        type: data?.type || '',
        category: data?.category || '',
      }
    }
  }
}, { immediate: true })
</script>
