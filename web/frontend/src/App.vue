<script setup>
import { computed, onMounted, onUnmounted, ref, watch } from 'vue'
import ChessBoard from './components/ChessBoard.vue'
import { api, setPlayerId } from './api.js'

const NAME_KEY = 'cpp_chess_player_name'
const GAME_KEY = 'cpp_chess_game_id'
const FILES = ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h']

const mode = ref('ai')
const difficulty = ref('2')
const playerColor = ref('white')
const playerName = ref(localStorage.getItem(NAME_KEY) || '')

const gameId = ref(null)
const state = ref(null)
const meta = ref(null)
const selected = ref(null)
const pendingPromotion = ref(null)
const lastAiMove = ref(null)
const searching = ref(false)
const knownVersion = ref(-1)
const turnText = ref('Ładowanie…')
const message = ref('')
const hint = ref('')
const modeInfo = ref('')
const lobbyInfo = ref('')

let pollTimer = null
let searchTimer = null
let lobbyTimer = null

const highlightMove = computed(() => lastAiMove.value || meta.value?.lastMove || null)

function parseSquare(name) {
  if (!name || name.length !== 2) return null
  const file = FILES.indexOf(name[0])
  const rank = Number(name[1])
  if (file < 0 || Number.isNaN(rank) || rank < 1 || rank > 8) return null
  return { file, rankIndex: 8 - rank }
}

function isPlayerTurn() {
  if (!state.value || state.value.gameOver) return false
  if (meta.value?.mode === 'local') return true
  if (meta.value?.mode === 'ai' || meta.value?.mode === 'online') {
    const player = meta.value.playerColor || 'white'
    const turn = state.value.turn === 0 ? 'white' : 'black'
    return turn === player
  }
  return true
}

function stopPolling() {
  if (pollTimer) {
    clearInterval(pollTimer)
    pollTimer = null
  }
}

function stopSearch() {
  searching.value = false
  if (searchTimer) {
    clearInterval(searchTimer)
    searchTimer = null
  }
}

function startGamePolling() {
  stopPolling()
  if (meta.value?.mode !== 'online' || !gameId.value) return
  pollTimer = setInterval(async () => {
    try {
      const payload = await api(`/games/${gameId.value}`)
      if (payload.meta?.version !== knownVersion.value) {
        applyPayload(payload)
      }
    } catch (error) {
      message.value = error.message
    }
  }, 1200)
}

function renderStatus() {
  if (!state.value) {
    turnText.value = 'Ładowanie…'
    return
  }

  const turn = state.value.turnName || (state.value.turn === 0 ? 'białe' : 'czarne')
  if (state.value.gameOver) {
    turnText.value = 'Gra zakończona'
  } else if (!isPlayerTurn() && meta.value?.mode === 'ai') {
    turnText.value = `Tura AI (${turn})`
  } else if (!isPlayerTurn() && meta.value?.mode === 'online') {
    turnText.value = `Tura przeciwnika (${turn})`
  } else {
    turnText.value = `Tura: ${turn}`
  }

  hint.value = state.value.enPassantHint || ''
  if (!message.value || meta.value?.mode !== 'online') {
    // keep explicit matchmaking messages when online
  }
  if (state.value.message) {
    message.value = state.value.message
  }

  if (meta.value?.mode === 'ai') {
    const level = meta.value.difficultyName || meta.value.difficulty
    const you = meta.value.playerColor === 'black' ? 'czarne' : 'białe'
    modeInfo.value = lastAiMove.value
      ? `AI (${level}) zagrało ${lastAiMove.value}. Grasz: ${you}.`
      : `Tryb AI (${level}). Grasz: ${you}.`
  } else if (meta.value?.mode === 'online') {
    const you = meta.value.playerColor === 'black' ? 'czarne' : 'białe'
    modeInfo.value = `Online vs ${meta.value.opponentName || 'przeciwnik'} · grasz: ${you}`
  } else {
    modeInfo.value = 'Tryb lokalny — dwóch graczy przy tym samym ekranie'
  }
}

function applyPayload(payload) {
  gameId.value = payload.gameId
  state.value = payload.state
  meta.value = payload.meta || null
  lastAiMove.value = payload.aiMove || null
  knownVersion.value = meta.value?.version ?? -1
  selected.value = null
  pendingPromotion.value = null

  if (meta.value?.mode === 'ai') {
    mode.value = 'ai'
    if (meta.value.difficulty) difficulty.value = String(meta.value.difficulty)
    if (meta.value.playerColor) playerColor.value = meta.value.playerColor
  } else if (meta.value?.mode === 'online') {
    mode.value = 'online'
  } else if (meta.value?.mode === 'local') {
    mode.value = 'local'
  }

  localStorage.setItem(GAME_KEY, gameId.value)
  renderStatus()
  startGamePolling()
}

async function refreshLobby() {
  try {
    const lobby = await api('/lobby')
    lobbyInfo.value = searching.value
      ? `Szukam przeciwnika… (kolejka: ${lobby.waiting})`
      : `Lobby: ${lobby.waiting} w kolejce · ${lobby.onlineGames} partii online`
  } catch (_) {
    lobbyInfo.value = ''
  }
}

function storedName() {
  const name = playerName.value.trim()
  if (name) {
    localStorage.setItem(NAME_KEY, name)
    return name
  }
  return localStorage.getItem(NAME_KEY) || ''
}

async function createAiOrLocalGame() {
  stopSearch()
  stopPolling()
  message.value = ''
  const body =
    mode.value === 'ai'
      ? {
          vsAi: true,
          difficulty: Number(difficulty.value),
          playerColor: playerColor.value,
        }
      : { vsAi: false }

  const payload = await api('/games', {
    method: 'POST',
    body: JSON.stringify(body),
  })
  applyPayload(payload)
  await refreshLobby()
}

async function enterOnlineMatch(ticket) {
  stopSearch()
  setPlayerId(ticket.playerId)
  const payload = await api(`/games/${ticket.gameId}`)
  applyPayload(payload)
  message.value = `Sparowano z ${ticket.opponentName || 'przeciwnikiem'}!`
  await refreshLobby()
}

async function pollSearch() {
  const status = await api('/matchmaking/status')
  setPlayerId(status.playerId)
  if (status.status === 'matched') {
    await enterOnlineMatch(status)
    return
  }
  await refreshLobby()
}

async function findOpponent() {
  mode.value = 'online'
  searching.value = true
  message.value = 'Szukam losowego przeciwnika…'

  const payload = await api('/matchmaking/join', {
    method: 'POST',
    body: JSON.stringify({ name: storedName() || undefined }),
  })
  setPlayerId(payload.playerId)

  if (payload.status === 'matched') {
    await enterOnlineMatch(payload)
    return
  }

  searching.value = true
  searchTimer = setInterval(() => {
    pollSearch().catch((error) => {
      message.value = error.message
    })
  }, 1000)
  await refreshLobby()
}

async function cancelSearch() {
  stopSearch()
  try {
    await api('/matchmaking/leave', { method: 'POST', body: JSON.stringify({}) })
  } catch (_) {
    /* ignore */
  }
  message.value = 'Anulowano szukanie.'
  await refreshLobby()
}

async function sendMove(from, to, promotion) {
  if (!isPlayerTurn()) {
    message.value =
      meta.value?.mode === 'online' ? 'Teraz kolej przeciwnika.' : 'Teraz kolej AI.'
    return
  }

  const body = { from, to }
  if (promotion) body.promotion = promotion

  try {
    const payload = await api(`/games/${gameId.value}/move`, {
      method: 'POST',
      body: JSON.stringify(body),
    })
    applyPayload(payload)
  } catch (error) {
    if (error.payload?.needsPromotion) {
      pendingPromotion.value = { from, to }
      return
    }
    message.value = error.message
    selected.value = null
  }
}

function onSquareClick(name) {
  if (!state.value || state.value.gameOver || !isPlayerTurn()) return
  const coords = parseSquare(name)
  if (!coords) return

  const piece = state.value.board[coords.rankIndex][coords.file]
  const currentTurn = state.value.turn === 0 ? 'white' : 'black'

  if (!selected.value) {
    if (piece && piece.color === currentTurn) selected.value = name
    return
  }
  if (selected.value === name) {
    selected.value = null
    return
  }
  if (piece && piece.color === currentTurn) {
    selected.value = name
    return
  }
  sendMove(selected.value, name)
}

async function resetGame() {
  if (mode.value === 'online') {
    message.value = 'W online użyj „Szukaj przeciwnika” aby zacząć nową partię.'
    return
  }
  if (!gameId.value) {
    await createAiOrLocalGame()
    return
  }
  const payload = await api(`/games/${gameId.value}/reset`, { method: 'POST' })
  applyPayload(payload)
}

async function onNewGame() {
  if (mode.value === 'online') {
    await findOpponent()
    return
  }
  await createAiOrLocalGame()
}

watch(playerName, (value) => {
  if (value.trim()) localStorage.setItem(NAME_KEY, value.trim())
})

onMounted(async () => {
  lobbyTimer = setInterval(refreshLobby, 5000)
  try {
    const savedId = localStorage.getItem(GAME_KEY)
    const playerId = localStorage.getItem('cpp_chess_player_id')
    if (savedId && playerId) {
      const payload = await api(`/games/${savedId}`)
      applyPayload(payload)
      await refreshLobby()
      return
    }
  } catch (_) {
    localStorage.removeItem(GAME_KEY)
  }

  mode.value = 'ai'
  await createAiOrLocalGame()
})

onUnmounted(() => {
  stopPolling()
  stopSearch()
  if (lobbyTimer) clearInterval(lobbyTimer)
})
</script>

<template>
  <main class="layout">
    <header class="header">
      <h1>cpp_chess</h1>
      <p class="subtitle">Vue GUI · AI, lokalnie albo online</p>
    </header>

    <section class="panel status-panel">
      <div class="status-line">{{ turnText }}</div>
      <div class="status-line message">{{ message }}</div>
      <div class="status-line hint">{{ hint }}</div>
      <div class="status-line hint">{{ modeInfo }}</div>
      <div class="status-line hint">{{ lobbyInfo }}</div>
    </section>

    <section class="board-wrap">
      <ChessBoard
        :board="state?.board"
        :selected="selected"
        :highlight-move="highlightMove"
        @square-click="onSquareClick"
      />
    </section>

    <section class="panel settings">
      <label class="setting">
        Tryb
        <select v-model="mode">
          <option value="ai">Przeciw AI</option>
          <option value="local">Lokalnie (2 graczy)</option>
          <option value="online">Online — losowy przeciwnik</option>
        </select>
      </label>
      <label v-if="mode === 'ai'" class="setting">
        Poziom AI
        <select v-model="difficulty">
          <option value="1">Łatwy</option>
          <option value="2">Średni</option>
          <option value="3">Trudny</option>
        </select>
      </label>
      <label v-if="mode === 'ai'" class="setting">
        Twój kolor (AI)
        <select v-model="playerColor">
          <option value="white">Białe</option>
          <option value="black">Czarne</option>
        </select>
      </label>
      <label v-if="mode === 'online'" class="setting">
        Nick
        <input v-model="playerName" type="text" maxlength="32" placeholder="Gracz" />
      </label>
    </section>

    <section class="panel controls">
      <button type="button" @click="onNewGame().catch((e) => (message = e.message))">
        Nowa gra
      </button>
      <button
        v-if="mode === 'online'"
        type="button"
        class="secondary"
        @click="findOpponent().catch((e) => (message = e.message))"
      >
        Szukaj przeciwnika
      </button>
      <button
        v-if="mode === 'online' && searching"
        type="button"
        class="secondary"
        @click="cancelSearch().catch((e) => (message = e.message))"
      >
        Anuluj szukanie
      </button>
      <button
        v-if="mode !== 'online'"
        type="button"
        @click="resetGame().catch((e) => (message = e.message))"
      >
        Reset
      </button>
    </section>

    <section v-if="pendingPromotion" class="panel promotion">
      <p>Wybierz figurę promocji:</p>
      <div class="promotion-buttons">
        <button type="button" @click="sendMove(pendingPromotion.from, pendingPromotion.to, 'q')">
          ♕ Hetman
        </button>
        <button type="button" @click="sendMove(pendingPromotion.from, pendingPromotion.to, 'r')">
          ♖ Wieża
        </button>
        <button type="button" @click="sendMove(pendingPromotion.from, pendingPromotion.to, 'b')">
          ♗ Goniec
        </button>
        <button type="button" @click="sendMove(pendingPromotion.from, pendingPromotion.to, 'n')">
          ♘ Skoczek
        </button>
      </div>
    </section>
  </main>
</template>
