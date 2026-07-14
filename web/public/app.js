const boardEl = document.getElementById("board");
const turnEl = document.getElementById("turn");
const messageEl = document.getElementById("message");
const hintEl = document.getElementById("hint");
const promotionEl = document.getElementById("promotion");
const newGameBtn = document.getElementById("new-game");
const resetGameBtn = document.getElementById("reset-game");

const API_BASE = "/api";

let gameId = null;
let state = null;
let selected = null;
let pendingPromotion = null;

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
  turnEl.textContent = state.gameOver ? "Gra zakończona" : `Tura: ${turn}`;
  messageEl.textContent = state.message || "";
  hintEl.textContent = state.enPassantHint || "";
}

function applyState(nextState) {
  state = nextState;
  selected = null;
  pendingPromotion = null;
  promotionEl.classList.add("hidden");
  renderBoard();
  renderStatus();
}

async function createGame() {
  const payload = await api("/games", { method: "POST" });
  gameId = payload.gameId;
  localStorage.setItem("cpp_chess_game_id", gameId);
  applyState(payload.state);
}

async function loadGame(id) {
  const payload = await api(`/games/${id}`);
  gameId = payload.gameId;
  applyState(payload.state);
}

async function sendMove(from, to, promotion) {
  const body = { from, to };
  if (promotion) {
    body.promotion = promotion;
  }

  try {
    const payload = await api(`/games/${gameId}/move`, {
      method: "POST",
      body: JSON.stringify(body),
    });
    applyState(payload.state);
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
  if (!state || state.gameOver) {
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
  applyState(payload.state);
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
