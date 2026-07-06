import { ref, watch } from 'vue'

const LANG_KEY = 'pokemon_locale'
const saved = localStorage.getItem(LANG_KEY) || 'zh'

export const locale = ref(saved)
export const isZh = () => locale.value === 'zh'

export function toggleLocale() {
  locale.value = locale.value === 'zh' ? 'en' : 'zh'
}

watch(locale, (v) => {
  localStorage.setItem(LANG_KEY, v)
})

/** Return display name: prefer Chinese if available, fall back to English */
export function displayName(item) {
  if (!item) return ''
  if (isZh() && item.chineseName) return item.chineseName
  return item.name || ''
}

/** Return description in current language */
export function displayDesc(item) {
  if (!item) return ''
  if (isZh() && item.chineseDesc) return item.chineseDesc
  return item.description || ''
}
