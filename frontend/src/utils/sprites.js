/**
 * Pokemon sprite URLs — sprite-sheet-first, GIF fallback.
 * Complete sheet: 16 cols x 96×96 cells, covers IDs 1-1025 (~2033 KB).
 * Sprites sourced from pokemondex.com (Gen 5 style, static PNG).
 */
import { ICON_SHEET } from './iconSheet'

const COLS = ICON_SHEET.cols
const CELL = ICON_SHEET.cellW  // 96, square
const SHEET_URL = ICON_SHEET.url

/**
 * Return CSS background style for a sprite-sheet icon cell.
 */
export function getIconStyle(speciesId, displaySize = 96) {
  if (!speciesId || speciesId < 1) return {}
  const n = speciesId - 1
  const col = n % COLS
  const row = Math.floor(n / COLS)
  const scale = displaySize / CELL
  const sheetW = COLS * CELL * scale
  return {
    width: displaySize + 'px',
    height: displaySize + 'px',
    backgroundImage: `url(${SHEET_URL})`,
    backgroundPosition: `-${col * displaySize}px -${row * displaySize}px`,
    backgroundSize: `${sheetW}px auto`,
    backgroundRepeat: 'no-repeat',
    imageRendering: 'pixelated',
    display: 'inline-block',
    flexShrink: '0',
  }
}

// CDN fallback (rarely needed — sheet covers all 1-1025)
const HOME_CDN = (id) =>
  `https://raw.githubusercontent.com/PokeAPI/master/pokemon/other/home/${id}.png`

export function getIconSprite(speciesId) { return SHEET_URL }
export function getHomeSprite(speciesId) { return HOME_CDN(speciesId) }
export function getAnimatedSprite(speciesId) { return SHEET_URL }
export function getStaticSprite(speciesId) { return SHEET_URL }
export function getShowdownSprite(speciesId) { return SHEET_URL }
