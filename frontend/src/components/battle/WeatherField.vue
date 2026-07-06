<template>
  <div v-if="hasContent" class="weather-field-bar">
    <span v-if="weather?.label" class="weather-tag" :class="weatherClass">{{ weather.label }}</span>
    <span v-if="weather?.duration" class="duration-tag">{{ weather.duration }}回合</span>
    <span v-if="field?.label" class="field-tag">{{ field.label }}</span>
  </div>
</template>

<script setup>
import { computed } from 'vue'

const props = defineProps({
  weather: { type: Object, default: null },
  field: { type: Object, default: null },
})

const hasContent = computed(() => {
  return (props.weather?.label) || (props.field?.label)
})

const weatherClass = computed(() => {
  const t = props.weather?.type || 0
  const map = { 1: 'rain', 2: 'sun', 3: 'sand', 4: 'hail', 5: 'snow', 6: 'strong-wind' }
  return map[t] || ''
})
</script>

<style scoped>
.weather-field-bar {
  position: absolute; top: 2%; left: 50%; transform: translateX(-50%); z-index: 10;
  display: flex; gap: 6px; align-items: center;
}
.weather-tag, .field-tag {
  padding: 2px 8px; border-radius: 6px; font-size: clamp(10px,1.2vw,14px);
  background: rgba(0,0,0,0.6); color: #ddd; font-weight: 600;
}
.rain { color: #60a5fa; } .sun { color: #facc15; }
.sand { color: #d4a373; } .hail { color: #a5f3fc; }
.snow { color: #e0f2fe; } .strong-wind { color: #c4b5fd; }
.duration-tag { font-size: clamp(9px,1vw,12px); color: #888; }
</style>
