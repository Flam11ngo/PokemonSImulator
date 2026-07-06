<template>
  <div>
    <h1 class="text-2xl font-bold text-gray-800 mb-6">📋 对战历史</h1>

    <!-- Filters -->
    <div class="flex gap-3 mb-5 flex-wrap">
      <select v-model="filterStatus"
        class="bg-white/70 border border-gray-200 rounded-xl px-3 py-2 text-sm text-gray-600 focus:outline-none focus:ring-2 focus:ring-pokeball-red/30">
        <option value="">全部状态</option>
        <option value="completed">已完成</option>
        <option value="active">进行中</option>
        <option value="pending">等待中</option>
        <option value="error">错误</option>
      </select>
      <button @click="refresh"
        class="px-4 py-2 bg-white/70 hover:bg-white border border-gray-200 text-gray-600 rounded-xl text-sm transition-all duration-200 shadow-sm hover:shadow-md">
        🔄 刷新
      </button>
    </div>

    <!-- Battle list -->
    <div v-if="battles.length > 0" class="space-y-3">
      <div v-for="battle in battles" :key="battle.id"
        class="glass-card p-4 glass-card-hover transition-all duration-200">
        <div class="flex items-center justify-between">
          <div class="flex-1">
            <div class="flex items-center gap-3">
              <span class="text-xs px-2.5 py-1 rounded-full font-medium" :class="statusClass(battle.status)">
                {{ battle.status }}
              </span>
              <span class="text-sm text-gray-500">
                Turn {{ battle.total_turns || 0 }}
              </span>
              <span v-if="battle.winner_side" class="text-sm text-amber-500 font-semibold">
                🏆 Winner: Side {{ battle.winner_side.toUpperCase() }}
              </span>
            </div>
            <div class="text-xs text-gray-400 mt-1.5">
              {{ battle.created_at }} · Seed: {{ battle.seed }}
            </div>
          </div>
          <div class="flex gap-2">
            <router-link :to="`/battles/${battle.id}`"
              class="px-4 py-1.5 bg-pokedex-blue/10 text-pokedex-blue rounded-full text-xs font-medium hover:bg-pokedex-blue/20 transition-colors">
              👁️ 查看
            </router-link>
          </div>
        </div>
      </div>
    </div>

    <!-- Empty state -->
    <div v-else class="text-center text-gray-400 py-20">
      <div class="text-5xl mb-4">📭</div>
      <p class="text-lg mb-2 font-medium text-gray-600">暂无对战记录</p>
      <p class="text-sm">去 <router-link to="/matchmaking" class="text-pokeball-red hover:text-red-600 font-semibold">对战竞技场</router-link> 开始第一场对战吧！</p>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, watch } from 'vue'
import { battleAPI } from '../api/battles'

const battles = ref([])
const filterStatus = ref('')

async function refresh() {
  const params = {}
  if (filterStatus.value) params.status = filterStatus.value
  const result = await battleAPI.list(params)
  battles.value = result.data || []
}

function statusClass(status) {
  switch (status) {
    case 'completed': return 'bg-green-100 text-green-600'
    case 'active': return 'bg-blue-100 text-blue-600'
    case 'pending': return 'bg-amber-100 text-amber-600'
    case 'error': return 'bg-red-100 text-red-500'
    default: return 'bg-gray-100 text-gray-500'
  }
}

watch(filterStatus, refresh)
onMounted(refresh)
</script>
