<template>
  <Teleport to="body">
    <div v-if="visible" class="fixed inset-0 z-50 flex">
      <!-- Backdrop -->
      <div class="absolute inset-0 bg-black/40 backdrop-blur-sm" @click="$emit('close')"></div>

      <!-- Panel -->
      <div class="relative ml-auto w-full max-w-xl bg-white shadow-2xl h-full overflow-hidden flex flex-col animate-slide-in">
        <!-- Header -->
        <div class="px-5 py-4 border-b border-gray-100 flex items-center gap-3 shrink-0">
          <div class="flex items-center gap-3 flex-1 min-w-0">
            <img :src="spriteUrl(info?.name)" :data-name="info?.name" @error="onImgError"
              class="w-12 h-12 object-contain shrink-0" />
            <div class="min-w-0">
              <div class="text-base font-bold text-gray-800 truncate">{{ info?.chinese_name || info?.name }}</div>
              <div class="text-xs text-gray-400">{{ info?.name }}</div>
            </div>
          </div>
          <div class="flex items-center gap-3 shrink-0">
            <div class="text-right">
              <div class="text-sm font-bold text-rose-500">{{ usageStr }}</div>
              <div class="text-xs text-gray-400">使用率</div>
            </div>
            <div class="text-right">
              <div class="text-sm font-bold text-purple-500">{{ info?.viability_ceiling?.toFixed(1) || '-' }}</div>
              <div class="text-xs text-gray-400">VC</div>
            </div>
            <button @click="$emit('close')" class="text-gray-400 hover:text-gray-600 text-xl leading-none ml-2">&times;</button>
          </div>
        </div>

        <!-- Tabs -->
        <div class="flex border-b border-gray-100 shrink-0 overflow-x-auto">
          <button v-for="tab in tabs" :key="tab.key"
            @click="activeTab = tab.key"
            class="px-3 py-2 text-xs font-medium whitespace-nowrap transition-all border-b-2"
            :class="activeTab === tab.key ? 'text-rose-400 border-rose-400' : 'text-gray-400 border-transparent hover:text-gray-600'">
            {{ tab.label }}
          </button>
        </div>

        <!-- Content -->
        <div class="flex-1 overflow-y-auto p-4">
          <!-- Loading -->
          <div v-if="loading" class="flex items-center justify-center py-16">
            <div class="animate-spin w-6 h-6 border-2 border-rose-400 border-t-transparent rounded-full"></div>
          </div>

          <!-- Overview -->
          <div v-else-if="activeTab === 'overview'" class="space-y-4">
            <div class="grid grid-cols-2 gap-3">
              <div class="bg-gray-50 rounded-xl p-3 text-center">
                <div class="text-2xl font-bold text-rose-500">{{ usageStr }}</div>
                <div class="text-xs text-gray-400">使用率</div>
              </div>
              <div class="bg-gray-50 rounded-xl p-3 text-center">
                <div class="text-2xl font-bold text-purple-500">{{ info?.viability_ceiling?.toFixed(1) || '-' }}</div>
                <div class="text-xs text-gray-400">Viability Ceiling</div>
              </div>
            </div>
            <!-- Trend line -->
            <div v-if="trendData.length" class="bg-gray-50 rounded-xl p-3">
              <div class="text-xs text-gray-500 mb-2">使用率趋势 ({{ trendData.length }} 个月)</div>
              <div class="flex items-end gap-1 h-24">
                <div v-for="t in trendData" :key="t.time_bucket"
                  class="flex-1 bg-rose-400 rounded-t hover:bg-rose-500 transition-colors cursor-pointer relative group"
                  :style="{ height: trendHeight(t.usage_pct) + '%' }"
                  :title="`${t.time_bucket}: ${t.usage_pct}%`">
                  <span class="absolute -top-5 left-1/2 -translate-x-1/2 text-[10px] text-gray-500 opacity-0 group-hover:opacity-100 whitespace-nowrap">{{ t.usage_pct }}%</span>
                </div>
              </div>
              <div class="flex justify-between mt-1">
                <span class="text-[9px] text-gray-400">{{ trendData[0]?.time_bucket }}</span>
                <span class="text-[9px] text-gray-400">{{ trendData[trendData.length-1]?.time_bucket }}</span>
              </div>
            </div>
          </div>

          <!-- Section: list of items with usage bars -->
          <div v-else-if="activeTab === 'moves'" class="space-y-2">
            <div v-for="m in detail?.moves?.slice(0, 15)" :key="m.name"
              class="flex items-center gap-3 p-2 rounded-lg hover:bg-gray-50">
              <span class="text-xs text-gray-600 font-medium flex-1 truncate">{{ m.chinese_name || m.name }}</span>
              <div class="w-20 h-1.5 bg-gray-100 rounded-full overflow-hidden shrink-0">
                <div class="h-full bg-rose-400 rounded-full" :style="{ width: pct(m.usage) + '%' }"></div>
              </div>
              <span class="text-xs text-gray-400 font-mono w-12 text-right shrink-0">{{ pct(m.usage) }}%</span>
            </div>
            <div v-if="!detail?.moves?.length" class="text-center py-8 text-gray-400 text-sm">暂无数据</div>
          </div>

          <div v-else-if="activeTab === 'items'" class="space-y-2">
            <div v-for="it in detail?.items?.slice(0, 15)" :key="it.name"
              class="flex items-center gap-3 p-2 rounded-lg hover:bg-gray-50">
              <span class="text-xs text-amber-700 font-medium flex-1 truncate">{{ it.chinese_name || it.name }}</span>
              <div class="w-20 h-1.5 bg-gray-100 rounded-full overflow-hidden shrink-0">
                <div class="h-full bg-amber-400 rounded-full" :style="{ width: pct(it.usage) + '%' }"></div>
              </div>
              <span class="text-xs text-gray-400 font-mono w-12 text-right shrink-0">{{ pct(it.usage) }}%</span>
            </div>
            <div v-if="!detail?.items?.length" class="text-center py-8 text-gray-400 text-sm">暂无数据</div>
          </div>

          <div v-else-if="activeTab === 'abilities'" class="space-y-2">
            <div v-for="a in detail?.abilities?.slice(0, 10)" :key="a.name"
              class="flex items-center gap-3 p-2 rounded-lg hover:bg-gray-50">
              <span class="text-xs text-blue-700 font-medium flex-1 truncate">{{ a.chinese_name || a.name }}</span>
              <div class="w-20 h-1.5 bg-gray-100 rounded-full overflow-hidden shrink-0">
                <div class="h-full bg-blue-400 rounded-full" :style="{ width: pct(a.usage) + '%' }"></div>
              </div>
              <span class="text-xs text-gray-400 font-mono w-12 text-right shrink-0">{{ pct(a.usage) }}%</span>
            </div>
            <div v-if="!detail?.abilities?.length" class="text-center py-8 text-gray-400 text-sm">暂无数据</div>
          </div>

          <div v-else-if="activeTab === 'tera'" class="space-y-2">
            <div v-if="hasTeraData" v-for="t in detail?.teras?.slice(0, 10)" :key="t.type"
              class="flex items-center gap-3 p-2 rounded-lg hover:bg-gray-50">
              <span class="text-xs text-purple-700 font-medium flex-1 truncate">{{ t.type }}</span>
              <div class="w-20 h-1.5 bg-gray-100 rounded-full overflow-hidden shrink-0">
                <div class="h-full bg-purple-400 rounded-full" :style="{ width: pct(t.usage) + '%' }"></div>
              </div>
              <span class="text-xs text-gray-400 font-mono w-12 text-right shrink-0">{{ pct(t.usage) }}%</span>
            </div>
            <div v-else class="text-center py-8 text-gray-400 text-sm">太晶属性分布数据暂不可用</div>
          </div>

          <div v-else-if="activeTab === 'teammates'" class="space-y-2">
            <div v-for="t in detail?.teammates?.slice(0, 20)" :key="t.mate"
              @click="$emit('selectTeammate', t.mate)"
              class="flex items-center gap-3 p-2 rounded-lg hover:bg-green-50 cursor-pointer">
              <span class="text-xs text-green-700 font-medium flex-1 truncate">
                {{ t.chinese_name || t.mate }}
                <span class="text-gray-400 text-[10px]">({{ t.mate }})</span>
              </span>
              <div class="w-20 h-1.5 bg-gray-100 rounded-full overflow-hidden shrink-0">
                <div class="h-full bg-green-400 rounded-full" :style="{ width: pct(t.usage) + '%' }"></div>
              </div>
              <span class="text-xs text-gray-400 font-mono w-12 text-right shrink-0">{{ pct(t.usage) }}%</span>
              <span class="text-gray-300 text-xs">→</span>
            </div>
            <div v-if="!detail?.teammates?.length" class="text-center py-8 text-gray-400 text-sm">暂无数据</div>
          </div>

          <div v-else-if="activeTab === 'cc'" class="space-y-2">
            <div v-if="detail?.ccs?.length">
              <div v-for="c in detail.ccs.slice(0, 20)" :key="c.opp"
                class="flex items-center gap-3 p-2 rounded-lg hover:bg-gray-50">
                <span class="text-xs text-gray-700 font-medium flex-1 truncate">
                  {{ c.chinese_name || c.opp }}
                </span>
                <span class="text-xs text-gray-600 font-mono shrink-0">{{ c.percentage?.toFixed(1) }}%</span>
                <span class="text-[10px] text-gray-400 shrink-0">σ {{ c.stddev?.toFixed(1) }}</span>
              </div>
              <div v-if="ccNote" class="text-xs text-amber-500 mt-2 text-center">{{ ccNote }}</div>
            </div>
            <div v-else class="text-center py-8 text-gray-400 text-sm">
              该分级暂无 Checks/Counters 数据
            </div>
          </div>

          <div v-else-if="activeTab === 'spreads'" class="space-y-2">
            <div v-for="s in detail?.spreads?.slice(0, 20)" :key="s.nature + s.evs"
              class="flex items-center gap-3 p-2 rounded-lg hover:bg-gray-50">
              <span class="text-xs text-amber-600 font-medium w-20 shrink-0">{{ s.nature }}</span>
              <span class="text-xs text-gray-500 font-mono flex-1">{{ s.evs }}</span>
              <div class="w-16 h-1.5 bg-gray-100 rounded-full overflow-hidden shrink-0">
                <div class="h-full bg-indigo-400 rounded-full" :style="{ width: pct(s.usage) + '%' }"></div>
              </div>
              <span class="text-xs text-gray-400 font-mono w-12 text-right shrink-0">{{ pct(s.usage) }}%</span>
            </div>
            <div v-if="!detail?.spreads?.length" class="text-center py-8 text-gray-400 text-sm">暂无数据</div>
          </div>
        </div>
      </div>
    </div>
  </Teleport>
</template>

<script setup>
import { ref, computed, watch } from 'vue'
import { smogonAPI } from '../../api/smogon'

const props = defineProps({
  pokemonName: { type: String, default: '' },
  visible: { type: Boolean, default: false },
  source: { type: String, default: 'smogon' },
  timeBucket: { type: String, default: '' },
  rating: { type: Number, default: 1760 },
})
const emit = defineEmits(['close', 'selectTeammate'])

const activeTab = ref('overview')
const loading = ref(false)
const detail = ref(null)
const trendData = ref([])

const tabs = [
  { key: 'overview', label: '概览' },
  { key: 'moves', label: '招式' },
  { key: 'items', label: '道具' },
  { key: 'abilities', label: '特性' },
  { key: 'tera', label: '太晶' },
  { key: 'teammates', label: '队友' },
  { key: 'cc', label: '天敌' },
  { key: 'spreads', label: 'EV' },
]

const info = computed(() => detail.value?.info)
const usageStr = computed(() => info.value ? (info.value.usage * 100).toFixed(2) + '%' : '-')
const hasTeraData = computed(() => {
  return detail.value?.teras?.some(t => t.type && t.type !== 'nothing')
})
const ccNote = computed(() => {
  if (props.rating >= 1630) return '提示: CC 数据仅对 0/1500 分级可用'
  return ''
})

async function load() {
  if (!props.pokemonName || !props.visible) return
  loading.value = true
  activeTab.value = 'overview'
  try {
    const [det, trend] = await Promise.all([
      smogonAPI.detail(props.pokemonName, {
        source: props.source,
        time_bucket: props.timeBucket,
        rating: props.rating,
      }),
      smogonAPI.trend(props.pokemonName, { rating: props.rating }),
    ])
    detail.value = det
    trendData.value = trend || []
  } catch (e) {
    detail.value = null
    trendData.value = []
  } finally {
    loading.value = false
  }
}

watch(() => [props.pokemonName, props.visible, props.timeBucket, props.rating], () => {
  if (props.visible && props.pokemonName) load()
})

function pct(v) { return Math.min(100, Math.round((v || 0) * 100)) }
function trendHeight(v) { return Math.max(4, Math.min(100, (v || 0) * 2)) }

import { cdnFrontSprite, cdnGen5Sprite } from '../../utils/spriteUrl'

function spriteUrl(name) {
  return cdnFrontSprite(name)
}
function onImgError(e) {
  const el = e.target
  if (!el._fbStep) { el._fbStep = 1; el.src = cdnGen5Sprite(el.getAttribute('data-name') || ''); return }
  el.style.display = 'none'
}
</script>

<style scoped>
.animate-slide-in {
  animation: slideIn 0.2s ease-out;
}
@keyframes slideIn {
  from { transform: translateX(100%); }
  to { transform: translateX(0); }
}
</style>
