<template>
  <Teleport to="body">
    <div v-if="visible" class="fixed inset-0 z-50 flex items-start justify-center pt-[10vh]"
      @click.self="$emit('close')">
      <!-- Backdrop -->
      <div class="absolute inset-0 bg-black/30 backdrop-blur-sm"></div>

      <!-- Panel -->
      <div class="relative bg-white rounded-2xl shadow-2xl border border-gray-200 w-[800px] max-h-[80vh] overflow-hidden flex flex-col animate-pop">
        <!-- Close button -->
        <button @click="$emit('close')"
          class="absolute top-3 right-3 z-10 w-7 h-7 flex items-center justify-center rounded-full bg-gray-100 hover:bg-gray-200 text-gray-400 hover:text-gray-600 transition-colors text-sm">
          ✕
        </button>

        <!-- Loading -->
        <div v-if="loading" class="flex items-center justify-center py-16">
          <div class="animate-spin w-6 h-6 border-2 border-indigo-400 border-t-transparent rounded-full mr-2"></div>
          <span class="text-sm text-gray-400">加载 Smogon 数据...</span>
        </div>

        <!-- No data -->
        <div v-else-if="!smogonData" class="py-12 text-center">
          <div class="text-gray-300 text-lg mb-2">暂无数据</div>
          <div class="text-sm text-gray-400">Smogon 暂无该宝可梦在 Gen 9 1v1 中的数据</div>
        </div>

        <!-- Content -->
        <template v-else>
          <!-- === TOP: Same as TeamBuilder summary bar === -->
          <div class="px-4 py-3 border-b border-gray-100 shrink-0 flex items-start gap-3 flex-wrap bg-gradient-to-b from-white to-gray-50/50">
            <!-- Left: name + sprite (stacked) -->
            <div class="flex flex-col items-center shrink-0 w-28">
              <div class="text-sm font-bold text-gray-800 text-center truncate w-full">{{ props.pokemon?.name || '?' }}</div>
              <div class="text-xs text-gray-400">#{{ props.pokemon?.speciesID || '?' }}</div>
              <div class="w-28 h-28 mt-0.5">
                <img :src="spriteUrl(props.pokemon?.speciesID || 0)"
                  @error="spriteFallback"
                  class="w-24 h-24 object-contain drop-shadow-md" />
              </div>
              <div class="flex gap-0.5 mt-0.5">
                <img v-for="t in (props.pokemon?.types||[])" :key="t" :src="'/types/'+capitalize(t)+'.png'" class="h-4 w-auto" />
              </div>
            </div>

            <!-- Center: 2x2 move grid -->
            <div class="grid grid-cols-2 gap-2 w-[320px] shrink-0">
              <button v-for="i in 4" :key="'m'+i" @click="$emit('focusTab', 'moves')"
                class="text-sm border-2 transition-all duration-150 p-3 h-[68px] text-left"
                style="border-radius: 4px"
                :class="topMoves[i-1] ? 'bg-white border-gray-200 hover:border-blue-400 hover:shadow-inner' : 'bg-gray-50 border-dashed border-gray-250 text-gray-400 hover:border-gray-350 hover:bg-white'">
                <template v-if="topMoves[i-1]">
                  <div class="flex items-center gap-1.5 mb-1">
                    <img :src="'/types/'+capitalize(topMoves[i-1].type||'Normal')+'.png'" class="h-4 w-auto" />
                    <span class="ml-auto text-[10px] text-gray-400 font-mono">{{ topMoves[i-1].power||'-' }}·{{ topMoves[i-1].pp||'-' }}</span>
                  </div>
                  <div class="text-gray-700 font-semibold text-xs truncate">{{ topMoves[i-1].chinese_name || fmtName(topMoves[i-1].name) }}</div>
                </template>
                <template v-else>
                  <div class="flex items-center justify-center h-full text-gray-350 text-sm">招式 {{ i }}</div>
                </template>
              </button>
            </div>

            <!-- Right: Item + Details stacked -->
            <div class="flex flex-col gap-2 shrink-0 w-[150px]">
              <button @click="$emit('focusTab', 'items')"
                class="text-sm border-2 transition-all duration-150 p-3 text-left h-[68px]"
                style="border-radius: 4px"
                :class="topItem ? 'bg-white border-gray-200 hover:border-amber-400 hover:shadow-inner' : 'bg-gray-50 border-dashed border-gray-250 text-gray-400 hover:border-gray-350'">
                <template v-if="topItem">
                  <div class="flex items-center gap-2">
                    <div :style="itemSpriteStyle(topItem.name)" class="w-6 h-6 shrink-0 rounded" />
                    <span class="text-amber-700 font-semibold text-xs truncate">{{ topItem.chinese_name || fmtName(topItem.name) }}</span>
                  </div>
                </template>
                <template v-else>
                  <div class="flex items-center justify-center h-full text-gray-350 text-sm">道具</div>
                </template>
              </button>
              <button @click="$emit('focusTab', 'details')"
                class="text-sm border-2 transition-all duration-150 p-3 text-left h-[68px]"
                style="border-radius: 4px"
                :class="topSpread ? 'bg-white border-gray-200 hover:border-purple-300 hover:shadow-inner' : 'bg-gray-50 border-dashed border-gray-250 text-gray-400'">
                <template v-if="topSpread">
                  <div class="text-gray-700 font-semibold text-xs">{{ topSpread.nature }}</div>
                  <div class="text-gray-400 text-[11px] mt-0.5 font-mono">{{ topSpread.evs }}</div>
                </template>
                <template v-else>
                  <div class="flex items-center justify-center h-full text-gray-350 text-sm">详情</div>
                </template>
              </button>
            </div>

            <!-- One-click Build -->
            <button @click="$emit('build'); $emit('close')"
              class="px-3 py-2 bg-indigo-500 hover:bg-indigo-600 text-white rounded-xl text-xs font-bold transition-colors shadow-sm shrink-0 self-center">
              一键<br>配置
            </button>
          </div>

          <!-- === BOTTOM: Recommended Teammates === -->
          <div class="p-4 bg-gray-50/50 flex-1 overflow-y-auto">
            <div class="text-xs text-gray-500 font-bold mb-3 flex items-center gap-1">
              <span>推荐队友</span>
              <span class="text-indigo-400 text-[10px] font-normal">Smogon</span>
            </div>
            <div class="flex gap-3 overflow-x-auto pb-2">
              <div v-for="tm in smogonData.teammates?.slice(0, 10)" :key="tm.mate"
                @click="$emit('selectTeammate', tm.mate)"
                class="shrink-0 w-[140px] bg-white rounded-xl border border-gray-200 p-2.5 cursor-pointer hover:border-indigo-300 hover:shadow-sm transition-all group">
                <!-- Mini GIF -->
                <div class="w-12 h-12 mx-auto mb-1">
                  <img :src="miniSpriteUrl(tm.mate)" :data-name="tm.mate"
                    @error="miniFallback"
                    class="w-12 h-12 object-contain" />
                </div>
                <!-- Name -->
                <div class="text-[11px] text-gray-700 font-medium text-center truncate">
                  {{ tm.chinese_name || tm.mate }}
                </div>
                <div class="text-[10px] text-gray-400 text-center truncate">{{ tm.mate }}</div>
                <!-- Usage -->
                <div class="flex items-center gap-1 mt-1.5 justify-center">
                  <div class="w-10 h-1 bg-gray-100 rounded-full overflow-hidden">
                    <div class="h-full bg-indigo-400 rounded-full"
                      :style="{ width: Math.round(tm.usage * 100) + '%' }"></div>
                  </div>
                  <span class="text-[10px] text-gray-400 font-mono">{{ Math.round(tm.usage * 100) }}%</span>
                </div>
              </div>
              <!-- No teammates -->
              <div v-if="!smogonData.teammates?.length" class="text-xs text-gray-400 py-4 text-center w-full">
                暂无推荐队友数据
              </div>
            </div>
          </div>
        </template>
      </div>
    </div>
  </Teleport>
</template>

<script setup>
import { ref, computed } from 'vue'

const props = defineProps({
  visible: { type: Boolean, default: false },
  pokemon: { type: Object, default: null },
  smogonData: { type: Object, default: null },
  loading: { type: Boolean, default: false },
})
defineEmits(['close', 'build', 'selectTeammate', 'focusTab'])

const imgFailed = ref(false)

const topMoves = computed(() => (props.smogonData?.moves || []).slice(0, 4))
const topItem = computed(() => props.smogonData?.items?.[0] || null)
const topSpread = computed(() => props.smogonData?.spreads?.[0] || null)

import { frontSprite, cdnFrontSprite, cdnGen5Sprite, spriteFallback } from '../../utils/spriteUrl'
import { ITEM_SHEET } from '../../utils/itemSheet'
import { request } from '../../api/wsClient'

// Cache for looking up move info from game data
const moveInfoCache = ref({})

function getMoveInfo(name) {
  if (!name) return {}
  const key = name.toLowerCase()
  if (!moveInfoCache.value[key]) {
    // Lazily fetch via WebSocket
    request('get_moves', { search: name, limit: 1 }).then(list => {
      if (list?.[0]) moveInfoCache.value[key] = list[0]
    }).catch(() => {})
    return {}
  }
  return moveInfoCache.value[key]
}

/** Format Showdown name: dragonpulse → Dragon Pulse */
function fmtName(n) {
  if (!n) return ''
  return n.replace(/-/g, ' ').replace(/\b\w/g, c => c.toUpperCase())
}
function capitalize(s) { return s ? s.charAt(0).toUpperCase()+s.slice(1).toLowerCase() : '' }

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

function spriteUrl(speciesId) {
  return frontSprite(speciesId)
}

function miniSpriteUrl(name) {
  return cdnFrontSprite(name)
}

function miniFallback(e) {
  const el = e.target
  const name = el.getAttribute('data-name') || ''
  if (!el._fbStep) { el._fbStep = 1; el.src = cdnGen5Sprite(name); return }
  el.style.display = 'none'
}

</script>

<style scoped>
.animate-pop {
  animation: popIn 0.2s ease-out;
}
@keyframes popIn {
  from { transform: scale(0.95) translateY(-10px); opacity: 0; }
  to { transform: scale(1) translateY(0); opacity: 1; }
}
</style>
