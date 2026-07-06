import client from './client'

// client.js interceptor already unwraps {ok, data} -> response.data = inner data
async function get(path, params) {
  const res = await client.get(path, params ? { params } : undefined)
  return res.data
}

export const smogonAPI = {
  filters: () => get('/smogon/filters'),
  ranking: (params) => get('/smogon/pokemon', params),
  detail: (name, params) => get(`/smogon/pokemon/${encodeURIComponent(name)}`, params),
  summary: (params) => get('/smogon/summary', params),
  trend: (name, params) => get(`/smogon/trend/${encodeURIComponent(name)}`, params),
  types: (params) => get('/smogon/types', params),
  items: (params) => get('/smogon/items', params),
  moves: (params) => get('/smogon/moves', params),
}
