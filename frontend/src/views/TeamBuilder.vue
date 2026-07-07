<template>
  <div class="flex gap-4 h-[calc(100vh-80px)]">
    <!-- LEFT: Team slots -->
    <div class="w-64 shrink-0 space-y-1.5 overflow-y-auto">
      <input v-model="teamName" placeholder="队伍名称" class="bg-white/70 border border-gray-200 rounded-xl px-3 py-1.5 text-sm text-gray-800 w-full mb-2" />
      <button @click="saveTeam" class="w-full py-2 bg-rose-400 hover:bg-rose-500 text-white rounded-full text-xs font-bold">💾 保存</button>
      <div v-if="savedTeams.length" class="mt-3 pt-3 border-t border-gray-200">
        <div class="flex items-center justify-between mb-1">
          <span class="text-xs text-gray-400">已保存</span>
          <div class="flex gap-1">
            <button @click="flipTeam(-1)"
              class="text-xs px-1.5 rounded hover:bg-gray-200 text-gray-500">◀</button>
            <span class="text-xs text-gray-400">{{ teamPage + 1 }}/{{ savedTeams.length }}</span>
            <button @click="flipTeam(1)"
              class="text-xs px-1.5 rounded hover:bg-gray-200 text-gray-500">▶</button>
          </div>
        </div>
        <div v-if="savedTeams[teamPage]" class="text-xs px-2 py-1.5 rounded-lg border border-gray-200 flex items-center gap-1">
          <span class="flex-1 truncate cursor-pointer" @click="loadTeam(savedTeams[teamPage])">
            {{ savedTeams[teamPage].name }} ({{ savedTeams[teamPage].pokemon?.length||0 }})
          </span>
          <button @click.stop="deleteTeam(savedTeams[teamPage].name)" class="text-gray-400 hover:text-red-400">✕</button>
        </div>
      </div>
      <div v-for="(slot,idx) in slots" :key="idx" @click="selectSlot(idx)"
        draggable="true"
        @dragstart="onDragStart(idx, $event)"
        @dragover.prevent="onDragOver(idx)"
        @dragleave="onDragLeave"
        @drop="onDrop(idx)"
        @dragend="onDragEnd"
        class="rounded-xl p-2.5 cursor-pointer border-2 transition-all select-none"
        :class="[
          activeIdx===idx ? 'border-rose-300 shadow-sm' : 'border-transparent hover:border-gray-200',
          dragIdx===idx ? 'opacity-30' : '',
          dragOverIdx===idx ? 'border-blue-400 bg-blue-50' : ''
        ]">
        <div class="flex items-center gap-2">
          <div class="flex items-center justify-center shrink-0">
            <IconSprite v-if="slot.pkm" :species-id="slot._spriteId || slot.pkm.speciesID" size="md" />
            <span v-else class="text-gray-400 text-lg">+</span>
          </div>
          <div class="flex-1 min-w-0">
            <div class="text-xs text-gray-800 font-bold truncate">{{ slot._name||'空位 #'+(idx+1) }}</div>
            <div v-if="slot._types?.length" class="flex gap-0.5 mt-0.5">
              <img v-for="t in slot._types.filter(isValidType)" :key="t" :src="'/types/'+capitalize(t)+'.png'" class="h-3 w-auto" />
            </div>
          </div>
          <button v-if="slot.pkm" @click.stop="openBuildModal(idx)" class="text-xs px-1.5 py-1 rounded hover:bg-indigo-100 text-indigo-500 shrink-0" title="快速配置">⚡</button>
          <button v-if="slot.pkm" @click.stop="removeSlot(idx)" class="text-xs px-1.5 py-1 rounded hover:bg-red-100 text-red-400 shrink-0" title="移除">✕</button>
        </div>
      </div>
    </div>

    <!-- RIGHT: Editor -->
    <div class="flex-1 bg-white rounded-2xl border border-gray-200 shadow-sm flex flex-col overflow-hidden">
      <!-- Pokemon summary bar (when selected) -->
      <div v-if="activeSlot.pkm" class="px-4 py-3 border-b border-gray-100 shrink-0 flex items-start gap-3 flex-wrap bg-gradient-to-b from-white to-gray-50/50">
        <!-- Left: name + sprite (stacked) -->
        <div class="flex flex-col items-center shrink-0 w-28">
          <div class="text-sm font-bold text-gray-800 text-center truncate w-full">{{ activeSlot._name }}</div>
          <div class="text-xs text-gray-400">#{{ activeSlot.pkm.speciesID }}</div>
          <div class="w-28 h-28 mt-0.5">
            <img v-if="!summaryGifFailed"
                 :src="frontSprite(activeSlot.pkm.speciesID)"
                 @error="summaryGifFailed = true"
                 class="w-28 h-28 object-contain drop-shadow-md" />
            <IconSprite v-else :species-id="activeSlot.pkm.speciesID" size="lg" class="drop-shadow-md" />
          </div>
          <div class="flex gap-0.5 mt-0.5">
            <img v-for="t in (activeSlot._types||[]).filter(isValidType)" :key="t" :src="'/types/'+capitalize(t)+'.png'" class="h-4 w-auto" />
          </div>
        </div>

        <!-- Center: 2x2 move grid -->
        <div class="grid grid-cols-2 gap-2 w-[320px] shrink-0">
          <button v-for="i in 4" :key="'m'+i" @click="editMoveSlot=i-1; activeTab='moves'"
            class="text-sm border-2 transition-all duration-150 p-3 h-[68px] text-left"
            style="border-radius: 4px"
            :class="activeSlot.pkm.moves[i-1]
              ? (editMoveSlot===i-1 ? 'bg-blue-100 border-blue-400 shadow-inner' : 'bg-white border-gray-200 hover:border-blue-400 hover:shadow-inner')
              : (editMoveSlot===i-1 ? 'bg-blue-50 border-blue-300 border-dashed' : 'bg-gray-50 border-dashed border-gray-250 text-gray-400 hover:border-gray-350 hover:bg-white')">
            <template v-if="activeSlot.pkm.moves[i-1]">
              <div class="flex items-center gap-1.5 mb-1">
                <img :src="'/types/'+capitalize(moveType(activeSlot.pkm.moves[i-1]))+'.png'" class="h-4 w-auto" />
                <img :src="'/categories/'+moveCat(activeSlot.pkm.moves[i-1])+'.png'" class="h-4 w-auto" />
                <span class="ml-auto text-[10px] text-gray-400 font-mono">{{ movePower(activeSlot.pkm.moves[i-1])||'-' }}·{{ movePP(activeSlot.pkm.moves[i-1])||'-' }}</span>
              </div>
              <div class="text-gray-700 font-semibold text-xs truncate">{{ moveNamesCache[activeSlot.pkm.moves[i-1]]||'...' }}</div>
            </template>
            <template v-else>
              <div class="flex items-center justify-center h-full text-gray-350 text-sm">招式 {{ i }}</div>
            </template>
          </button>
        </div>

        <!-- Right: Item + Nature (stacked) -->
        <div class="flex flex-col gap-2 shrink-0 w-[150px]">
          <button @click="activeTab='items'"
            class="text-sm border-2 transition-all duration-150 p-3 text-left h-[68px]"
            style="border-radius: 4px"
            :class="activeSlot.pkm.item
              ? (activeTab==='items' ? 'bg-amber-100 border-amber-400 shadow-inner' : 'bg-white border-gray-200 hover:border-amber-400 hover:shadow-inner')
              : (activeTab==='items' ? 'bg-amber-50 border-amber-300 border-dashed' : 'bg-gray-50 border-dashed border-gray-250 text-gray-400 hover:border-gray-350')">
            <template v-if="activeSlot._itemName">
              <div class="flex items-center gap-2">
                <div :style="itemSpriteStyle(itemImgName(activeSlot.pkm.item))" class="w-6 h-6 shrink-0 rounded" />
                <span class="text-amber-700 font-semibold text-xs truncate">{{ activeSlot._itemName }}</span>
              </div>
            </template>
            <template v-else>
              <div class="flex items-center justify-center h-full text-gray-350 text-sm">🎒 道具</div>
            </template>
          </button>
          <button @click="activeTab='details'"
            class="text-sm border-2 transition-all duration-150 p-3 text-left h-[68px]"
            style="border-radius: 4px"
            :class="activeSlot.pkm.nature !== undefined
              ? (activeTab==='details' ? 'bg-purple-50 border-purple-300 shadow-inner' : 'bg-white border-gray-200 hover:border-purple-300 hover:shadow-inner')
              : (activeTab==='details' ? 'bg-purple-50 border-purple-300 border-dashed' : 'bg-gray-50 border-dashed border-gray-250 text-gray-400')">
            <div class="text-gray-700 font-semibold text-xs">{{ natureName(activeSlot.pkm.nature) }}</div>
            <div class="text-gray-400 text-[11px] mt-0.5">{{ natureDesc(activeSlot.pkm.nature) }}</div>
          </button>
        </div>

      </div>

      <!-- Tab bar -->
      <div class="flex border-b border-gray-200 shrink-0">
        <button v-for="tab in tabs" :key="tab.key" @click="activeTab=tab.key"
          class="flex-1 py-2.5 text-sm font-medium transition-all border-b-2"
          :class="activeTab===tab.key ? 'text-rose-400 border-rose-400 bg-rose-50/50' : 'text-gray-400 border-transparent hover:text-gray-600'">
          {{ tab.label }}
        </button>
      </div>

      <!-- Search + List -->
      <div class="flex-1 flex flex-col overflow-hidden">
        <div class="p-3 shrink-0">
          <input v-if="activeTab==='species'" v-model="speciesSearch" placeholder="搜索名称/ID/属性..." class="w-full bg-gray-50 border border-gray-200 rounded-xl px-3 py-2 text-sm" />
          <input v-if="activeTab==='moves'" v-model="moveSearch" @input="onMoveSearch" placeholder="搜索招式..." class="w-full bg-gray-50 border border-gray-200 rounded-xl px-3 py-2 text-sm" />
          <input v-if="activeTab==='items'" v-model="itemSearch" @input="onItemSearch" placeholder="搜索道具..." class="w-full bg-gray-50 border border-gray-200 rounded-xl px-3 py-2 text-sm" />
        </div>

        <div class="flex-1 overflow-y-auto px-3 pb-3">
          <!-- Species Sheet -->
          <div v-if="activeTab==='species'" class="flex flex-col h-full">
            <!-- Column headers -->
            <div class="flex items-center gap-1 text-[10px] text-gray-400 uppercase tracking-wider px-1 pb-1 shrink-0 border-b border-gray-100">
              <span class="w-7 text-center cursor-pointer hover:text-gray-600" @click="toggleSpeciesSort('id')">#{{ sortArrow('id') }}</span>
              <span class="w-24"></span>
              <span class="flex-1 cursor-pointer hover:text-gray-600 pl-1" @click="toggleSpeciesSort('name')">名称{{ sortArrow('name') }}</span>
              <span class="w-14 text-center">属性</span>
              <span v-for="s in statCols" :key="s.k" class="w-8 text-center cursor-pointer hover:text-gray-600" @click="toggleSpeciesSort(s.k)">{{ s.n }}{{ sortArrow(s.k) }}</span>
              <span class="w-12 text-center cursor-pointer hover:text-gray-600" @click="toggleSpeciesSort('pickrate')">Pick率{{ sortArrow('pickrate') }}</span>
              <span class="w-10 text-center cursor-pointer hover:text-gray-600" @click="toggleSpeciesSort('bst')">BST{{ sortArrow('bst') }}</span>
            </div>
            <!-- Rows -->
            <div class="flex-1 overflow-y-auto">
              <div v-for="sp in speciesSorted" :key="sp.id"
                @click="pickSpecies(sp)"
                class="flex items-center gap-1 px-1 py-0.5 cursor-pointer border-b border-gray-50 hover:bg-rose-50/60 transition-colors"
                :class="activeSlot.pkm?.speciesID===sp.id ? 'bg-rose-100 ring-1 ring-rose-200 rounded' : ''">
                <span class="w-7 text-center text-[10px] text-gray-400 font-mono shrink-0">#{{ sp.id }}</span>
                <IconSprite :species-id="sp.id" size="lg" class="shrink-0" />
                <span class="flex-1 text-sm text-gray-800 font-medium truncate pl-1">{{ sp.chineseName || sp.name }}</span>
                <div class="w-14 flex gap-0.5 justify-center shrink-0">
                  <span v-for="t in (sp.types||[]).filter(isValidType)" :key="t"
                    class="px-1 py-px rounded-full text-white text-[9px] font-medium leading-tight"
                    :style="{ backgroundColor: typeColor(t) }">{{ typeLabel(t) }}</span>
                </div>
                <span v-for="(s,i) in statCols" :key="s.k"
                  class="w-8 text-center text-[11px] font-mono shrink-0"
                  :class="statColor(s.k, (sp.baseStats||[])[i])">{{ (sp.baseStats||[])[i] ?? '-' }}</span>
                <span class="w-12 text-center text-[11px] font-mono shrink-0"
                  :class="pickColor(smogonUsage[sp.name.toLowerCase()])">
                  {{ fmtPickrate(smogonUsage[sp.name.toLowerCase()]) }}
                </span>
                <span class="w-10 text-center text-[11px] font-bold font-mono text-gray-700 shrink-0">{{ (sp.baseStats||[]).reduce((a,b)=>a+b,0) }}</span>
              </div>
              <div v-if="!speciesSorted.length" class="text-center text-gray-400 py-8 text-sm">没有匹配的宝可梦</div>
            </div>
          </div>

          <!-- Moves list -->
          <div v-if="activeTab==='moves' && activeSlot.pkm">
            <div v-for="mv in mvResults" :key="mv.id" @click="toggleMove(mv)"
              class="flex items-center gap-3 px-3 py-2 hover:bg-gray-50 cursor-pointer rounded-xl border-b border-gray-50 text-sm"
              :class="{'bg-rose-50': activeSlot.pkm.moves.includes(mv.id)}">
              <img :src="'/types/'+capitalize(mv.type)+'.png'" class="h-4 w-auto shrink-0" />
              <span class="font-medium" :style="{color: typeColor(mv.type)}">{{ mv.chineseName || mv.name }}</span>
              <span class="text-gray-400 text-xs ml-auto shrink-0">{{ mv.power||'-' }}pwr · {{ mv.accuracy||'-' }}%</span>
              <span v-if="activeSlot.pkm.moves.includes(mv.id)" class="text-rose-400 font-bold shrink-0">✓</span>
            </div>
          </div>

          <!-- Items -->
          <div v-if="activeTab==='items' && activeSlot.pkm">
            <div v-for="it in itemResults" :key="it.id" @click="selectItem(it)"
              class="flex items-center gap-3 px-3 py-2 hover:bg-gray-50 cursor-pointer rounded-xl border-b border-gray-50 text-sm"
              :class="{'bg-rose-50': activeSlot.pkm.item===it.id}">
              <div :style="itemSpriteStyle(it.name)" class="w-6 h-6 shrink-0 rounded" />
              <div class="flex-1">
                <span class="text-gray-700 font-medium">{{ it.chineseName || it.name }}</span>
                <p class="text-gray-400 text-xs truncate">{{ it.description||'' }}</p>
              </div>
              <span v-if="activeSlot.pkm.item===it.id" class="text-rose-400 font-bold shrink-0">✓</span>
            </div>
          </div>

          <!-- Details -->
          <div v-if="activeTab==='details' && activeSlot.pkm" class="space-y-4">
            <div>
              <label class="text-xs text-gray-400 mb-1 block">特性</label>
              <select v-model.number="activeSlot.pkm.ability" class="w-full bg-gray-50 border border-gray-200 rounded-xl px-3 py-2 text-sm">
                <option :value="0">选择特性</option>
                <option v-for="aid in (activeSlot._species?.abilities||[])" :key="aid" :value="aid">{{ abilityNamesCache[aid]||'#'+aid }}</option>
                <option v-if="activeSlot._species?.hiddenAbilityID" :value="activeSlot._species.hiddenAbilityID">{{ abilityNamesCache[activeSlot._species.hiddenAbilityID]||'...' }} (隐)</option>
              </select>
            </div>
            <div>
              <label class="text-xs text-gray-400 mb-1 block">性格</label>
              <div class="grid grid-cols-2 gap-1">
                <div v-for="n in NATURES" :key="n.v" @click="activeSlot.pkm.nature=n.v"
                  class="px-2 py-1 rounded-lg cursor-pointer text-xs" :class="activeSlot.pkm.nature===n.v ? 'bg-rose-100 text-rose-600' : 'text-gray-500 hover:bg-gray-50'">{{ n.name }} {{ n.desc }}</div>
              </div>
            </div>
            <div>
              <label class="text-xs text-gray-400 mb-1 block">EVs (剩余 {{ 508 - evTotal }})</label>
              <div class="grid grid-cols-6 gap-1 text-xs">
                <div v-for="s in EVS" :key="s.k" class="text-center">
                  <div class="text-gray-400">{{ s.n }}</div>
                  <input v-model.number="activeSlot.pkm.evs[s.k]" type="number" min="0" max="255"
                    @change="clampEV(s.k)" @blur="clampEV(s.k)"
                    class="w-full bg-gray-50 border border-gray-200 rounded px-1 py-0.5 text-gray-700 text-center" />
                </div>
              </div>
            </div>
            <div class="bg-gray-50 rounded-xl p-3">
              <div class="text-xs text-gray-400 mb-1">能力值 (Lv.50)</div>
              <div class="grid grid-cols-6 gap-1 text-center text-xs">
                <div v-for="s in EVS" :key="s.k"><div class="text-gray-400">{{ s.n }}</div><div class="text-gray-700 font-mono font-bold">{{ calcStat(s.k) }}</div></div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>

  <!-- Smogon Build Modal -->
  <SmogonBuildModal
    :visible="showSmogonModal"
    :pokemon="modalPokemon"
    :smogon-data="smogonData"
    :loading="loadingSmogon"
    :move-name-meta="moveNameMeta"
    @close="showSmogonModal = false"
    @build="oneClickBuild"
    @select-teammate="onSelectTeammate"
    @focus-tab="activeTab = $event" />
</template>

<script setup>
import { ref, reactive, computed, onMounted, watch } from 'vue'
import { TYPES } from '../utils/enums'
import { connect, request, send, getPlayerId } from '../api/wsClient'
import { trackTeamSave, setPlayerState } from '../utils/analytics'
import { useSpeciesStore } from '../stores/speciesStore'
import IconSprite from '../components/shared/IconSprite.vue'
import { ITEM_SHEET } from '../utils/itemSheet'
import { smogonAPI } from '../api/smogon'
import { frontSprite } from '../utils/spriteUrl'
import SmogonBuildModal from '../components/stats/SmogonBuildModal.vue'

const teamName = ref('Team')
const activeIdx = ref(0)
const activeTab = ref('species')
const summaryGifFailed = ref(false)
const speciesSearch = ref(''); const speciesResults = ref([])
const moveSearch = ref(''); const mvResults = ref([]); const editMoveSlot = ref(-1)
const itemSearch = ref(''); const itemResults = ref([])
const moveNamesCache = ref({}); const itemNamesCache = ref({}); const abilityNamesCache = ref({})
const moveMetaCache = ref({})  // Persistent id→{type,category,power,pp,name} — survives mvResults replacement
const savedTeams = ref([])
const teamPage = ref(0)
let timers = {}

function flipTeam(dir) {
  const n = savedTeams.value.length
  if (!n) return
  teamPage.value = dir > 0 ? (teamPage.value + 1) % n : (teamPage.value > 0 ? teamPage.value - 1 : n - 1)
  loadTeam(savedTeams.value[teamPage.value])
}

// Smogon analytics
const smogonData = ref(null)
const loadingSmogon = ref(false)
const showSmogonModal = ref(false)
const modalPokemonName = ref('')

const modalPokemon = computed(() => {
  const name = modalPokemonName.value
  if (!name) return null
  const sp = allSpeciesCache.value.find(s => s.name === name)
  return { name, speciesID: sp?.id || 0, types: sp?.types || [] }
})

async function fetchSmogonData(speciesName) {
  if (!speciesName) { smogonData.value = null; return }
  loadingSmogon.value = true
  smogonData.value = null
  try {
    // Try 1760 first, fall back to 1500, then 0
    const ratings = [1760, 1500, 0]
    let detail = null
    for (const r of ratings) {
      try {
        const res = await smogonAPI.detail(speciesName, { source: 'smogon', time_bucket: '2026-05', rating: r })
        if (res && res.info) { detail = res; break }
      } catch {}
    }
    let trend = []
    if (detail) {
      try { trend = await smogonAPI.trend(speciesName, { rating: ratings[0] }) } catch {}
    }
    smogonData.value = detail ? { ...detail, trend } : null
  } catch (e) {
    smogonData.value = null
  } finally {
    loadingSmogon.value = false
  }
}

// Normalize to Showdown-style: lowercase, no spaces/hyphens/underscores
function normName(n) { return (n || '').toLowerCase().replace(/[^a-z0-9]/g, '') }
// Display name: prefer Chinese if available
function cn(item) { return item?.chineseName || item?.name || '' }

async function oneClickBuild() {
  const d = smogonData.value
  const s = activeSlot.value
  if (!d || !s.pkm) return

  // Top 4 moves (with server fallback like items)
  const topMoves = []
  for (const smogonMove of (d.moves || []).slice(0, 4)) {
    let mv = mvResults.value.find(x => normName(x.name) === normName(smogonMove.name))
    if (!mv) {
      const found = await request('get_moves', { search: smogonMove.name, limit: 50 })
      mv = found?.find(x => normName(x.name) === normName(smogonMove.name))
      if (mv) {
        // Persist to both caches so type/category survive any mvResults replacement
        moveMetaCache.value[mv.id] = { type: mv.type, category: mv.category, power: mv.power, pp: mv.pp, name: mv.name }
        moveNamesCache.value[mv.id] = mv.chineseName || mv.name
        if (!mvResults.value.find(x => x.id === mv.id)) mvResults.value.push(mv)
      }
    }
    if (mv) topMoves.push(mv.id)
  }
  if (topMoves.length) s.pkm.moves = topMoves.slice(0, 4)

  // Top item
  if (d.items?.length) {
    let topItem = null
    for (const smogonItem of d.items.slice(0, 5)) {
      topItem = itemResults.value.find(x => normName(x.name) === normName(smogonItem.name))
      if (!topItem) {
        const found = await request('get_items', { search: smogonItem.name, limit: 5 })
        topItem = found?.find(x => normName(x.name) === normName(smogonItem.name))
        if (topItem && !itemResults.value.find(x => x.id === topItem.id)) {
          itemResults.value.push(topItem)
        }
      }
      if (topItem) break
    }
    if (topItem) s.pkm.item = topItem.id
  }

  // Top ability
  if (d.abilities?.length) {
    const sp = s._species
    if (sp) {
      const topAbName = normName(d.abilities[0].name)
      const aid = sp.abilities?.find(aid =>
        normName(abilityNamesCache.value[aid]) === topAbName
      )
      if (aid) s.pkm.ability = aid
    }
  }

  // Top spread (nature + EVs)
  if (d.spreads?.length) {
    const topSpread = d.spreads[0]
    const natureObj = NATURES.find(n => normName(n.name) === normName(topSpread.nature))
    if (natureObj) s.pkm.nature = natureObj.v
    // Parse EVs like "0/252/0/0/4/252"
    const evParts = topSpread.evs.split('/').map(Number)
    if (evParts.length === 6) {
      const keys = ['hp', 'atk', 'def', 'spa', 'spd', 'spe']
      keys.forEach((k, i) => { s.pkm.evs[k] = evParts[i] || 0 })
    }
  }

  // Refresh display — fetch full move info if not in mvResults
  const uncachedMoves = (s.pkm.moves || []).filter(mid => mid && !mvResults.value.find(m => m.id === mid))
  if (uncachedMoves.length) {
    const loaded = await Promise.all(uncachedMoves.map(mid =>
      request('get_move', { id: mid }).then(r => {
        moveNamesCache.value[mid] = r?.chineseName || r?.name || '#' + mid
        moveMetaCache.value[mid] = { type: r?.type, category: r?.category, power: r?.power, pp: r?.pp, name: r?.name }
        return r ? { id: mid, name: r.name, type: r.type, category: r.category, power: r.power, pp: r.pp, chineseName: r.chineseName } : null
      })
    ))
    loaded.filter(Boolean).forEach(m => { if (m && !mvResults.value.find(x => x.id === m.id)) mvResults.value.push(m) })
  }
  s._moveNames = (s.pkm.moves || []).map(mid => moveNamesCache.value[mid] || '#' + mid)
  const it = itemResults.value.find(x => x.id === s.pkm.item)
  s._itemName = it?.chineseName || it?.name || ''
}

function addTeammate(mateName) {
  // Find empty slot
  const emptyIdx = slots.findIndex(s => !s.pkm)
  if (emptyIdx < 0) return
  // Find species by name
  const sp = allSpeciesCache.value.find(s =>
    s.name.toLowerCase() === mateName.toLowerCase()
  )
  if (sp) {
    activeIdx.value = emptyIdx
    pickSpecies(sp)
  }
}
function onSelectTeammate(mateName) {
  // Find the species in cache
  const sp = allSpeciesCache.value.find(s => s.name.toLowerCase() === mateName.toLowerCase())
  if (!sp) {
    // Species not found — just switch modal data
    modalPokemonName.value = mateName
    fetchSmogonData(mateName)
    return
  }
  // Find next empty slot
  const emptyIdx = slots.findIndex(s => !s.pkm)
  if (emptyIdx >= 0) {
    activeIdx.value = emptyIdx
    pickSpecies(sp)
  }
  // Keep modal open with teammate's data
  modalPokemonName.value = sp.name
  fetchSmogonData(sp.name)
}

const EVS = [{k:'hp',n:'HP'},{k:'atk',n:'Atk'},{k:'def',n:'Def'},{k:'spa',n:'SpA'},{k:'spd',n:'SpD'},{k:'spe',n:'Spe'}]
const NATURES = [{v:3,name:'Adamant',desc:'+Atk -SpA'},{v:10,name:'Timid',desc:'+Spe -Atk'},{v:13,name:'Jolly',desc:'+Spe -SpA'},{v:15,name:'Modest',desc:'+SpA -Atk'},{v:23,name:'Careful',desc:'+SpD -SpA'},{v:5,name:'Bold',desc:'+Def -Atk'},{v:8,name:'Impish',desc:'+Def -SpA'},{v:0,name:'Hardy',desc:'修正なし'},{v:1,name:'Lonely',desc:'+Atk -Def'},{v:2,name:'Brave',desc:'+Atk -Spe'},{v:4,name:'Naughty',desc:'+Atk -SpD'},{v:7,name:'Relaxed',desc:'+Def -Spe'},{v:9,name:'Lax',desc:'+Def -SpD'},{v:11,name:'Hasty',desc:'+Spe -Def'},{v:14,name:'Naive',desc:'+Spe -SpD'},{v:16,name:'Mild',desc:'+SpA -Def'},{v:17,name:'Quiet',desc:'+SpA -Spe'},{v:19,name:'Rash',desc:'+SpA -SpD'},{v:20,name:'Calm',desc:'+SpD -Atk'},{v:21,name:'Gentle',desc:'+SpD -Def'},{v:22,name:'Sassy',desc:'+SpD -Spe'}]
const tabs = [{key:'species',label:'🔍 宝可梦'},{key:'moves',label:'⚔️ 技能'},{key:'items',label:'🎒 道具'},{key:'details',label:'📊 详情'}]

const slots = reactive(Array.from({length:6}, () => ({ pkm: null, _name: '', _types: [], _species: null, _moveNames: [], _itemName: '' })))
const activeSlot = computed(() => slots[activeIdx.value])
// Move name→meta lookup for Smogon modal — normName → {type, category, power, pp, chineseName}
const moveNameMeta = computed(() => {
  const map = {}
  for (const [id, m] of Object.entries(moveMetaCache.value)) {
    if (m.name) map[normName(m.name)] = { type: m.type, category: m.category, power: m.power, pp: m.pp, name: m.name, id: Number(id) }
  }
  return map
})
const dragIdx = ref(-1)
const dragOverIdx = ref(-1)
const evTotal = computed(() => activeSlot.value.pkm?.evs ? Object.values(activeSlot.value.pkm.evs).reduce((a,b)=>a+b,0) : 0)
function clampEV(key) {
  if (!activeSlot.value.pkm?.evs) return
  let v = activeSlot.value.pkm.evs[key]
  if (isNaN(v) || v < 0) v = 0
  if (v > 255) v = 255
  // Enforce 508 total cap
  const others = Object.entries(activeSlot.value.pkm.evs).reduce((s, [k, val]) => k === key ? s : s + (parseInt(val)||0), 0)
  if (v + others > 508) v = Math.max(0, 508 - others)
  activeSlot.value.pkm.evs[key] = v
}

// -- Species sheet state --
const allSpeciesCache = ref([])
const speciesSortKey = ref('id')
const speciesSortAsc = ref(true)
const smogonUsage = ref({})  // name_lower → usage_pct
const statCols = [
  { k: 'hp', n: 'HP' },
  { k: 'atk', n: 'Atk' },
  { k: 'def', n: 'Def' },
  { k: 'spa', n: 'SpA' },
  { k: 'spd', n: 'SpD' },
  { k: 'spe', n: 'Spe' },
]

const speciesFiltered = computed(() => {
  const q = speciesSearch.value.toLowerCase().trim()
  // Deduplicate by ID
  const seen = new Set()
  const src = allSpeciesCache.value.filter(s => {
    if (seen.has(s.id)) return false
    seen.add(s.id)
    return true
  })
  if (!q) return src
  return src.filter(s => {
    const key = `${s.id} ${s.name} ${(s.types||[]).join(' ')}`.toLowerCase()
    return key.includes(q)
  })
})

const speciesSorted = computed(() => {
  const arr = [...speciesFiltered.value]
  const k = speciesSortKey.value
  arr.sort((a, b) => {
    let va, vb
    if (k === 'id') { va = a.id; vb = b.id }
    else if (k === 'name') { va = a.name || ''; vb = b.name || '' }
    else if (k === 'bst') { va = (a.baseStats||[]).reduce((x,y)=>x+y,0); vb = (b.baseStats||[]).reduce((x,y)=>x+y,0) }
    else if (k === 'pickrate') {
      va = smogonUsage.value[a.name.toLowerCase()] || 0
      vb = smogonUsage.value[b.name.toLowerCase()] || 0
    }
    else {
      const i = statCols.findIndex(s => s.k === k)
      va = (a.baseStats||[])[i] ?? 0
      vb = (b.baseStats||[])[i] ?? 0
    }
    if (typeof va === 'string') return speciesSortAsc.value ? va.localeCompare(vb) : vb.localeCompare(va)
    const d = va - vb
    return speciesSortAsc.value ? d : -d
  })
  return arr
})

function toggleSpeciesSort(key) {
  if (speciesSortKey.value === key) { speciesSortAsc.value = !speciesSortAsc.value; return }
  speciesSortKey.value = key
  speciesSortAsc.value = key === 'pickrate' || key === 'bst' ? false : true
}
function sortArrow(key) {
  if (speciesSortKey.value !== key) return ''
  return speciesSortAsc.value ? ' ↑' : ' ↓'
}
function fmtPickrate(v) { return v ? v.toFixed(1) + '%' : '-' }
function pickColor(v) {
  if (!v) return 'text-gray-300'
  if (v >= 15) return 'text-rose-600 font-bold'
  if (v >= 5) return 'text-orange-500'
  return 'text-gray-400'
}
function statColor(k, v) {
  if (v == null) return ''
  if (k === 'hp' && v >= 100) return 'text-green-600 font-bold'
  if (v >= 130) return 'text-rose-600 font-bold'
  if (v >= 100) return 'text-orange-600'
  if (v >= 70) return 'text-gray-600'
  return 'text-gray-400'
}

function capitalize(s) { return s ? s.charAt(0).toUpperCase()+s.slice(1).toLowerCase() : '' }
function moveType(mid) {
  const cached = moveMetaCache.value[mid]
  if (cached) return cached.type || 'Normal'
  const mv = mvResults.value.find(m=>m.id===mid); return mv?.type||'Normal'
}
function moveCat(mid) {
  const cached = moveMetaCache.value[mid]
  if (cached) { const c = cached.category || 'status'; return c.charAt(0).toUpperCase()+c.slice(1) }
  const mv = mvResults.value.find(m=>m.id===mid)
  const c = mv?.category||'status'
  return c.charAt(0).toUpperCase()+c.slice(1)
}
function movePower(mid) {
  const cached = moveMetaCache.value[mid]
  if (cached) return cached.power||0
  const mv = mvResults.value.find(m=>m.id===mid); return mv?.power||0
}
function movePP(mid) {
  const cached = moveMetaCache.value[mid]
  if (cached) return cached.pp||0
  const mv = mvResults.value.find(m=>m.id===mid); return mv?.pp||0
}
function isValidType(t) { return t !== undefined && t !== null && t !== '' && TYPES[t] !== undefined }
function typeColor(n) {
  if (!n) return '#666'
  const nl = String(n).toLowerCase()
  for (const t of Object.values(TYPES)) { if (t.name.toLowerCase()===nl) return t.color }
  return '#666'
}
function typeLabel(t) {
  const found = Object.values(TYPES).find(x => x.name && x.name.toLowerCase() === String(t).toLowerCase())
  return found ? found.label : t
}
function natureName(v) { const n = NATURES.find(x=>x.v===v); return n ? n.name : '性格' }
function natureDesc(v) { const n = NATURES.find(x=>x.v===v); return n ? n.desc : '' }
function catColor(c) { if (c==='Physical') return '#E05D3B'; if (c==='Special') return '#4A7EB5'; return '#8B8B8B' }
function itemImgName(id) { const it = itemResults.value.find(x=>x.id===id); return it?.name||'' }
function itemDesc(id) { const it = itemResults.value.find(x=>x.id===id); return it?.description||'' }
function itemSpriteStyle(name) {
  if (!name) return { background: '#f3f4f6', borderRadius: '4px' }
  const sk = name.toLowerCase().replace(/[^a-z0-9]/g, '')
  const hk = name.toLowerCase().replace(/\s+/g, '-').replace(/['.]/g, '')
  const pos = ITEM_SHEET.mapping[sk] || ITEM_SHEET.mapping[hk] || ITEM_SHEET.mapping[name]
  if (!pos) return { background: '#f3f4f6', borderRadius: '4px' }
  return {
    backgroundImage: `url(${ITEM_SHEET.url})`,
    backgroundPosition: pos,
    backgroundSize: `${ITEM_SHEET.cols * ITEM_SHEET.size}px auto`,
    imageRendering: 'pixelated',
  }
}

function selectSlot(idx) {
  activeIdx.value = idx
  if (slots[idx].pkm) { activeTab.value = 'moves'; loadSlotData() }
  else { activeTab.value = 'species' }
}
function onDragStart(idx, e) {
  dragIdx.value = idx
  e.dataTransfer.effectAllowed = 'move'
  e.dataTransfer.setData('text/plain', String(idx))
}
function onDragOver(idx) { dragOverIdx.value = idx }
function onDragLeave() { dragOverIdx.value = -1 }
function onDrop(idx) {
  dragOverIdx.value = -1
  const src = dragIdx.value
  if (src < 0 || src === idx) return
  // Swap the two slots
  const tmp = { ...slots[src] }
  Object.assign(slots[src], slots[idx])
  Object.assign(slots[idx], tmp)
  // Update active index to follow the moved pokemon
  if (activeIdx.value === src) activeIdx.value = idx
  else if (activeIdx.value === idx) activeIdx.value = src
}
function onDragEnd() { dragIdx.value = -1; dragOverIdx.value = -1 }
function removePokemon() {
  const s = slots[activeIdx.value]; s.pkm = null; s._name = ''; s._cnName = ''; s._types = []; s._species = null; s._moveNames = []; s._itemName = ''
  activeTab.value = 'species'
}
function removeSlot(idx) {
  const s = slots[idx]; s.pkm = null; s._name = ''; s._cnName = ''; s._types = []; s._species = null; s._moveNames = []; s._itemName = ''
  if (activeIdx.value === idx) activeTab.value = 'species'
}
function openBuildModal(idx) {
  selectSlot(idx)
  if (slots[idx].pkm && slots[idx]._species) {
    fetchSmogonData(slots[idx]._species.name)
    modalPokemonName.value = slots[idx]._species.name
    showSmogonModal.value = true
  }
}

watch(activeIdx, () => { loadSlotData(); summaryGifFailed.value = false }, { immediate: true })
watch(() => activeSlot.value.pkm?.speciesID, () => { summaryGifFailed.value = false })
watch(activeTab, (tab) => {
  if (tab === 'items' && !itemResults.value.length) loadItems()
})
function loadSlotData() {
  let sp = activeSlot.value._species
  // Try to find species from results if _species is null
  if (!sp && activeSlot.value.pkm?.speciesID) {
    sp = allSpeciesCache.value.find(s => s.id === activeSlot.value.pkm.speciesID)
    if (sp) activeSlot.value._species = sp
  }
  if (sp && (sp.learnableMoves||[]).length > 0) {
    request('get_moves',{learnset:sp.learnableMoves,limit:200}).then(r=>{
      mvResults.value = r
      // Persist move metadata so types survive mvResults replacement
      for (const m of r) {
        moveMetaCache.value[m.id] = { type: m.type, category: m.category, power: m.power, pp: m.pp, name: m.name }
        moveNamesCache.value[m.id] = m.chineseName || m.name
      }
    })
  } else {
    mvResults.value = []
  }
}
function loadItems() {
  request('get_items',{search:'',limit:200}).then(r=>{itemResults.value=r})
}

onMounted(async () => {
  setPlayerState('teambuilding')
  await connect('TeamBuilder')
  const { speciesList, load } = useSpeciesStore()
  if (!speciesList.value.length) await load()
  allSpeciesCache.value = speciesList.value
  speciesResults.value = allSpeciesCache.value
  await loadSavedTeams()
  // Preload items + Smogon usage for pick rate column
  request('get_items',{search:'',limit:200}).then(r=>{itemResults.value=r})
  smogonAPI.ranking({source:'smogon',time_bucket:'2026-05',rating:1760,limit:200}).then(list => {
    const map = {}
    list.forEach(item => { map[item.name.toLowerCase()] = item.usage * 100 })
    smogonUsage.value = map
  }).catch(() => {})
})

function pickSpecies(sp) {
  const s = slots[activeIdx.value]
  // For form aliases, use base species ID for game mechanics but keep alias for display
  const gameId = sp.baseSpeciesId || sp.id
  s.pkm = { speciesID: gameId, level: 50, ability: (sp.abilities && sp.abilities[0]) || 0, nature: 3, moves: [], item: 0, evs: {hp:0,atk:0,def:0,spa:0,spd:0,spe:0} }
  s._name = sp.chineseName || sp.name; s._cnName = sp.chineseName || sp.name
  s._types = sp.types||[]; s._species = sp; s._moveNames = []; s._itemName = ''
  s._spriteId = sp.id  // Use alias ID for sprite
  activeTab.value = 'moves'
  const ls = sp.learnableMoves||[]
  if (ls.length) request('get_moves',{learnset:ls,limit:200}).then(r=>{
    mvResults.value = r
    for (const m of r) {
      moveMetaCache.value[m.id] = { type: m.type, category: m.category, power: m.power, pp: m.pp, name: m.name }
      moveNamesCache.value[m.id] = m.chineseName || m.name
    }
  })
  const allAb = [...(sp.abilities||[]), sp.hiddenAbilityID].filter(Boolean)
  allAb.forEach(aid => request('get_ability',{id:aid}).then(r=>{abilityNamesCache.value[aid]=r?.chineseName||r?.name||'#'+aid}))
  request('get_items',{search:'',limit:200}).then(r=>{itemResults.value=r})
  fetchSmogonData(sp.name)
  modalPokemonName.value = sp.name
  showSmogonModal.value = true
}

async function onMoveSearch() {
  clearTimeout(timers.mv); timers.mv = setTimeout(async () => {
    const ls = activeSlot.value._species?.learnableMoves||[]
    try {
      const r = await request('get_moves',{search:moveSearch.value,learnset:ls,limit:200})
      mvResults.value = r
      for (const m of r) {
        moveMetaCache.value[m.id] = { type: m.type, category: m.category, power: m.power, pp: m.pp, name: m.name }
        moveNamesCache.value[m.id] = m.chineseName || m.name
      }
    } catch { mvResults.value = [] }
  }, 150)
}
function toggleMove(mv) {
  const slot = slots[activeIdx.value]
  if (!slot.pkm) return
  moveNamesCache.value[mv.id] = mv.name
  const idx = editMoveSlot.value
  if (idx >= 0) {
    // Replace specific slot
    const m = [...slot.pkm.moves]
    // Remove if already in another slot
    const existing = m.indexOf(mv.id)
    if (existing >= 0) m.splice(existing, 1)
    m[idx] = mv.id
    slot.pkm.moves = m.filter(Boolean)
    editMoveSlot.value = -1
  } else {
    const m = [...slot.pkm.moves]
    if (m.includes(mv.id)) {
      slot.pkm.moves = m.filter(x=>x!==mv.id)
    } else if (m.length >= 4) {
      return
    } else {
      slot.pkm.moves = [...m, mv.id]
    }
  }
  slot._moveNames = slot.pkm.moves.map(mid => moveNamesCache.value[mid]||'')
}

async function onItemSearch() {
  clearTimeout(timers.it); timers.it = setTimeout(async () => {
    try { itemResults.value = await request('get_items',{search:itemSearch.value,limit:200}) } catch { itemResults.value = [] }
  }, 150)
}
function selectItem(it) {
  if (activeSlot.value.pkm.item === it.id) {
    activeSlot.value.pkm.item = 0; activeSlot.value._itemName = ''
  } else {
    activeSlot.value.pkm.item = it.id; activeSlot.value._itemName = it.name
  }
  itemNamesCache.value[it.id] = it.chineseName || it.name
}

function calcStat(k) {
  const p = activeSlot.value.pkm; const sp = activeSlot.value._species
  if (!p||!sp) return '?'
  const b = ['hp','atk','def','spa','spd','spe'].indexOf(k)
  const base = sp.baseStats?.[b]||0; const iv=31; const ev=p.evs?.[k]||0
  let s
  if (k==='hp') s = Math.floor((2*base+iv+Math.floor(ev/4))*50/100)+60
  else { s = Math.floor((2*base+iv+Math.floor(ev/4))*50/100)+5
    const nm = {3:{atk:1.1,spa:0.9},10:{spe:1.1,atk:0.9},13:{spe:1.1,spa:0.9},15:{spa:1.1,atk:0.9},23:{spd:1.1,spa:0.9},5:{def:1.1,atk:0.9},8:{def:1.1,spa:0.9}}[p.nature||3]
    if (nm) { if (nm[k]===1.1) s=Math.floor(s*1.1); else if (nm[k]===0.9) s=Math.floor(s*0.9) }
  }
  return s
}

async function loadSavedTeams() {
  const uid = getPlayerId(); if (!uid) return
  try { savedTeams.value = await request('get_user_teams',{user_id:uid}) } catch { savedTeams.value = [] }
}
async function deleteTeam(name) { send('delete_team',{username:getPlayerId(),team_name:name}); loadSavedTeams() }
async function loadTeam(t) {
  teamName.value = t.name
  slots.forEach(s => { s.pkm = null; s._name = ''; s._types = []; s._species = null; s._moveNames = []; s._itemName = '' })
  const promises = []
  t.pokemon.forEach((p,i) => {
    if (i>=6) return
    const sp = allSpeciesCache.value.find(s=>s.id===p.speciesID)
    slots[i].pkm = { speciesID: p.speciesID, level: 50, ability: p.ability||0, nature: p.nature||3, moves: p.moves||[], item: p.item||0, evs: p.evs||{hp:0,atk:0,def:0,spa:0,spd:0,spe:0} }
    slots[i]._spriteId = sp?.id || p.speciesID  // use species cache ID for sprite mapping
    slots[i]._name = sp?.chineseName || sp?.name||'#'+p.speciesID
    slots[i]._cnName = slots[i]._name; slots[i]._types = sp?.types||[]; slots[i]._species = sp||null
    if (p.moves) p.moves.forEach(mid => {
      if (mid) promises.push(request('get_move',{id:mid}).then(r=>{
        moveNamesCache.value[mid] = r?.chineseName||r?.name||'#'+mid
        moveMetaCache.value[mid] = { type: r?.type, category: r?.category, power: r?.power, pp: r?.pp, name: r?.name }
        slots[i]._moveNames = (slots[i].pkm.moves||[]).map(mid=>moveNamesCache.value[mid]||'?')
      }))
    })
    if (p.item) promises.push(request('get_items',{search:''}).then(r=>{ const it = r.find(x=>x.id===p.item); if (it) { slots[i]._itemName = it.chineseName||it.name; itemNamesCache.value[p.item] = it.chineseName||it.name } }))
    if (sp) {
      const allAb = [...(sp.abilities||[]), sp.hiddenAbilityID].filter(Boolean)
      allAb.forEach(aid => promises.push(request('get_ability',{id:aid}).then(r=>{abilityNamesCache.value[aid]=r?.chineseName||r?.name||'#'+aid})))
    }
  })
  await Promise.all(promises)
  activeIdx.value = 0; activeTab.value = 'moves'
  // Load learnset for the active slot only (not all slots)
  loadSlotData()
}
function saveTeam() {
  const pokemon = slots.filter(s=>s.pkm).map(s=>({speciesID:s.pkm.speciesID, level:50,ability:s.pkm.ability||0,nature:s.pkm.nature||3,moves:(s.pkm.moves||[]).filter(m=>m!==0),item:s.pkm.item||0,evs:s.pkm.evs||{hp:0,atk:0,def:0,spa:0,spd:0,spe:0}}))
  if (!pokemon.length) return
  trackTeamSave(teamName.value, pokemon)
  send('save_team',{user_id:getPlayerId(),name:teamName.value,pokemon})
  setTimeout(loadSavedTeams, 500)
}
</script>
