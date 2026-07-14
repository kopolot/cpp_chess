const API_BASE = '/api'
const PLAYER_KEY = 'cpp_chess_player_id'

let playerId = localStorage.getItem(PLAYER_KEY) || ''

export function getPlayerId() {
  return playerId
}

export function setPlayerId(id) {
  if (!id) return
  playerId = id
  localStorage.setItem(PLAYER_KEY, playerId)
}

export async function api(path, options = {}) {
  const headers = {
    'Content-Type': 'application/json',
    ...(options.headers || {}),
  }
  if (playerId) {
    headers['X-Player-Id'] = playerId
  }

  const response = await fetch(`${API_BASE}${path}`, { ...options, headers })
  const payload = await response.json().catch(() => ({}))
  if (!response.ok) {
    const error = new Error(payload.error || 'Błąd API')
    error.payload = payload
    error.status = response.status
    throw error
  }
  return payload
}
