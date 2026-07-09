import client from './client'

export const statsAPI = {
  snapshot: () => client.get('/stats/snapshot'),
  global: () => client.get('/stats/global'),
  // Deep analytics
  deepSummary: () => client.get('/stats/deep/summary'),
  deepMeta: () => client.get('/stats/deep/meta'),
  deepMoves: () => client.get('/stats/deep/moves'),
  deepItems: () => client.get('/stats/deep/items'),
  deepAbilities: () => client.get('/stats/deep/abilities'),
  deepTypes: () => client.get('/stats/deep/types'),
  deepEvents: () => client.get('/stats/deep/events'),
  deepAll: () => client.get('/stats/deep/all'),
  deepLive: () => client.get('/stats/deep/live'),
  // UI Analytics
  uiSummary: () => client.get('/stats/ui/summary'),
  uiClicks: () => client.get('/stats/ui/clicks'),
  uiPlayers: () => client.get('/stats/ui/players'),
  uiRecent: () => client.get('/stats/ui/recent'),
  uiFavorites: () => client.get('/stats/ui/favorites'),
  uiPageDwell: () => client.get('/stats/ui/page_dwell'),
}

export const dataAPI = {
  enums: () => client.get('/data/enums'),
}
