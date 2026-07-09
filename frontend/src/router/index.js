import { createRouter, createWebHistory } from 'vue-router'
import { trackPageView, trackSessionStart } from '../utils/track'

const routes = [
  { path: '/', name: 'home', component: () => import('../views/HomePage.vue') },
  { path: '/login', name: 'login', component: () => import('../views/LoginPage.vue') },
  { path: '/teams', name: 'teams', component: () => import('../views/TeamBuilder.vue') },
  { path: '/teams/:id', name: 'team-edit', component: () => import('../views/TeamBuilder.vue') },
  { path: '/battles', redirect: '/matchmaking' },
  { path: '/matchmaking', name: 'matchmaking', component: () => import('../views/MatchmakingPage.vue') },
  { path: '/stats', name: 'stats', component: () => import('../views/StatsDashboard.vue') },
  { path: '/data', name: 'data', component: () => import('../views/DataExplorer.vue') },
  { path: '/realtime', name: 'realtime', component: () => import('../views/RealTimeStats.vue') },
  { path: '/analytics', name: 'analytics', component: () => import('../views/AnalyticsPage.vue') },
]

const router = createRouter({
  history: createWebHistory(),
  routes,
})

// ── Session start ──
trackSessionStart()

// ── Page view tracking ──
router.afterEach((to) => {
  trackPageView(to.path)
})

export default router
