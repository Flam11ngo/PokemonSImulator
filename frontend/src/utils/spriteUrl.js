/**
 * Unified sprite URL helper.
 * Strategy: local GIF → local PNG → Showdown CDN
 * All components should use these functions for consistent fallback.
 */

const CDN_ANI = 'https://play.pokemonshowdown.com/sprites/ani'
const CDN_GEN5 = 'https://play.pokemonshowdown.com/sprites/gen5'

/** Normalize to Showdown ID: lowercase, strip all non-alphanumeric chars. */
export function showdownId(name) {
  if (!name) return ''
  return String(name).toLowerCase().replace(/[^a-z0-9]/g, '')
}

/** Front sprite for a Pokemon by species ID. */
export function frontSprite(speciesId) {
  if (!speciesId) return ''
  // Form aliases (virtual IDs >= 100000) always PNG
  if (speciesId >= 100000) return `/${speciesId}.png`
  // Gen 9 (900+) and known PNG-only: try PNG first
  if (speciesId >= 900) return `/${speciesId}.png`
  return `/ani/${speciesId}.gif`
}

/** Back sprite for a Pokemon by species ID. */
export function backSprite(speciesId) {
  if (!speciesId) return ''
  return `/back/${speciesId}.gif`
}

/** Front sprite from Showdown CDN by Pokemon name. */
export function cdnFrontSprite(name) {
  if (!name) return ''
  return `${CDN_ANI}/${showdownId(name)}.gif`
}

/** Back sprite CDN fallback. */
export function cdnBackSprite(speciesId, name) {
  if (name) return `${CDN_ANI}-back/${showdownId(name)}.gif`
  return `${CDN_ANI}-back/${showdownId(String(speciesId))}.gif`
}

/** Gen5 PNG CDN fallback for newer Pokemon. Preserves hyphens for form names. */
export function cdnGen5Sprite(name) {
  if (!name) return ''
  const n = String(name).toLowerCase().replace(/['.\s]/g, '').replace(/é/g, 'e')
  return `${CDN_GEN5}/${n}.png`
}

/**
 * On-error handler for <img> tags.
 * GIF → PNG → Gen5 CDN → hide.
 * @param {Event} e - DOM error event
 */
export function spriteFallback(e) {
  const el = e.target
  const src = el.getAttribute('src') || el.src
  if (el._fbStep === undefined) el._fbStep = 0

  // Step 0→1: GIF → PNG (local)
  if (el._fbStep === 0 && src.includes('.gif')) {
    el._fbStep = 1
    el.src = src.replace(/\.gif$/, '.png')
    return
  }
  // Step 1→2: Try gen5 CDN (for newer mons)
  if (el._fbStep <= 1) {
    el._fbStep = 2
    const m = src.match(/\/(\d+)\.(gif|png)/)
    if (m) {
      const id = parseInt(m[1])
      el.src = `${CDN_GEN5}/${id}.png`
      return
    }
  }
  // Step 2→3: hide
  el.style.display = 'none'
}

/**
 * On-error handler for back sprites (with extra CDN fallback).
 */
export function backSpriteFallback(e, speciesName) {
  const el = e.target
  if (el._fbStep === undefined) el._fbStep = 0

  if (el._fbStep === 0) {
    el._fbStep = 1
    el.src = el.src.replace(/\.gif$/, '.png')
    return
  }
  if (el._fbStep === 1 && speciesName) {
    el._fbStep = 2
    el.src = cdnBackSprite(0, speciesName)
    return
  }
  if (el._fbStep <= 2) {
    el._fbStep = 3
    el.src = el.src.replace(/\.(gif|png)$/, '.png')
    el.src = el.src.replace('/back/', '/gen5/')
    return
  }
  el.style.display = 'none'
}
