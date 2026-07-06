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
import { useSpeciesStore } from './stores/speciesStore'
import { connect } from './api/wsClient'

const router = useRouter()
const { load: loadSpecies } = useSpeciesStore()
onMounted(async () => {
  startSession()
  await connect('Player').catch(() => {})
  loadSpecies()
})
router.afterEach((to) => { trackPageView(to.path) })
</script>
