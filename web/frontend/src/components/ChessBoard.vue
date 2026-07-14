<script setup>
defineProps({
  board: { type: Array, default: null },
  selected: { type: String, default: null },
  highlightMove: { type: String, default: null },
})

const emit = defineEmits(['square-click'])

const files = ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h']

function squareName(file, rankIndex) {
  return `${files[file]}${8 - rankIndex}`
}

function isHighlight(name, move, part) {
  if (!move || move.length < 4) return false
  return part === 'from' ? name === move.slice(0, 2) : name === move.slice(2, 4)
}
</script>

<template>
  <div class="board" aria-label="Szachownica">
    <template v-if="board">
      <template v-for="(rank, rankIndex) in board" :key="rankIndex">
        <button
          v-for="(piece, file) in rank"
          :key="`${rankIndex}-${file}`"
          type="button"
          class="square"
          :class="{
            light: (file + rankIndex) % 2 === 0,
            dark: (file + rankIndex) % 2 !== 0,
            selected: selected === squareName(file, rankIndex),
            'ai-from': isHighlight(squareName(file, rankIndex), highlightMove, 'from'),
            'ai-to': isHighlight(squareName(file, rankIndex), highlightMove, 'to'),
          }"
          @click="emit('square-click', squareName(file, rankIndex))"
        >
          <span v-if="piece" class="piece" :class="piece.color">{{ piece.unicode }}</span>
        </button>
      </template>
    </template>
  </div>
</template>
