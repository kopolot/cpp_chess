import { getPlayerId, setPlayerId } from './api.js'

function wsUrl() {
  const proto = window.location.protocol === 'https:' ? 'wss:' : 'ws:'
  return `${proto}//${window.location.host}/ws`
}

export function createGameSocket({ onMessage, onClose, onError }) {
  let socket = null
  let closedByUs = false
  let helloResolved = false

  function send(obj) {
    if (!socket || socket.readyState !== WebSocket.OPEN) {
      throw new Error('Brak połączenia WebSocket.')
    }
    socket.send(JSON.stringify(obj))
  }

  function connect() {
    return new Promise((resolve, reject) => {
      closedByUs = false
      helloResolved = false
      socket = new WebSocket(wsUrl())

      const fail = (err) => {
        if (!helloResolved) {
          reject(err instanceof Error ? err : new Error('WebSocket error'))
        }
        onError?.(err)
      }

      socket.addEventListener('open', () => {
        try {
          send({ type: 'hello', playerId: getPlayerId() || undefined })
        } catch (error) {
          fail(error)
        }
      })

      socket.addEventListener('message', (event) => {
        let data
        try {
          data = JSON.parse(event.data)
        } catch (_) {
          return
        }
        if (data.type === 'hello_ok' && data.playerId && !helloResolved) {
          helloResolved = true
          setPlayerId(data.playerId)
          resolve(socket)
        }
        onMessage?.(data)
      })

      socket.addEventListener('error', fail)

      socket.addEventListener('close', () => {
        if (!helloResolved) {
          reject(new Error('WebSocket zamknięty przed hello_ok'))
        }
        if (!closedByUs) {
          onClose?.()
        }
      })
    })
  }

  function joinGame(gameId) {
    send({ type: 'join_game', gameId })
  }

  function move(from, to, promotion) {
    const body = { type: 'move', from, to }
    if (promotion) body.promotion = promotion
    send(body)
  }

  function resign() {
    send({ type: 'resign' })
  }

  function close() {
    closedByUs = true
    if (socket && socket.readyState <= WebSocket.OPEN) {
      socket.close()
    }
    socket = null
  }

  return { connect, joinGame, move, resign, close, send }
}
