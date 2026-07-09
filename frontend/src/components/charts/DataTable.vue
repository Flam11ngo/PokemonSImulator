<template>
  <div class="dt-card">
    <div class="dt-header">
      <h3 class="dt-title">{{ title }}</h3>
      <div class="dt-controls">
        <input v-model="search" @input="applyFilter" :placeholder="searchPlaceholder"
          class="dt-search" />
        <select v-model="sortBy" @change="onSortChange" class="dt-sort-select">
          <option v-for="col in sortableCols" :key="col.key" :value="col.key">{{ col.label }}</option>
        </select>
        <button @click="toggleDir" class="dt-sort-dir">
          {{ dir === 'desc' ? '↓' : '↑' }}
        </button>
      </div>
    </div>

    <!-- Loading skeleton -->
    <div v-if="loading" class="dt-loading">
      <div v-for="i in 8" :key="i" class="dt-skeleton" :style="{ animationDelay: `${i * 80}ms` }" />
    </div>

    <!-- Empty -->
    <div v-else-if="filtered.length === 0" class="dt-empty">
      <div class="dt-empty-icon">📭</div>
      <p class="dt-empty-text">{{ emptyText }}</p>
    </div>

    <!-- Table -->
    <div v-else class="dt-table-wrap" :class="{ 'dt-fixed': stableHeight }"
      :style="stableHeight ? { height: stableHeightPx + 'px' } : {}">
      <table ref="tableEl" class="dt-table" :class="{ 'dt-stable': stableHeight, 'no-anim': _skip }">
        <thead>
          <tr class="dt-thead-tr">
            <th class="dt-th-rank">#</th>
            <th v-for="col in columns" :key="col.key" class="dt-th" :class="col.class || ''">
              {{ col.label }}
            </th>
          </tr>
        </thead>
        <TransitionGroup name="rank-slide" tag="tbody">
          <tr v-for="(row, i) in pagedData" :key="row[idKey]"
            class="dt-row"
            :class="{
              'dt-row-clickable': clickable,
              'dt-row-selected': clickable && selectedId === row[idKey],
              'dt-row-slot': row._slot,
              [`dt-rank-${(page - 1) * perPage + i + 1}`]: true,
            }"
            @click="handleRowClick(row)">
            <!-- Rank cell with medal for top 3 -->
            <td class="dt-rank-cell">
              <span v-if="(page - 1) * perPage + i + 1 === 1" class="dt-medal dt-medal-gold">🥇</span>
              <span v-else-if="(page - 1) * perPage + i + 1 === 2" class="dt-medal dt-medal-silver">🥈</span>
              <span v-else-if="(page - 1) * perPage + i + 1 === 3" class="dt-medal dt-medal-bronze">🥉</span>
              <span v-else class="dt-rank-num">{{ (page - 1) * perPage + i + 1 }}</span>
              <!-- Rank change arrow -->
              <span v-if="rankArrow(row)" class="dt-rank-arrow" :class="rankArrow(row)">{{ rankArrowIcon(row) }}</span>
            </td>
            <td v-for="(col, ci) in columns" :key="col.key"
              class="dt-cell"
              :class="[col.class || '', ci === 0 ? 'dt-cell-first' : '', ci === columns.length - 1 ? 'dt-cell-last' : '']"
              v-html="formatCell(row, col, i)" />
          </tr>
        </TransitionGroup>
      </table>

      <!-- Pagination -->
      <div v-if="totalPages > 1" class="dt-pagination">
        <span class="dt-page-info">{{ filtered.length }} 条结果，第 {{ page }}/{{ totalPages }} 页</span>
        <div class="dt-page-btns">
          <button v-for="p in pageButtons" :key="p" @click="page = p"
            class="dt-page-btn" :class="{ 'dt-page-active': p === page }">
            {{ p }}
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, watch } from 'vue'

const props = defineProps({
  title: { type: String, default: '数据表' },
  columns: { type: Array, default: () => [] },
  rows: { type: Array, default: () => [] },
  loading: { type: Boolean, default: false },
  searchPlaceholder: { type: String, default: '搜索...' },
  emptyText: { type: String, default: '暂无可显示的数据' },
  idKey: { type: String, default: 'id' },
  clickable: { type: Boolean, default: false },
  selectedId: { type: [String, Number], default: null },
  defaultSort: { type: String, default: '' },
  stableHeight: { type: Boolean, default: false },
  stableHeightPx: { type: Number, default: 500 },
})

const emit = defineEmits(['row-click'])

const search = ref('')
const sortBy = ref('')
const dir = ref('desc')
const page = ref(1)
const perPage = 10

const sortableCols = computed(() =>
  props.columns.filter(c => c.sortable !== false).map(c => ({ key: c.key, label: c.label }))
)

const tableEl = ref(null)
const filtered = ref([])
const _display = ref(null)

function applyFilter() {
  const q = search.value.toLowerCase().trim()
  filtered.value = q
    ? props.rows.filter(r => props.columns.some(c => String(r[c.key] || '').toLowerCase().includes(q)))
    : [...props.rows]
  page.value = 1
  applySort()
}

function onSortChange() {
  _queueId++
  _skip.value = true
  const col = props.columns.find(c => c.key === sortBy.value)
  dir.value = (col && col.spark) ? 'desc' : 'asc'
  const target = [...filtered.value]
  const fn = col?.sortFn || ((a, b) => {
    const va = a[sortBy.value], vb = b[sortBy.value]
    if (typeof va === 'number' && typeof vb === 'number') return va - vb
    return String(va).localeCompare(String(vb))
  })
  target.sort((a, b) => dir.value === 'desc' ? fn(b, a) : fn(a, b))
  filtered.value = target
  setTimeout(() => { _skip.value = false }, 100)
}

watch(page, () => {
  _skip.value = true
  setTimeout(() => { _skip.value = false }, 100)
})

function applySort() {
  if (!sortBy.value) return
  for (const col of props.columns) {
    if (col.key === sortBy.value) continue
    if (!col.spark && !col.format && col.sortable !== false) {
      filtered.value.sort((a, b) => String(a[col.key] || '').localeCompare(String(b[col.key] || '')))
    }
  }
  const col = props.columns.find(c => c.key === sortBy.value)
  const fn = col?.sortFn || ((a, b) => {
    const va = a[sortBy.value], vb = b[sortBy.value]
    if (typeof va === 'number' && typeof vb === 'number') return va - vb
    return String(va).localeCompare(String(vb))
  })
  filtered.value.sort((a, b) => dir.value === 'desc' ? fn(b, a) : fn(a, b))
}

function toggleDir() { dir.value = dir.value === 'desc' ? 'asc' : 'desc'; applySort() }

// ── Rank movement tracking ──────────────────────────
const prevOrder = ref(new Map())
const moveDirs = ref(new Map())

watch(() => props.rows, () => {
  try {
    const rows = props.rows || []
    const idKey = props.idKey || 'id'
    const old = prevOrder.value
    const cur = new Map()
    rows.forEach((row, i) => { if (row && row[idKey] != null) cur.set(row[idKey], i) })
    const dirs = new Map()
    cur.forEach((newIdx, id) => {
      if (!old.has(id)) return
      const oldIdx = old.get(id)
      if (newIdx < oldIdx) dirs.set(id, 'up')
      else if (newIdx > oldIdx) dirs.set(id, 'down')
      else dirs.set(id, 'same')
    })
    moveDirs.value = dirs
    prevOrder.value = cur
    if (dirs.size > 0) setTimeout(() => moveDirs.value = new Map(), 8000)
  } catch {}
}, { immediate: true, deep: false })

function rankArrow(row) {
  try { return moveDirs.value.get(row?.[props.idKey || 'id']) || '' } catch { return '' }
}
function rankArrowIcon(row) {
  const d = rankArrow(row)
  if (d === 'up') return '▲'
  if (d === 'down') return '▼'
  if (d === 'same') return '─'
  return ''
}

function handleRowClick(row) {
  if (props.clickable) emit('row-click', row)
}

const pagedData = computed(() => {
  const source = _display.value || filtered.value
  const start = (page.value - 1) * perPage
  const items = source.slice(start, start + perPage)
  if (props.stableHeight) {
    while (items.length < perPage) {
      items.push({ _slot: true, [props.idKey || 'id']: `_slot_${items.length}` })
    }
  }
  return items
})

const totalPages = computed(() => Math.max(1, Math.ceil(filtered.value.length / perPage)))

const pageButtons = computed(() => {
  if (totalPages.value <= 7) return Array.from({ length: totalPages.value }, (_, i) => i + 1)
  const pages = []
  for (let i = 1; i <= 3; i++) pages.push(i)
  if (page.value > 4) pages.push('...')
  for (let i = Math.max(4, page.value - 1); i <= Math.min(totalPages.value - 3, page.value + 1); i++) pages.push(i)
  if (page.value < totalPages.value - 3) pages.push('...')
  for (let i = totalPages.value - 2; i <= totalPages.value; i++) pages.push(i)
  return [...new Set(pages)]
})

function formatCell(row, col, i) {
  const value = row[col.key]
  if (col.format) return col.format(value, row)
  // Icon sprite sheet (local, instant, no network requests)
  if (col.iconSheet) {
    const sid = parseInt(row.species_id) || parseInt(row.name) || 0
    const name = row[col.key] || ''
    if (sid > 0 && sid < 100000) {
      const ICON_SIZE = 24
      const COLS = 16
      const n = sid - 1
      const col = n % COLS
      const rowNum = Math.floor(n / COLS)
      const bw = COLS * ICON_SIZE
      return `<div class="dt-cell-name"><div style="width:${ICON_SIZE}px;height:${ICON_SIZE}px;background-image:url(/icons-sheet.png);background-position:-${col*ICON_SIZE}px -${rowNum*ICON_SIZE}px;background-size:${bw}px auto;background-repeat:no-repeat;image-rendering:pixelated;border-radius:2px;flex-shrink:0"></div><span class="dt-name-text">${name}</span></div>`
    }
    return `<div class="dt-cell-name"><span class="dt-name-text">${name}</span></div>`
  }
  if (col.image) {
    const url = row[col.image] || value
    const fallback = col.fallback || ''
    return url
      ? `<div class="dt-cell-name"><img src="${url}" alt="" class="dt-sprite" onerror="this.style.display='none'" /><span class="dt-name-text">${fallback || row[col.key] || ''}</span></div>`
      : (fallback || value || '')
  }
  if (col.spark && typeof value === 'number') {
    const max = Math.max(...filtered.value.map(r => r[col.key] || 0), 1)
    const pct = max > 0 ? (value / max) * 100 : 0
    return `<div class="dt-spark"><div class="dt-spark-track"><div class="dt-spark-bar" style="width:${pct}%"></div></div><span class="dt-spark-val">${Number(value).toLocaleString()}</span></div>`
  }
  return value
}

// ── FLIP animation engine ───────────────────────────
let _queueId = 0
const _skip = ref(false)
let _tick = 0

async function _tickUpdate(target) {
  const tick = ++_tick
  const idKey = props.idKey || 'id'
  const old = [...filtered.value]
  // Don't animate first load
  if (old.length === 0) { filtered.value = target; applySort(); return }

  filtered.value = target
  applySort()

  // Only animate moves visible on current page
  const start = (page.value - 1) * perPage
  const end = start + perPage
  const pageSet = new Set()
  for (let i = start; i < end; i++) pageSet.add(i)

  const moved = []
  old.forEach((row, oldIdx) => {
    const newIdx = target.findIndex(r => r?.[idKey] === row?.[idKey])
    if (newIdx !== -1 && newIdx !== oldIdx && (pageSet.has(oldIdx) || pageSet.has(newIdx))) {
      moved.push({ row, oldIdx, newIdx })
    }
  })
  moved.sort((a, b) => a.newIdx - b.newIdx)

  // FLIP: capture old → apply new → CSS transitions slide
  const map = new Map(target.map(r => [r?.[idKey], r]).filter(([k]) => k != null))
  const displayArr = old.map(r => {
    const next = map.get(r?.[idKey])
    return next ? { ...next } : r
  })

  _display.value = displayArr
  await new Promise(r => requestAnimationFrame(r))
  if (tick !== _tick) return

  _display.value = target
  await new Promise(r => setTimeout(r, 1000))
  if (tick === _tick) _display.value = null
}

watch(() => props.rows, (rows) => {
  // Skip if data unchanged (prevents redundant FLIP animations)
  const prevIds = filtered.value.map(r => r[props.idKey || 'id']).join(',')
  const nextIds = rows.map(r => r[props.idKey || 'id']).join(',')
  const target = [...rows]
  if (sortableCols.value.length > 0) {
    if (!sortBy.value) {
      const preferred = props.defaultSort || sortableCols.value[0].key
      sortBy.value = sortableCols.value.find(c => c.key === preferred)?.key || sortableCols.value[0].key
    }
    for (const c of props.columns) {
      if (c.key === sortBy.value) continue
      if (!c.spark && !c.format && c.sortable !== false) {
        target.sort((a, b) => String(a[c.key] || '').localeCompare(String(b[c.key] || '')))
      }
    }
    const col = props.columns.find(c => c.key === sortBy.value)
    const fn = col?.sortFn || ((a, b) => {
      const va = a[sortBy.value], vb = b[sortBy.value]
      if (typeof va === 'number' && typeof vb === 'number') return va - vb
      return String(va).localeCompare(String(vb))
    })
    target.sort((a, b) => dir.value === 'desc' ? fn(b, a) : fn(a, b))
  }
  // Only animate if order/content actually changed
  const prevLen = filtered.value.length
  if (prevIds !== nextIds || prevLen !== rows.length) {
    _tickUpdate(target)
  } else {
    filtered.value = target
  }
}, { immediate: true })
</script>

<style>
/* ── Card ─────────────────────────────────── */
.dt-card {
  background: rgba(255,255,255,0.85);
  backdrop-filter: blur(16px);
  border: 1px solid rgba(255,255,255,0.6);
  border-radius: 20px;
  padding: 22px;
  box-shadow: 0 4px 24px rgba(0,0,0,0.06), 0 1px 4px rgba(0,0,0,0.04);
  transition: box-shadow 0.3s;
}
.dt-card:hover {
  box-shadow: 0 8px 32px rgba(0,0,0,0.1), 0 2px 8px rgba(0,0,0,0.06);
}

/* ── Header ───────────────────────────────── */
.dt-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 16px;
  flex-wrap: wrap;
  gap: 8px;
}
.dt-title {
  font-size: 0.95rem;
  font-weight: 700;
  color: #1e293b;
  letter-spacing: -0.01em;
}
.dt-controls {
  display: flex;
  gap: 6px;
  align-items: center;
}
.dt-search {
  font-size: 0.75rem;
  padding: 6px 14px;
  border-radius: 99px;
  background: #f8fafc;
  border: 1px solid #e2e8f0;
  width: 150px;
  transition: all 0.2s;
  outline: none;
}
.dt-search:focus {
  border-color: #818cf8;
  box-shadow: 0 0 0 3px rgba(129,140,248,0.12);
  background: #fff;
}
.dt-sort-select {
  font-size: 0.7rem;
  padding: 6px 10px;
  border-radius: 99px;
  background: #f8fafc;
  border: 1px solid #e2e8f0;
  cursor: pointer;
  outline: none;
}
.dt-sort-dir {
  font-size: 0.75rem;
  padding: 4px 8px;
  border-radius: 99px;
  background: #f8fafc;
  border: 1px solid #e2e8f0;
  cursor: pointer;
  transition: all 0.15s;
}
.dt-sort-dir:hover { background: #e2e8f0; }

/* ── Loading ──────────────────────────────── */
.dt-loading { display: flex; flex-direction: column; gap: 6px; }
.dt-skeleton {
  height: 38px;
  border-radius: 10px;
  background: linear-gradient(120deg, #f1f5f9 25%, #e2e8f0 50%, #f1f5f9 75%);
  background-size: 200% 100%;
  animation: dt-shimmer 2s infinite;
}

/* ── Empty ────────────────────────────────── */
.dt-empty { text-align: center; padding: 40px 0; color: #94a3b8; }
.dt-empty-icon { font-size: 2.5rem; margin-bottom: 8px; }
.dt-empty-text { font-size: 0.8rem; }

/* ── Table ────────────────────────────────── */
.dt-table-wrap { overflow-x: auto; }
.dt-fixed { overflow: hidden; }
.dt-table { width: 100%; font-size: 0.85rem; border-collapse: separate; border-spacing: 0 3px; }
.dt-thead-tr { text-align: left; }
.dt-th {
  font-size: 0.7rem;
  font-weight: 500;
  color: #94a3b8;
  padding-bottom: 8px;
  text-transform: uppercase;
  letter-spacing: 0.04em;
}
.dt-th-rank { width: 44px; padding-bottom: 8px; }

/* ── Rows ─────────────────────────────────── */
.dt-row {
  transition: background 0.2s;
}
.dt-row-slot { visibility: hidden; }
.dt-row-clickable { cursor: pointer; }
.dt-row:hover .dt-cell {
  background: #f0f4ff;
  box-shadow: 0 2px 12px rgba(99,102,241,0.1);
}
.dt-row-selected .dt-cell {
  background: #eef2ff;
  box-shadow: 0 0 0 2px rgba(99,102,241,0.25);
}

/* ── Rank cell ────────────────────────────── */
.dt-rank-cell {
  display: flex;
  align-items: center;
  gap: 2px;
  padding: 9px 0;
  min-width: 42px;
}
.dt-col-icon {
  width: 44px;
  padding: 6px 4px !important;
  text-align: center;
}
.dt-col-icon > div {
  margin: 0 auto;
}
.dt-col-item-icon {
  width: 40px;
  padding: 6px 2px !important;
  text-align: center;
}
.dt-col-item-icon > div {
  margin: 0 auto;
}
.dt-medal { font-size: 1.15rem; line-height: 1; }
.dt-medal-gold { filter: drop-shadow(0 1px 2px rgba(245,158,11,0.4)); }
.dt-medal-silver { filter: drop-shadow(0 1px 2px rgba(148,163,184,0.4)); }
.dt-medal-bronze { filter: drop-shadow(0 1px 2px rgba(180,83,9,0.3)); }
.dt-rank-num {
  font-size: 0.75rem;
  color: #94a3b8;
  font-family: 'SF Mono', 'JetBrains Mono', monospace;
  font-weight: 500;
  min-width: 22px;
  text-align: center;
}
.dt-rank-arrow {
  font-size: 0.6rem;
  margin-left: 2px;
  transition: opacity 0.3s;
}
.dt-rank-arrow.up { color: #22c55e; }
.dt-rank-arrow.down { color: #ef4444; }
.dt-rank-arrow.same { color: #cbd5e1; }

/* ── Cells (pill shape) ───────────────────── */
.dt-cell {
  background: #fff;
  padding: 9px 10px;
  border: none !important;
  box-shadow: 0 1px 3px rgba(0,0,0,0.05);
  transition: all 0.25s ease;
}
.dt-cell-first {
  border-radius: 10px 0 0 10px !important;
  padding-left: 14px !important;
}
.dt-cell-last {
  border-radius: 0 10px 10px 0 !important;
  padding-right: 14px !important;
}

/* ── Cell: name with sprite ───────────────── */
.dt-cell-name {
  display: flex;
  align-items: center;
  gap: 6px;
  line-height: 1;
}
.dt-sprite {
  width: 24px;
  height: 24px;
  object-fit: contain;
  image-rendering: pixelated;
  border-radius: 2px;
  flex-shrink: 0;
}
.dt-name-text {
  color: #1e293b;
  font-weight: 600;
  font-size: 0.85rem;
}

/* ── Cell: spark bar ──────────────────────── */
.dt-spark {
  display: flex;
  align-items: center;
  gap: 8px;
}
.dt-spark-track {
  flex: 1;
  height: 10px;
  border-radius: 99px;
  background: #f1f5f9;
  overflow: hidden;
}
.dt-spark-bar {
  height: 100%;
  border-radius: 99px;
  background: linear-gradient(90deg, #fbbf24, #f59e0b, #ef4444);
  width: var(--bar-width);
  transition: width 1.2s cubic-bezier(0.22, 1, 0.36, 1);
  box-shadow: 0 0 8px rgba(245,158,11,0.35);
}
.dt-spark-val {
  font-size: 0.78rem;
  color: #64748b;
  width: 64px;
  text-align: right;
  font-family: 'SF Mono', 'JetBrains Mono', monospace;
  font-weight: 500;
}

/* ── Pagination ───────────────────────────── */
.dt-pagination {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-top: 12px;
  padding-top: 10px;
  border-top: 1px solid #f1f5f9;
}
.dt-page-info { font-size: 0.7rem; color: #94a3b8; }
.dt-page-btns { display: flex; gap: 4px; }
.dt-page-btn {
  width: 28px;
  height: 28px;
  font-size: 0.7rem;
  border-radius: 99px;
  border: none;
  background: #f8fafc;
  color: #64748b;
  cursor: pointer;
  transition: all 0.15s;
  font-weight: 500;
}
.dt-page-btn:hover { background: #e2e8f0; }
.dt-page-active { background: #6366f1; color: #fff; }
.dt-page-active:hover { background: #4f46e5; }

/* ── FLIP slide animations ────────────────── */
.rank-slide-move,
.rank-slide-enter-active,
.rank-slide-leave-active {
  transition: all 0.8s cubic-bezier(0.22, 1, 0.36, 1);
}
.rank-slide-enter-from {
  opacity: 0;
  transform: translateY(40px);
}
.rank-slide-leave-to {
  opacity: 0;
  transform: translateY(-40px);
}
.dt-stable .rank-slide-enter-active {
  transition: opacity 0.4s ease 0.3s, transform 0.4s ease 0.3s;
}
.dt-stable .rank-slide-leave-active {
  transition: opacity 0.3s ease, transform 0.3s ease;
}
.dt-stable .rank-slide-enter-from {
  opacity: 0;
  transform: translateY(50px);
}
.dt-stable .rank-slide-leave-to {
  opacity: 0;
  transform: translateY(50px);
}

/* Disable animation during sort/page */
.no-anim .rank-slide-move,
.no-anim .rank-slide-enter-active,
.no-anim .rank-slide-leave-active {
  transition: none !important;
}

@keyframes dt-shimmer {
  0% { background-position: 200% 0; }
  100% { background-position: -200% 0; }
}
</style>
