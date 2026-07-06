<template>
  <div :style="finalStyle" :title="'#'+speciesId"></div>
</template>

<script setup>
import { computed } from 'vue'
import { ICON_SHEET } from '../../utils/iconSheet'
import { FORM_ICON_SHEET } from '../../utils/iconSheetForms'

const props = defineProps({
  speciesId: { type: Number, required: true },
  size: { type: String, default: 'md' },
})

const sizeMap = { sm: 32, md: 48, lg: 96, xl: 128 }
const dispSize = computed(() => sizeMap[props.size] || sizeMap.md)

const finalStyle = computed(() => {
  const px = dispSize.value

  // Form variants (ID >= 100000): use lookup from forms sheet
  if (props.speciesId >= 100000) {
    const entry = FORM_ICON_SHEET.mapping[String(props.speciesId)]
    if (entry) {
      const fw = FORM_ICON_SHEET.cols * FORM_ICON_SHEET.cellW * (px / FORM_ICON_SHEET.cellW)
      return {
        width: px + 'px', height: px + 'px',
        backgroundImage: `url(${FORM_ICON_SHEET.url})`,
        backgroundPosition: `-${entry.col * px}px -${entry.row * px}px`,
        backgroundSize: `${fw}px auto`,
        backgroundRepeat: 'no-repeat', imageRendering: 'pixelated',
      }
    }
    return { width: px + 'px', height: px + 'px', background: '#f3f4f6' }
  }

  // Base species: mathematical positioning
  const n = props.speciesId - 1
  const col = n < 0 ? 0 : n % ICON_SHEET.cols
  const row = n < 0 ? 0 : Math.floor(n / ICON_SHEET.cols)
  const bw = ICON_SHEET.cols * ICON_SHEET.cellW * (px / ICON_SHEET.cellW)
  return {
    width: px + 'px', height: px + 'px',
    backgroundImage: `url(${ICON_SHEET.url})`,
    backgroundPosition: `-${col * px}px -${row * px}px`,
    backgroundSize: `${bw}px auto`,
    backgroundRepeat: 'no-repeat', imageRendering: 'pixelated',
  }
})
</script>

<style>
/* placeholder */
</style>
