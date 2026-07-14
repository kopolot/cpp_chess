const boardEl = document.getElementById("board");
const turnEl = document.getElementById("turn");
const messageEl = document.getElementById("message");
const hintEl = document.getElementById("hint");
const aiInfoEl = document.getElementById("ai-info");
const promotionEl = document.getElementById("promotion");
const newGameBtn = document.getElementById("new-game");
const resetGameBtn = document.getElementById("reset-game");
const vsAiEl = document.getElementById("vs-ai");
const difficultyEl = document.getElementById("difficulty");
const playerColorEl = document.getElementById("player-color");

const API_BASE = "/api";

let gameId = null;
let state = null;
let meta = null;
let selected = null;
let pendingPromotion = null;
let lastAiMove = null;

const files = ["a", "b", "c", "d", "e", "f", "g", "h"];

function squareName(file, rankIndex) {
  const rank = 8 - rankIndex;
  return `${files[file]}${rank}`;
}

function parseSquare(name) {
  if (!name || name.length !== 2) {
    return null;
  }
  const file = files.indexOf(name[0]);
  const rank = Number(name[1]);
  if (file < 0 || Number.isNaN(rank) || rank < 1 || rank > 8) {
    return null;
  }
  return { file, rankIndex: 8 - rank };
}

function gameOptions() {
  return {
    vsAi: vsAiEl.checked,
    difficulty: Number(difficultyEl.value),
    playerColor: playerColorEl.value,
  };
}

async function api(path, options = {}) {
  const response = await fetch(`${API_BASE}${path}`, {
    headers: { "Content-Type": "application/json", ...(options.headers || {}) },
    ...options,
  });

  const payload = await response.json().catch(() => ({}));
  if (!response.ok) {
    const error = new Error(payload.error || "Błąd API");
    error.payload = payload;
    error.status = response.status;
    throw error;
  }
  return payload;
}

function isPlayerTurn() {
  if (!state || state.gameOver) {
    return false;
  }
  if (!meta?.vsAi) {
    return true;
  }
  const player = meta.playerColor || "white";
  const turn = state.turn === 0 ? "white" : "black";
  return turn === player;
}

function renderBoard() {
  boardEl.innerHTML = "";
  if (!state?.board) {
    return;
  }

  state.board.forEach((rank, rankIndex) => {
    rank.forEach((piece, file) => {
      const square = document.createElement("button");
      square.type = "button";
      square.className = `square ${(file + rankIndex) % 2 === 0 ? "light" : "dark"}`;
      square.dataset.square = squareName(file, rankIndex);

      if (selected === square.dataset.square) {
        square.classList.add("selected");
      }
      if (lastAiMove && square.dataset.square === lastAiMove.slice(0, 2)) {
        square.classList.add("ai-from");
      }
      if (lastAiMove && square.dataset.square === lastAiMove.slice(2, 4)) {
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
  } else if (meta?.vsAi && !isPlayerTurn()) {
    turnEl.textContent = `Tura AI (${turn})`;
  } else {
    turnEl.textContent = `Tura: ${turn}`;
  }

  messageEl.textContent = state.message || "";
  hintEl.textContent = state.enPassantHint || "";

  if (meta?.vsAi) {
    const level = meta.difficultyName || meta.difficulty;
    const you = meta.playerColor === "black" ? "czarne" : "białe";
    aiInfoEl.textContent = lastAiMove
      ? `AI (${level}) zagrało ${lastAiMove}. Grasz: ${you}.`
      : `Tryb AI (${level}). Grasz: ${you}.`;
  } else {
    aiInfoEl.textContent = "Tryb: dwóch graczy";
  }
}

function applyPayload(payload) {
  gameId = payload.gameId;
  state = payload.state;
  meta = payload.meta || null;
  lastAiMove = payload.aiMove || null;
  selected = null;
  pendingPromotion = null;
  promotionEl.classList.add("hidden");
  if (meta) {
    vsAiEl.checked = !!meta.vsAi;
    if (meta.difficulty) {
      difficultyEl.value = String(meta.difficulty);
    }
    if (meta.playerColor) {
      playerColorEl.value = meta.playerColor;
    }
  }
  renderBoard();
  renderStatus();
}

async function createGame() {
  const payload = await api("/games", {
    method: "POST",
    body: JSON.stringify(gameOptions()),
  });
  localStorage.setItem("cpp_chess_game_id", payload.gameId);
  applyPayload(payload);
}

async function loadGame(id) {
  const payload = await api(`/games/${id}`);
  applyPayload(payload);
}

async function sendMove(from, to, promotion) {
  if (!isPlayerTurn()) {
    messageEl.textContent = "Teraz kolej AI.";
    return;
  }

  const body = { from, to };
  if (promotion) {
    body.promotion = promotion;
  }

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
  if (!state || state.gameOver || !isPlayerTurn()) {
    return;
  }

  const coords = parseSquare(name);
  if (!coords) {
    return;
  }

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
  if (!gameId) {
    await createGame();
    return;
  }
  const payload = await api(`/games/${gameId}/reset`, { method: "POST" });
  applyPayload(payload);
}

promotionEl.querySelectorAll("button[data-piece]").forEach((button) => {
  button.addEventListener("click", () => {
    if (!pendingPromotion) {
      return;
    }
    sendMove(pendingPromotion.from, pendingPromotion.to, button.dataset.piece);
  });
});

newGameBtn.addEventListener("click", createGame);
resetGameBtn.addEventListener("click", resetGame);

async function boot() {
  try {
    const savedId = localStorage.getItem("cpp_chess_game_id");
    if (savedId) {
      await loadGame(savedId);
      return;
    }
  } catch (_) {
    localStorage.removeItem("cpp_chess_game_id");
  }
  await createGame();
}

boot().catch((error) => {
  turnEl.textContent = "Nie udało się połączyć z API.";
  messageEl.textContent = error.message;
});
