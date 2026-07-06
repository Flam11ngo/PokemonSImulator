import { createRouter, createWebHistory } from 'vue-router'

const routes = [
  { path: '/', name: 'home', component: () => import('../views/HomePage.vue') },
  { path: '/login', name: 'login', component: () => import('../views/LoginPage.vue') },
  { path: '/teams', name: 'teams', component: () => import('../views/TeamBuilder.vue') },
  { path: '/teams/:id', name: 'team-edit', component: () => import('../views/TeamBuilder.vue') },
  { path: '/battles', redirect: '/matchmaking' },
  { path: '/matchmaking', name: 'matchmaking', component: () => import('../views/MatchmakingPage.vue') },
  { path: '/stats', name: 'stats', component: () => import('../views/StatsDashboard.vue') },
  { path: '/data', name: 'data', component: () => import('../views/DataExplorer.vue') },
]

const router = createRouter({
  history: createWebHistory(),
  routes,
})

export default router
