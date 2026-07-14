const boardEl = document.getElementById("board");
const turnEl = document.getElementById("turn");
const messageEl = document.getElementById("message");
const hintEl = document.getElementById("hint");
const aiInfoEl = document.getElementById("ai-info");
const lobbyInfoEl = document.getElementById("lobby-info");
const promotionEl = document.getElementById("promotion");
const newGameBtn = document.getElementById("new-game");
const resetGameBtn = document.getElementById("reset-game");
const findOpponentBtn = document.getElementById("find-opponent");
const cancelSearchBtn = document.getElementById("cancel-search");
const modeEl = document.getElementById("mode");
const difficultyEl = document.getElementById("difficulty");
const playerColorEl = document.getElementById("player-color");
const playerNameEl = document.getElementById("player-name");
const difficultyWrap = document.getElementById("difficulty-wrap");
const playerColorWrap = document.getElementById("player-color-wrap");
const nameWrap = document.getElementById("name-wrap");

const API_BASE = "/api";
const PLAYER_KEY = "cpp_chess_player_id";
const NAME_KEY = "cpp_chess_player_name";
const GAME_KEY = "cpp_chess_game_id";

let gameId = null;
let state = null;
let meta = null;
let selected = null;
let pendingPromotion = null;
let lastAiMove = null;
let playerId = localStorage.getItem(PLAYER_KEY) || "";
let pollTimer = null;
let searchTimer = null;
let searching = false;
let knownVersion = -1;

const files = ["a", "b", "c", "d", "e", "f", "g", "h"];

function squareName(file, rankIndex) {
  return `${files[file]}${8 - rankIndex}`;
}

function parseSquare(name) {
  if (!name || name.length !== 2) return null;
  const file = files.indexOf(name[0]);
  const rank = Number(name[1]);
  if (file < 0 || Number.isNaN(rank) || rank < 1 || rank > 8) return null;
  return { file, rankIndex: 8 - rank };
}

function ensurePlayerId(id) {
  if (id) {
    playerId = id;
    localStorage.setItem(PLAYER_KEY, playerId);
  }
}

function playerName() {
  const name = (playerNameEl.value || "").trim();
  if (name) {
    localStorage.setItem(NAME_KEY, name);
    return name;
  }
  return localStorage.getItem(NAME_KEY) || "";
}

function syncModeUi() {
  const mode = modeEl.value;
  difficultyWrap.classList.toggle("hidden", mode !== "ai");
  playerColorWrap.classList.toggle("hidden", mode !== "ai");
  nameWrap.classList.toggle("hidden", mode !== "online");
  findOpponentBtn.classList.toggle("hidden", mode !== "online");
  cancelSearchBtn.classList.toggle("hidden", !(mode === "online" && searching));
  resetGameBtn.classList.toggle("hidden", mode === "online");
}

async function api(path, options = {}) {
  const headers = {
    "Content-Type": "application/json",
    ...(options.headers || {}),
  };
  if (playerId) {
    headers["X-Player-Id"] = playerId;
  }

  const response = await fetch(`${API_BASE}${path}`, { ...options, headers });
  const payload = await response.json().catch(() => ({}));
  if (!response.ok) {
    const error = new Error(payload.error || "Błąd API");
    error.payload = payload;
    error.status = response.status;
    throw error;
  }
  return payload;
}

function isOnline() {
  return meta?.mode === "online";
}

function isPlayerTurn() {
  if (!state || state.gameOver) return false;
  if (meta?.mode === "local") return true;
  if (meta?.mode === "ai" || meta?.mode === "online") {
    const player = meta.playerColor || "white";
    const turn = state.turn === 0 ? "white" : "black";
    return turn === player;
  }
  return true;
}

function stopPolling() {
  if (pollTimer) {
    clearInterval(pollTimer);
    pollTimer = null;
  }
}

function stopSearch() {
  searching = false;
  if (searchTimer) {
    clearInterval(searchTimer);
    searchTimer = null;
  }
  syncModeUi();
}

function startGamePolling() {
  stopPolling();
  if (!isOnline() || !gameId) return;
  pollTimer = setInterval(async () => {
    try {
      const payload = await api(`/games/${gameId}`);
      if (payload.meta?.version !== knownVersion) {
        applyPayload(payload);
      }
    } catch (error) {
      messageEl.textContent = error.message;
    }
  }, 1200);
}

function renderBoard() {
  boardEl.innerHTML = "";
  if (!state?.board) return;

  state.board.forEach((rank, rankIndex) => {
    rank.forEach((piece, file) => {
      const square = document.createElement("button");
      square.type = "button";
      square.className = `square ${(file + rankIndex) % 2 === 0 ? "light" : "dark"}`;
      square.dataset.square = squareName(file, rankIndex);

      if (selected === square.dataset.square) square.classList.add("selected");
      const highlight = lastAiMove || meta?.lastMove;
      if (highlight && square.dataset.square === highlight.slice(0, 2)) {
        square.classList.add("ai-from");
      }
      if (highlight && square.dataset.square === highlight.slice(2, 4)) {
        square.classList.add("ai-to");
      }

      if (piece) {
        const span = document.createElement("span");
        span.className = `piece ${piece.color}`;
        span.textContent = piece.unicode;
        square.appendChild(span);
      }

      square.addEventListener("click", () => onSquareClick(square.dataset.square));
      boardEl.appendChild(square);
    });
  });
}

function renderStatus() {
  if (!state) {
    turnEl.textContent = "Ładowanie…";
    return;
  }

  const turn = state.turnName || (state.turn === 0 ? "białe" : "czarne");
  if (state.gameOver) {
    turnEl.textContent = "Gra zakończona";
  } else if (!isPlayerTurn() && meta?.mode === "ai") {
    turnEl.textContent = `Tura AI (${turn})`;
  } else if (!isPlayerTurn() && meta?.mode === "online") {
    turnEl.textContent = `Tura przeciwnika (${turn})`;
  } else {
    turnEl.textContent = `Tura: ${turn}`;
  }

  messageEl.textContent = state.message || "";
  hintEl.textContent = state.enPassantHint || "";

  if (meta?.mode === "ai") {
    const level = meta.difficultyName || meta.difficulty;
    const you = meta.playerColor === "black" ? "czarne" : "białe";
    aiInfoEl.textContent = lastAiMove
      ? `AI (${level}) zagrało ${lastAiMove}. Grasz: ${you}.`
      : `Tryb AI (${level}). Grasz: ${you}.`;
  } else if (meta?.mode === "online") {
    const you = meta.playerColor === "black" ? "czarne" : "białe";
    aiInfoEl.textContent = `Online vs ${meta.opponentName || "przeciwnik"} · grasz: ${you}`;
  } else {
    aiInfoEl.textContent = "Tryb lokalny — dwóch graczy przy tym samym ekranie";
  }
}

function applyPayload(payload) {
  gameId = payload.gameId;
  state = payload.state;
  meta = payload.meta || null;
  lastAiMove = payload.aiMove || null;
  knownVersion = meta?.version ?? -1;
  selected = null;
  pendingPromotion = null;
  promotionEl.classList.add("hidden");

  if (meta?.mode === "ai") {
    modeEl.value = "ai";
    if (meta.difficulty) difficultyEl.value = String(meta.difficulty);
    if (meta.playerColor) playerColorEl.value = meta.playerColor;
  } else if (meta?.mode === "online") {
    modeEl.value = "online";
  } else if (meta?.mode === "local") {
    modeEl.value = "local";
  }

  localStorage.setItem(GAME_KEY, gameId);
  syncModeUi();
  renderBoard();
  renderStatus();
  startGamePolling();
}

async function refreshLobby() {
  try {
    const lobby = await api("/lobby");
    lobbyInfoEl.textContent = searching
      ? `Szukam przeciwnika… (kolejka: ${lobby.waiting})`
      : `Lobby: ${lobby.waiting} w kolejce · ${lobby.onlineGames} partii online`;
  } catch (_) {
    lobbyInfoEl.textContent = "";
  }
}

async function createAiOrLocalGame() {
  stopSearch();
  stopPolling();
  const mode = modeEl.value;
  const body =
    mode === "ai"
      ? {
          vsAi: true,
          difficulty: Number(difficultyEl.value),
          playerColor: playerColorEl.value,
        }
      : { vsAi: false };

  const payload = await api("/games", {
    method: "POST",
    body: JSON.stringify(body),
  });
  applyPayload(payload);
  await refreshLobby();
}

async function enterOnlineMatch(ticket) {
  stopSearch();
  ensurePlayerId(ticket.playerId);
  const payload = await api(`/games/${ticket.gameId}`);
  applyPayload(payload);
  messageEl.textContent = `Sparowano z ${ticket.opponentName || "przeciwnikiem"}!`;
  await refreshLobby();
}

async function pollSearch() {
  const status = await api("/matchmaking/status");
  ensurePlayerId(status.playerId);
  if (status.status === "matched") {
    await enterOnlineMatch(status);
    return;
  }
  await refreshLobby();
}

async function findOpponent() {
  modeEl.value = "online";
  syncModeUi();
  searching = true;
  syncModeUi();
  messageEl.textContent = "Szukam losowego przeciwnika…";

  const body = { name: playerName() || undefined };
  const payload = await api("/matchmaking/join", {
    method: "POST",
    body: JSON.stringify(body),
  });
  ensurePlayerId(payload.playerId);

  if (payload.status === "matched") {
    await enterOnlineMatch(payload);
    return;
  }

  stopSearch();
  searching = true;
  syncModeUi();
  searchTimer = setInterval(() => {
    pollSearch().catch((error) => {
      messageEl.textContent = error.message;
    });
  }, 1000);
  await refreshLobby();
}

async function cancelSearch() {
  stopSearch();
  try {
    await api("/matchmaking/leave", {
      method: "POST",
      body: JSON.stringify({}),
    });
  } catch (_) {
    /* ignore */
  }
  messageEl.textContent = "Anulowano szukanie.";
  await refreshLobby();
}

async function sendMove(from, to, promotion) {
  if (!isPlayerTurn()) {
    messageEl.textContent =
      meta?.mode === "online" ? "Teraz kolej przeciwnika." : "Teraz kolej AI.";
    return;
  }

  const body = { from, to };
  if (promotion) body.promotion = promotion;

  try {
    const payload = await api(`/games/${gameId}/move`, {
      method: "POST",
      body: JSON.stringify(body),
    });
    applyPayload(payload);
  } catch (error) {
    if (error.payload?.needsPromotion) {
      pendingPromotion = { from, to };
      promotionEl.classList.remove("hidden");
      return;
    }
    messageEl.textContent = error.message;
    selected = null;
    renderBoard();
  }
}

function onSquareClick(name) {
  if (!state || state.gameOver || !isPlayerTurn()) return;

  const coords = parseSquare(name);
  if (!coords) return;

  const piece = state.board[coords.rankIndex][coords.file];
  const currentTurn = state.turn === 0 ? "white" : "black";

  if (!selected) {
    if (piece && piece.color === currentTurn) {
      selected = name;
      renderBoard();
    }
    return;
  }

  if (selected === name) {
    selected = null;
    renderBoard();
    return;
  }

  if (piece && piece.color === currentTurn) {
    selected = name;
    renderBoard();
    return;
  }

  sendMove(selected, name);
}

async function resetGame() {
  if (modeEl.value === "online") {
    messageEl.textContent = "W online użyj „Szukaj przeciwnika” aby zacząć nową partię.";
    return;
  }
  if (!gameId) {
    await createAiOrLocalGame();
    return;
  }
  const payload = await api(`/games/${gameId}/reset`, { method: "POST" });
  applyPayload(payload);
}

async function onNewGame() {
  if (modeEl.value === "online") {
    await findOpponent();
    return;
  }
  await createAiOrLocalGame();
}

promotionEl.querySelectorAll("button[data-piece]").forEach((button) => {
  button.addEventListener("click", () => {
    if (!pendingPromotion) return;
    sendMove(pendingPromotion.from, pendingPromotion.to, button.dataset.piece);
  });
});

modeEl.addEventListener("change", syncModeUi);
newGameBtn.addEventListener("click", () => onNewGame().catch(showBootError));
findOpponentBtn.addEventListener("click", () => findOpponent().catch(showBootError));
cancelSearchBtn.addEventListener("click", () => cancelSearch().catch(showBootError));
resetGameBtn.addEventListener("click", () => resetGame().catch(showBootError));

function showBootError(error) {
  turnEl.textContent = "Błąd";
  messageEl.textContent = error.message;
}

async function boot() {
  const savedName = localStorage.getItem(NAME_KEY);
  if (savedName) playerNameEl.value = savedName;
  syncModeUi();
  setInterval(refreshLobby, 5000);

  try {
    const savedId = localStorage.getItem(GAME_KEY);
    if (savedId && playerId) {
      const payload = await api(`/games/${savedId}`);
      applyPayload(payload);
      await refreshLobby();
      return;
    }
  } catch (_) {
    localStorage.removeItem(GAME_KEY);
  }

  modeEl.value = "ai";
  syncModeUi();
  await createAiOrLocalGame();
}

boot().catch(showBootError);
