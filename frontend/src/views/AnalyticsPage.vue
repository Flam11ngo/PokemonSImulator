<template>
  <div class="stats-page">
    <!-- Top bar -->
    <div class="topbar">
      <div class="topbar-left">
        <h1 class="topbar-title">👤 用户行为分析</h1>
        <span class="topbar-badge" :class="polling ? 'live' : 'off'">
          <span class="badge-dot" /> {{ polling ? 'LIVE' : '离线' }}
        </span>
        <span class="topbar-time">更新于 {{ lastUpdated }}</span>
      </div>
      <div class="topbar-right">
        <span class="topbar-metric">👥 {{ summary.players || 0 }} 玩家</span>
        <span class="topbar-metric">📋 {{ animatedEvents.toLocaleString() }} 事件</span>
        <span class="topbar-metric">⭐ {{ topFeature }}</span>
        <button @click="refresh" :disabled="refreshing" class="topbar-btn">🔄</button>
      </div>
    </div>

    <!-- Quick stats -->
    <div class="quick-row">
      <div class="quick-card" v-for="c in quickCards" :key="c.label">
        <div class="quick-num">{{ c.value }}</div>
        <div class="quick-lbl">{{ c.label }}</div>
      </div>
    </div>

    <!-- Nav tabs -->
    <div class="nav-row">
      <button v-for="tab in tabs" :key="tab.key" @click="activeTab = tab.key"
        class="nav-btn" :class="{ 'nav-active': activeTab === tab.key }">{{ tab.label }}</button>
    </div>

    <!-- ⭐ Favorites / Dwell Time -->
    <div v-if="activeTab === 'favorites'" class="tab-body">
      <div class="grid-2col">
        <div class="panel">
          <div class="panel-title">⏱️ 页面停留时长 (Spark 计算)</div>
          <div class="panel-body">
            <div v-if="dwellData.length === 0" class="skeleton-list">
              <div v-for="i in 6" :key="i" class="skel-row" :style="{ animationDelay: `${i * 0.1}s` }">
                <span class="skel-rank" />
                <span class="skel-label" />
                <span class="skel-bar" />
              </div>
            </div>
            <div v-for="(d, i) in dwellData" :key="d.page" class="bar-row">
              <span class="bar-rank">{{ i + 1 }}</span>
              <span class="bar-label">{{ PAGE_LABELS[d.page] || d.page }}</span>
              <div class="bar-track">
                <div class="bar-fill bar-dwell" :style="{ width: barPct(d.dwell, dwellMax) + '%' }" />
              </div>
              <span class="bar-val">{{ fmtDuration(d.dwell) }}</span>
            </div>
          </div>
        </div>
        <div class="panel">
          <div class="panel-title">⭐ 全站功能偏好排名</div>
          <div class="panel-body">
            <div v-for="(f, i) in topFeatures.slice(0, 12)" :key="f.feature" class="bar-row">
              <span class="bar-rank">{{ i + 1 }}</span>
              <span class="bar-label">{{ featureLabel(f.feature) }}</span>
              <div class="bar-track">
                <div class="bar-fill bar-fav" :style="{ width: barPct(f.score, favMax) + '%' }" />
              </div>
              <span class="bar-val">{{ f.score.toLocaleString() }}</span>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- 📄 Pages -->
    <div v-if="activeTab === 'pages'" class="tab-body">
      <DataTable title="📄 页面访问排行" :columns="pageCols" :rows="pageData" :loading="loading"
        id-key="page" defaultSort="views" :stableHeight="true" :stableHeightPx="580"
        search-placeholder="搜索页面..." />
    </div>

    <!-- 🖱️ Clicks -->
    <div v-if="activeTab === 'clicks'" class="tab-body">
      <DataTable title="🖱️ 按钮点击排行" :columns="clickCols" :rows="clickData" :loading="loading"
        id-key="element" defaultSort="clicks" :stableHeight="true" :stableHeightPx="580"
        search-placeholder="搜索..." />
    </div>

    <!-- 👤 Players -->
    <div v-if="activeTab === 'players'" class="tab-body">
      <DataTable title="👤 玩家活跃度" :columns="playerCols" :rows="playerData" :loading="loading"
        id-key="player_id" defaultSort="_events" :stableHeight="true" :stableHeightPx="580"
        search-placeholder="搜索玩家..." />
    </div>

    <!-- 📡 Live feed -->
    <div v-if="activeTab === 'feed'" class="tab-body">
      <div class="ticker-wrap">
        <div class="ticker-header">
          <span>📡 实时事件流</span>
          <span class="ticker-count">{{ feedEvents.length }} 条 · {{ FEED_POLL }}s 刷新</span>
        </div>
        <div class="ticker-track">
          <div class="ticker-scroll" :style="{ animationDuration: scrollDuration + 's' }">
            <div class="ticker-col">
              <div v-for="r in feedEvents" :key="r._k" class="ticker-item" :class="{ 'ticker-new': r._fresh }">
                <span class="tick-icon">{{ eventIcon(r.event) }}</span>
                <span class="tick-player">{{ r.player_id }}</span>
                <span class="tick-detail">{{ eventText(r) }}</span>
                <span class="tick-time">{{ fmtTime(r.timestamp) }}</span>
              </div>
            </div>
            <div class="ticker-col">
              <div v-for="r in feedEvents" :key="'d_' + r._k" class="ticker-item">
                <span class="tick-icon">{{ eventIcon(r.event) }}</span>
                <span class="tick-player">{{ r.player_id }}</span>
                <span class="tick-detail">{{ eventText(r) }}</span>
                <span class="tick-time">{{ fmtTime(r.timestamp) }}</span>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted } from 'vue'
import { statsAPI } from '../api/stats'
import DataTable from '../components/charts/DataTable.vue'

const FEED_POLL = 5
const DATA_POLL = 30

const loading = ref(true)
const polling = ref(false)
const refreshing = ref(false)
const activeTab = ref('favorites')
const lastUpdated = ref('--')

const summary = ref({})
const animatedEvents = ref(0)
const topFeatures = ref([])
const favPlayers = ref([])
const pageDwell = ref([])  // from /stats/ui/page_dwell
const clickItems = ref([])
const pageViews = ref([])
const players = ref([])

const topFeature = computed(() => {
  if (topFeatures.value.length === 0) return '--'
  return featureLabel(topFeatures.value[0].feature)
})
const favMax = computed(() => Math.max(...topFeatures.value.map(f => f.score || 0), 1))

// Dwell time per page (SQL-computed, no Spark needed)
const dwellData = computed(() =>
  pageDwell.value.map(d => ({ page: d.page, dwell: d.total_dwell_seconds || 0 }))
)
const dwellMax = computed(() => Math.max(...dwellData.value.map(d => d.dwell), 1))

function fmtDuration(sec) {
  if (!sec || sec < 1) return '<1s'
  if (sec < 60) return Math.round(sec) + 's'
  if (sec < 3600) return Math.floor(sec / 60) + 'm ' + Math.round(sec % 60) + 's'
  return Math.floor(sec / 3600) + 'h ' + Math.floor((sec % 3600) / 60) + 'm'
}

const quickCards = computed(() => [
  { value: summary.value.players || 0, label: '活跃玩家' },
  { value: animatedEvents.value.toLocaleString(), label: '总事件' },
  { value: topFeatures.value.length || 0, label: '功能类型' },
  { value: clickItems.value.length || 0, label: '操作类型' },
])

const pageData = computed(() =>
  pageViews.value.map(p => ({ page: p.page, _label: PAGE_LABELS[p.page] || p.page, views: p.n }))
)
const pageCols = [
  { key: '_label', label: '页面', sortable: true },
  { key: 'views', label: '访问次数', sortable: true, spark: true, class: 'text-right' },
]

const clickData = computed(() =>
  clickItems.value.map(c => ({ element: c.element, _label: CLICK_LABELS[c.element] || c.element, clicks: c.n }))
)
const clickCols = [
  { key: '_label', label: '按钮/功能', sortable: true },
  { key: 'clicks', label: '点击次数', sortable: true, spark: true, class: 'text-right' },
]

const playerData = computed(() =>
  players.value.map(p => ({ ...p, _events: p.events || 0, _last: fmtTime(p.last_seen) }))
)
const playerCols = [
  { key: 'player_id', label: '玩家', sortable: true },
  { key: '_events', label: '事件数', sortable: true, spark: true, class: 'text-right' },
  { key: '_last', label: '最后活跃', sortable: true },
]

// ── Labels ──
const PAGE_LABELS = { '/': '🏠 首页', '/matchmaking': '⚔️ 匹配', '/teams': '🧩 组队', '/stats': '📊 统计', '/data': '📁 数据', '/analytics': '👤 分析' }
const CLICK_LABELS = {
  'btn_confirm': '✅ 确认', 'btn_switch': '🔄 换人', 'btn_move_select': '💥 选招',
  'btn_join_match': '⚔️ PvP匹配', 'btn_join_bot': '🤖 对战Bot', 'btn_save_team': '💾 保存队伍',
  'nav_matchmaking': '导航-匹配', 'nav_teams': '导航-组队', 'nav_stats': '导航-统计', 'nav_data': '导航-数据',
}
const STATE_LABELS = { 'idle': '空闲', 'matching': '匹配中', 'battling': '对战中', 'teambuilding': '组队中' }
function featureLabel(f) { return PAGE_LABELS[f] || CLICK_LABELS[f] || f }
function eventIcon(e) { const m = { page_view: '📄', ui_click: '🖱️', player_state: '🔄', matchmaking_join: '⚔️', session_start: '👋', team_save: '💾' }; return m[e] || '📌' }
function eventText(r) {
  // VM's stats_server provides pre-formatted `detail` field
  if (r.detail) return r.detail
  if (r.event === 'page_view') return `浏览 ${PAGE_LABELS[r.page] || r.page}`
  if (r.event === 'ui_click') return `点击 ${CLICK_LABELS[r.element] || r.element}`
  if (r.event === 'player_state') return `${STATE_LABELS[r.state_from] || r.state_from} → ${STATE_LABELS[r.state_to] || r.state_to}`
  if (r.event === 'matchmaking_join') return '加入匹配队列'
  if (r.event === 'session_start') return '进入应用'
  if (r.event === 'team_save') return `保存队伍「${r.team_name}」`
  return r.event
}
function fmtTime(ts) { if (!ts) return '--'; try { return new Date(ts).toLocaleTimeString() } catch { return ts } }

function barPct(val, max) { return max > 0 ? (val / max) * 100 : 0 }
function animateNumber(ref, target) {
  if (target === ref.value) return
  const start = ref.value; const diff = target - start
  const st = performance.now()
  function tick(now) {
    const p = Math.min((now - st) / 500, 1)
    ref.value = Math.round(start + diff * (1 - Math.pow(1 - p, 3)))
    if (p < 1) requestAnimationFrame(tick)
  }
  requestAnimationFrame(tick)
}

// ── Data poll (15s) ──
async function loadData() {
  polling.value = true
  try {
    const res = await statsAPI.snapshot().catch(() => null)
    if (!res) return
    const d = (res.data || res) || {}
    summary.value = { players: d.summary?.players || 0, total_events: d.summary?.events || 0 }
    animateNumber(animatedEvents, d.summary?.events || 0)
    pageViews.value = (d.page_views || []).filter(p => !['/test','/login','/realtime'].includes(p.page))
    clickItems.value = d.clicks || []
    players.value = d.players || []
    pageDwell.value = d.page_dwell || []
    topFeatures.value = (d.clicks || []).map(c => ({ feature: c.element, score: c.n }))
    lastUpdated.value = new Date().toLocaleTimeString()
  } catch {} finally { polling.value = false; loading.value = false }
}
async function refresh() { refreshing.value = true; await loadData(); refreshing.value = false }

// ── Feed (3s, append to queue, CSS scroll) ──
const feedEvents = ref([])
const scrollDuration = ref(60)
let _feedSeq = 0
const _seenKeys = new Set()

async function pollFeed() {
  try {
    const res = await statsAPI.snapshot().catch(() => null)
    const data = (res && res.data ? res.data.recent : []) || []
    const fresh = []
    for (const r of data) {
      const k = String(r.id || r.timestamp || '')
      // Skip dirty / undefined pages
      if (/\/test|\/login|\/realtime/.test(r.detail || '')) continue
      if (!_seenKeys.has(k)) { _seenKeys.add(k); fresh.push({ ...r, _k: k + '_' + (++_feedSeq), _fresh: true }) }
    }
    if (fresh.length === 0) return
    feedEvents.value = [...feedEvents.value, ...fresh].slice(-120)
    scrollDuration.value = Math.max(30, feedEvents.value.length * 1.2)
    setTimeout(() => { fresh.forEach(f => { f._fresh = false }) }, 3000)
  } catch {}
}

const tabs = [
  { key: 'favorites', label: '⭐ 最爱功能' },
  { key: 'pages', label: '📄 页面访问' },
  { key: 'clicks', label: '🖱️ 操作偏好' },
  { key: 'players', label: '👤 玩家画像' },
  { key: 'feed', label: '📡 实时事件' },
]

let _dataPoll = null, _feedPoll = null
onMounted(async () => {
  loading.value = true; await loadData()
  try {
    const res = await statsAPI.uiRecent()
    const data = (res && res.data ? res.data : res) || []
    data.forEach(r => { _seenKeys.add(String(r.id || r.timestamp || '')) })
    feedEvents.value = data.map(r => ({ ...r, _k: (r.id || r.timestamp || '') + '_' + (++_feedSeq) }))
    scrollDuration.value = Math.max(30, feedEvents.value.length * 1.2)
  } catch {}
  _dataPoll = setInterval(loadData, DATA_POLL * 1000)
  _feedPoll = setInterval(pollFeed, FEED_POLL * 1000)
})
onUnmounted(() => { if (_dataPoll) clearInterval(_dataPoll); if (_feedPoll) clearInterval(_feedPoll) })
</script>

<style scoped>
.stats-page { max-width: 1200px; margin: 0 auto; padding: 16px 16px 40px; }

.topbar {
  display: flex; align-items: center; justify-content: space-between; flex-wrap: wrap; gap: 10px;
  padding: 12px 18px; margin-bottom: 14px;
  background: rgba(255,255,255,0.75); backdrop-filter: blur(12px);
  border: 1px solid rgba(255,255,255,0.5); border-radius: 14px;
}
.topbar-left { display: flex; align-items: center; gap: 10px; flex-wrap: wrap; }
.topbar-title { font-size: 1.05rem; font-weight: 700; color: #1e293b; margin: 0; }
.topbar-badge { display: inline-flex; align-items: center; gap: 5px; font-size: 0.58rem; font-weight: 700; letter-spacing: 0.08em; text-transform: uppercase; padding: 3px 10px; border-radius: 99px; }
.topbar-badge.live { background: #ecfdf5; color: #059669; }
.topbar-badge.off { background: #f1f5f9; color: #94a3b8; }
.badge-dot { width: 6px; height: 6px; border-radius: 50%; }
.live .badge-dot { background: #10b981; animation: dot-pulse 1.5s ease-in-out infinite; }
.off .badge-dot { background: #94a3b8; }
@keyframes dot-pulse { 0%,100% { opacity: 1; } 50% { opacity: 0.3; } }
.topbar-time { font-size: 0.7rem; color: #94a3b8; }
.topbar-right { display: flex; align-items: center; gap: 10px; flex-wrap: wrap; }
.topbar-metric { font-size: 0.76rem; font-weight: 600; color: #475569; }
.topbar-btn { font-size: 0.9rem; padding: 5px 10px; border-radius: 99px; border: 1px solid #e2e8f0; background: #f8fafc; cursor: pointer; }
.topbar-btn:hover { background: #f1f5f9; }

.quick-row { display: grid; grid-template-columns: repeat(4, 1fr); gap: 12px; margin-bottom: 14px; }
.quick-card { background: rgba(255,255,255,0.8); backdrop-filter: blur(12px); border: 1px solid rgba(255,255,255,0.5); border-radius: 16px; padding: 16px; text-align: center; }
.quick-num { font-size: 1.4rem; font-weight: 800; color: #1e293b; }
.quick-lbl { font-size: 0.7rem; color: #94a3b8; margin-top: 2px; }

.nav-row { display: flex; gap: 6px; margin-bottom: 14px; }
.nav-btn { padding: 9px 22px; border-radius: 12px; font-size: 0.84rem; font-weight: 600; border: none; background: rgba(255,255,255,0.65); color: #64748b; cursor: pointer; transition: all 0.2s; }
.nav-btn:hover { background: rgba(255,255,255,0.9); color: #334155; }
.nav-active { background: #fff; color: #1e293b; box-shadow: 0 1px 6px rgba(0,0,0,0.1); }

.tab-body { display: flex; flex-direction: column; gap: 14px; }
.grid-2col { display: grid; grid-template-columns: 1fr 1fr; gap: 14px; }
@media (max-width: 960px) { .grid-2col { grid-template-columns: 1fr; } }

.panel { background: rgba(255,255,255,0.8); backdrop-filter: blur(12px); border: 1px solid rgba(255,255,255,0.5); border-radius: 16px; overflow: hidden; }
.panel-title { font-size: 0.8rem; font-weight: 700; color: #475569; padding: 14px 18px; border-bottom: 1px solid #f1f5f9; }
.panel-body { padding: 10px 16px; max-height: 480px; overflow-y: auto; }

.bar-row { display: flex; align-items: center; gap: 8px; padding: 6px 0; }
.bar-rank { font-size: 0.7rem; color: #94a3b8; font-family: monospace; width: 20px; }
.bar-label { font-size: 0.82rem; color: #334155; width: 90px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.bar-track { flex: 1; height: 10px; border-radius: 99px; background: #f1f5f9; overflow: hidden; }
.bar-fill { height: 100%; border-radius: 99px; background: linear-gradient(90deg, #818cf8, #6366f1); transition: width 0.8s ease; }
.bar-fill.bar-fav { background: linear-gradient(90deg, #fbbf24, #f59e0b, #ef4444); }
.bar-fill.bar-dwell { background: linear-gradient(90deg, #06b6d4, #3b82f6); }
.bar-val { font-size: 0.75rem; color: #64748b; font-family: monospace; width: 60px; text-align: right; }

/* Skeleton loading */
.skeleton-list { display: flex; flex-direction: column; gap: 8px; padding: 4px 0; }
.skel-row { display: flex; align-items: center; gap: 8px; height: 20px; animation: skel-pulse 1.8s ease-in-out infinite; }
.skel-rank { width: 16px; height: 12px; border-radius: 3px; background: #e2e8f0; }
.skel-label { width: 60px; height: 12px; border-radius: 3px; background: #e2e8f0; }
.skel-bar { flex: 1; height: 10px; border-radius: 99px; background: #e2e8f0; }
@keyframes skel-pulse { 0%,100% { opacity: 0.4; } 50% { opacity: 0.8; } }

/* ── Ticker ──────────────────────────── */
.ticker-wrap {
  background: rgba(255,255,255,0.8); backdrop-filter: blur(12px);
  border: 1px solid rgba(99,102,241,0.12); border-radius: 16px; overflow: hidden;
}
.ticker-header {
  display: flex; align-items: center; justify-content: space-between;
  padding: 12px 18px; border-bottom: 1px solid #f1f5f9;
  font-size: 0.82rem; font-weight: 700; color: #1e293b;
}
.ticker-count { font-size: 0.68rem; color: #94a3b8; font-weight: 500; }
.ticker-track {
  height: 460px; overflow: hidden;
  mask-image: linear-gradient(to bottom, transparent 0%, black 6%, black 94%, transparent 100%);
}
.ticker-scroll { animation: ticker-roll linear infinite; will-change: transform; backface-visibility: hidden; }
.ticker-scroll:hover { animation-play-state: paused; }
@keyframes ticker-roll { 0% { transform: translate3d(0,0,0); } 100% { transform: translate3d(0,-50%,0); } }
.ticker-item {
  display: flex; align-items: center; gap: 8px; padding: 5px 18px;
  font-size: 0.79rem; border-bottom: 1px solid #f8fafc; height: 30px; box-sizing: border-box;
}
.ticker-item:hover { background: rgba(99,102,241,0.04); }
.ticker-new { background: rgba(99,102,241,0.06); animation: flash-in 0.6s ease-out; }
@keyframes flash-in { 0% { background: rgba(99,102,241,0.15); } 100% { background: rgba(99,102,241,0); } }
.tick-icon { font-size: 0.85rem; flex-shrink: 0; }
.tick-player { font-weight: 600; color: #6366f1; min-width: 56px; flex-shrink: 0; }
.tick-detail { color: #475569; flex: 1; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.tick-time { font-size: 0.66rem; color: #cbd5e1; white-space: nowrap; flex-shrink: 0; }
</style>
