<template>
  <div class="pokemon-sprite" :class="sizeClass">
    <!-- GIF first (animated), fallback to sprite sheet on error -->
    <img
      v-if="!gifFailed"
      :src="gifUrl"
      :alt="`#${speciesId}`"
      class="pixelated"
      @error="gifFailed = true"
      loading="lazy"
    />
    <IconSprite v-else :species-id="speciesId" :size="spriteSheetSize" />
  </div>
</template>

<script setup>
import { computed, ref, watch } from 'vue'
import IconSprite from './IconSprite.vue'

const props = defineProps({
  speciesId: { type: Number, required: true },
  animated: { type: Boolean, default: true },
  size: { type: String, default: 'md' }, // sm, md, lg, xl
  back: { type: Boolean, default: false },
})

const gifFailed = ref(false)

// Reset when species changes
watch(() => props.speciesId, () => { gifFailed.value = false })

const gifUrl = computed(() => `/${props.speciesId}.gif`)

// Map PokemonSprite size to IconSprite size
const spriteSheetSize = computed(() => {
  switch (props.size) {
    case 'sm': return 'sm'     // 32px
    case 'lg': return 'lg'     // 96px
    case 'xl': return 'xl'     // 128px
    default: return 'md'       // 48px
  }
})

const sizeClass = computed(() => {
  switch (props.size) {
    case 'sm': return 'w-8 h-8'
    case 'lg': return 'w-24 h-24'
    case 'xl': return 'w-32 h-32'
    default: return 'w-16 h-16'
  }
})
</script>

<style scoped>
.pokemon-sprite {
  display: flex;
  align-items: center;
  justify-content: center;
}
.pokemon-sprite img {
  width: 100%;
  height: 100%;
  object-fit: contain;
}
.pixelated {
  image-rendering: pixelated;
}
</style>
