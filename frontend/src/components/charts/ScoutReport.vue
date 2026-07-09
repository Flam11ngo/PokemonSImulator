<template>
  <div>
    <!-- Search bar -->
    <div class="glass-card p-5 mb-6">
      <div class="flex items-center gap-4">
        <div class="flex-1 relative">
          <input v-model="username" @keyup.enter="doSearch"
            placeholder="输入训练家用户名进行侦察..."
            class="w-full px-5 py-3 rounded-xl bg-gray-50 border border-gray-200 text-sm focus:border-indigo-400 focus:outline-none focus:ring-2 focus:ring-indigo-200 transition-all pr-24" />
          <span class="absolute right-3 top-1/2 -translate-y-1/2 text-xs text-gray-400">回车搜索</span>
        </div>
        <button @click="doSearch" :disabled="searching"
          class="px-6 py-3 rounded-xl font-bold text-sm text-white transition-all flex items-center gap-2"
          :class="searching ? 'bg-gray-400' : 'bg-gradient-to-r from-purple-600 to-indigo-600 hover:shadow-lg active:scale-95'">
          {{ searching ? '🔍 侦察中...' : '🔍 开始侦察' }}
        </button>
      </div>
      <!-- User suggestions -->
      <div v-if="suggestions.length && !result" class="flex gap-2 mt-3 flex-wrap">
        <span class="text-xs text-gray-400">推荐侦察:</span>
        <button v-for="u in suggestions" :key="u" @click="username = u; doSearch()"
          class="text-xs px-3 py-1 rounded-full bg-indigo-50 text-indigo-600 hover:bg-indigo-100 transition-colors">{{ u }}</button>
      </div>
    </div>

    <!-- Loading -->
    <div v-if="searching" class="glass-card p-12 text-center">
      <div class="text-6xl mb-4 animate-bounce">👁️</div>
      <p class="text-gray-500 text-sm">正在侦察 {{ username }} 的战术风格...</p>
    </div>

    <!-- Error -->
    <div v-if="error" class="glass-card p-12 text-center">
      <div class="text-5xl mb-4">❌</div>
      <p class="text-gray-700 font-medium">{{ error }}</p>
      <p class="text-sm text-gray-400 mt-2">换个用户名试试</p>
    </div>

    <!-- Report -->
    <div v-if="result" class="space-y-6">
      <!-- Style Banner -->
      <div class="relative overflow-hidden rounded-2xl bg-gradient-to-br from-purple-900 via-indigo-900 to-slate-900 p-8 text-white">
        <div class="absolute inset-0 opacity-15">
          <div class="absolute top-0 right-0 w-72 h-72 bg-purple-400 rounded-full blur-3xl" />
          <div class="absolute bottom-0 left-0 w-56 h-56 bg-pink-400 rounded-full blur-3xl" />
        </div>
        <div class="relative flex items-start justify-between flex-wrap gap-4">
          <div>
            <p class="text-purple-300 text-xs tracking-wider mb-2">🔮 SCOUTING REPORT</p>
            <h2 class="text-2xl font-bold mb-3">{{ result.username }} 的战术档案</h2>
            <div class="flex gap-2 flex-wrap">
              <span v-for="tag in result.style_tags" :key="tag"
                class="px-3 py-1 rounded-full text-xs font-bold bg-white/15 backdrop-blur border border-white/20">
                {{ tag }}
              </span>
            </div>
          </div>
          <div class="text-right text-xs text-purple-300/70">
            <div>加入于 {{ result.created_at?.split(' ')[0] }}</div>
            <div>队伍数 {{ result.stats.total_teams }}</div>
            <div>精灵库 {{ result.stats.unique_species }} 种</div>
          </div>
        </div>
      </div>

      <!-- Stats -->
      <div class="grid grid-cols-2 sm:grid-cols-4 gap-3">
        <div v-for="s in quickStats" :key="s.label" class="glass-card p-4 text-center">
          <div class="text-2xl font-bold" :class="s.color">{{ s.value }}</div>
          <div class="text-xs text-gray-400 mt-1">{{ s.label }}</div>
        </div>
      </div>

      <!-- Trends -->
      <div class="grid grid-cols-1 lg:grid-cols-3 gap-5">
        <div class="glass-card p-4">
          <h4 class="text-sm font-bold text-gray-700 mb-3">🔥 偏好属性</h4>
          <div class="space-y-1.5">
            <div v-for="t in result.stats.top_types" :key="t.name"
              class="flex items-center justify-between text-sm">
              <span class="text-gray-600 capitalize">{{ t.name }}</span>
              <span class="text-xs bg-gray-100 px-2 py-0.5 rounded-full text-gray-500">{{ t.count }}次</span>
            </div>
          </div>
        </div>
        <div class="glass-card p-4">
          <h4 class="text-sm font-bold text-gray-700 mb-3">⚡ 常用招式</h4>
          <div class="space-y-1.5">
            <div v-for="m in result.stats.top_moves" :key="m.name"
              class="flex items-center justify-between text-sm">
              <span class="text-gray-600">{{ m.name }}</span>
              <span class="text-xs bg-gray-100 px-2 py-0.5 rounded-full text-gray-500">{{ m.count }}次</span>
            </div>
          </div>
        </div>
        <div class="glass-card p-4">
          <h4 class="text-sm font-bold text-gray-700 mb-3">🐾 偏好精灵</h4>
          <div class="space-y-1.5">
            <div v-for="s in result.stats.top_species" :key="s.name"
              class="flex items-center justify-between text-sm">
              <span class="text-gray-600">{{ s.name }}</span>
              <span class="text-xs bg-gray-100 px-2 py-0.5 rounded-full text-gray-500">{{ s.count }}只</span>
            </div>
          </div>
        </div>
      </div>

      <!-- Teams -->
      <div v-for="(team, ti) in result.teams" :key="ti" class="glass-card p-5">
        <h4 class="text-sm font-bold text-gray-800 mb-4 flex items-center gap-2">
          🎒 {{ team.name }}
          <span class="text-xs text-gray-400 font-normal">更新于 {{ team.updated_at }}</span>
        </h4>
        <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-3">
          <div v-for="(p, pi) in team.pokemon" :key="pi"
            class="flex gap-3 p-3 rounded-xl bg-gray-50 hover:bg-white hover:shadow-sm transition-all group cursor-default">
            <img :src="p.sprite_url" class="w-14 h-14 object-contain shrink-0 drop-shadow"
              style="image-rendering: pixelated" @error="$event.target.style.display='none'" />
            <div class="min-w-0 flex-1 text-xs">
              <div class="font-bold text-gray-800 text-sm mb-1">{{ p.species_name }}</div>
              <div class="text-gray-500 space-y-0.5">
                <div>🎯 {{ p.ability.name || '?' }} <span v-if="p.item.name" class="ml-1">· 💍 {{ p.item.name }}</span></div>
                <div>🌿 {{ p.nature.name }}</div>
                <div class="flex gap-1 mt-1 flex-wrap">
                  <span v-for="m in p.moves" :key="m.id"
                    class="px-1.5 py-0.5 rounded bg-indigo-50 text-indigo-600 text-xs font-medium">{{ m.name }}</span>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed } from 'vue'
import axios from 'axios'

const username = ref('')
const result = ref(null)
const error = ref('')
const searching = ref(false)
const suggestions = ref(['myz', 'jxl', 'lzx', 'c'])

const quickStats = computed(() => {
  if (!result.value) return []
  const s = result.value.stats
  return [
    { label: '队伍数', value: s.total_teams, color: 'text-purple-500' },
    { label: '精灵总数', value: s.total_pokemon, color: 'text-indigo-500' },
    { label: '精灵种类', value: s.unique_species, color: 'text-pink-500' },
    { label: '属性偏好', value: s.top_types[0]?.name || '-', color: 'text-amber-500' },
  ]
})

async function doSearch() {
  if (!username.value.trim()) return
  searching.value = true
  error.value = ''
  result.value = null
  try {
    const res = await axios.get(`/api/v1/scout?username=${encodeURIComponent(username.value.trim())}`)
    if (res.data?.ok) {
      result.value = res.data.data
    } else {
      error.value = res.data?.error || '未知错误'
    }
  } catch (e) {
    error.value = e.response?.data?.error || '侦察失败，请检查服务是否运行'
  } finally {
    searching.value = false
  }
}
</script>
