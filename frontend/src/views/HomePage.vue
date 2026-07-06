<template>
  <div class="text-center py-12 relative overflow-hidden">
    <!-- Animated Pokemon background -->
    <div class="absolute inset-0 pointer-events-none opacity-10">
      <div v-for="p in bgPokemon" :key="p.id"
        class="absolute animate-float-bg"
        :style="{left:p.x+'%',top:p.y+'%',animationDelay:p.delay+'s'}">
        <IconSprite :species-id="p.id" :size="p.size >= 56 ? 'lg' : 'md'" />
      </div>
    </div>
    <h1 class="text-4xl font-extrabold text-gray-800 mb-3 tracking-tight">
      宝可梦<span class="text-rose-400">对战平台</span>
    </h1>
    <p class="text-lg text-gray-500 mb-2">成为最强的宝可梦训练家！</p>
    <p class="text-sm text-gray-400 mb-10">组建你的梦想队伍 · 挑战全球训练家 · 登上荣耀之巅</p>

    <!-- Feature Cards -->
    <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-5 max-w-4xl mx-auto">
      <router-link to="/matchmaking"
        class="glass-card glass-card-hover p-6 animate-slide-up animate-slide-up-1 group">
        <div class="text-5xl mb-3">🏟️</div>
        <h3 class="text-lg font-bold text-gray-800 group-hover:text-pokeball-red transition-colors">对战竞技场</h3>
        <p class="text-sm text-gray-500 mt-2">匹配对手 · 实时对战 · 决胜巅峰</p>
      </router-link>

      <router-link to="/teams"
        class="glass-card glass-card-hover p-6 animate-slide-up animate-slide-up-2 group">
        <div class="text-5xl mb-3">🎒</div>
        <h3 class="text-lg font-bold text-gray-800 group-hover:text-pokedex-blue transition-colors">队伍编辑</h3>
        <p class="text-sm text-gray-500 mt-2">可视化组队 · 配招策略 · 努力值分配</p>
      </router-link>

      <router-link to="/stats"
        class="glass-card glass-card-hover p-6 animate-slide-up animate-slide-up-3 group">
        <div class="text-5xl mb-3">📊</div>
        <h3 class="text-lg font-bold text-gray-800 group-hover:text-green-600 transition-colors">大数据分析</h3>
        <p class="text-sm text-gray-500 mt-2">使用率排行 · 环境趋势 · 对战数据</p>
      </router-link>

      <router-link to="/data"
        class="glass-card glass-card-hover p-6 animate-slide-up animate-slide-up-4 group">
        <div class="text-5xl mb-3">📖</div>
        <h3 class="text-lg font-bold text-gray-800 group-hover:text-amber-500 transition-colors">数据图鉴</h3>
        <p class="text-sm text-gray-500 mt-2">宝可梦 · 招式 · 特性 · 属性相克</p>
      </router-link>
    </div>

  </div>
</template>

<script setup>
import IconSprite from '../components/shared/IconSprite.vue'

const bgPokemon = [
  {id:25,x:5,y:10,delay:0,size:48},{id:6,x:15,y:60,delay:2,size:56},
  {id:1,x:25,y:30,delay:4,size:40},{id:133,x:35,y:70,delay:1,size:44},
  {id:150,x:45,y:20,delay:3,size:60},{id:149,x:55,y:50,delay:5,size:64},
  {id:9,x:65,y:15,delay:2,size:52},{id:3,x:75,y:55,delay:1,size:54},
  {id:94,x:85,y:25,delay:3,size:48},{id:248,x:10,y:80,delay:0,size:60},
  {id:445,x:50,y:80,delay:4,size:58},{id:658,x:80,y:75,delay:2,size:44},
]
</script>
