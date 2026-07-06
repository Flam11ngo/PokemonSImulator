<template>
  <div class="min-h-screen flex flex-col">
    <AppNavbar />
    <main class="flex-1 container mx-auto px-4 py-6 max-w-7xl">
      <router-view />
    </main>
  </div>
</template>

<script setup>
import { onMounted } from 'vue'
import { useRouter } from 'vue-router'
import AppNavbar from './components/layout/AppNavbar.vue'
import { startSession, trackPageView } from './utils/analytics'

const router = useRouter()
onMounted(() => {
  startSession()
  trackPageView(router.currentRoute.value?.path || '/')
})
router.afterEach((to) => { trackPageView(to.path) })
</script>
