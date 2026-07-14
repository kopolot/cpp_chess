# cpp_chess

A modular **C++20** chess engine with a CLI and a **Vue** web GUI.

## About this project

**A portfolio example on GitHub** — a demonstration of modular C++20 architecture, from the game engine to multiple interchangeable front-ends. The code is public and serves as a reference for separating game logic from UI, building a toolchain from scratch, and taking an MVP all the way to online play.

### What's included

- **Engine** — `ChessEngine<BoardType>` template, two board representations (`Board8x8`, `Board12x12`), full FIDE rules (castling, en passant, promotion, draws)
- **CLI** — console REPL with board selection and vs-AI mode
- **Web** — C++ REST API (`cpp-httplib`) + **Vue 3** frontend (Vite, Composition API)
- **AI** — minimax with alpha-beta pruning and three difficulty levels
- **Online multiplayer** — matchmaking queue, random 1v1 pairing, WebSocket sync

### Tech stack

| Layer | Technologies |
|-------|--------------|
| Language / standard | **C++20** (concepts, templates) |
| Build | **CMake**, **Conan 2** |
| Testing | **Google Test** |
| C++ backend | **fmt**, **cpp-httplib**, **nlohmann/json** |
| Frontend | **Vue 3**, **Vite** |
| Infrastructure | **Docker**, **nginx** (reverse proxy) |
| Code quality | clang-format, clang-tidy, Doxygen |

Project structure inspired by [ModernCppStarter](https://github.com/TheLartians/ModernCppStarter).

### Cursor

This project was built and developed in **[Cursor](https://cursor.com)** — an AI-powered IDE. Cursor helped with:

- the CMake + Conan scaffold and toolchain setup,
- game logic implementation and GTest tests,
- REST API, WebSocket, and Vue frontend integration,
- Docker Compose, nginx, and documentation (`AGENTS.md`, rules in `.cursor/rules/`).

This is not a "one-prompt prototype" — it is a real, iteratively built project where AI assisted with writing code, refactoring, and keeping the architecture consistent, while design decisions and review stayed with the author.

## Status

MVP complete: full game logic, AI, Vue frontend, and **online multiplayer** (random opponent). Details: [STATUS.md](STATUS.md).

## Requirements

- CMake ≥ 3.10
- Conan 2
- C++20 compiler (GCC/Clang)
- Node.js ≥ 20 + npm (frontend)
- clang-format, clang-tidy (optional)
- Docker + Compose (optional, for the web stack)

## Quick start — CLI

```bash
./bin/conan-install
./bin/cmake
./build/cpp_chess_cli
./build/cpp_chess_cli --board 12x12
./build/cpp_chess_cli --ai --difficulty medium
./build/cpp_chess_cli --help
```

REPL commands: `board`, `move e2e4`, `move e2 e4`, `new`, `help`, `exit`.

## Quick start — Web GUI

### Docker (recommended)

```bash
./bin/docker-up
# → http://localhost:8080
# nginx serves the built Vue app; /api → C++ backend
```

### Local (dev)

```bash
./build/cpp_chess_web -p 8081
cd web/frontend && npm install && npm run dev
# → http://localhost:5173  (Vite proxies /api → :8081)
```

### REST API

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/api/health` | healthcheck |
| `GET` | `/api/lobby` | queue / active games |
| `POST` | `/api/matchmaking/join` | find a random opponent |
| `GET` | `/api/matchmaking/status` | matchmaking status |
| `POST` | `/api/matchmaking/leave` | cancel search |
| `POST` | `/api/games` | new AI/local game (`vsAi`, `difficulty`, `playerColor`) |
| `GET` | `/api/games/:id` | game state |
| `POST` | `/api/games/:id/move` | `{ "from": "e2", "to": "e4", "promotion": "q" }` |
| `POST` | `/api/games/:id/reset` | reset (AI/local) |

AI levels: `1` easy, `2` medium, `3` hard. Online: `X-Player-Id` header, colors assigned randomly; moves via WebSocket `/ws` (disconnect = resign).

## Structure

| Directory | Description |
|-----------|-------------|
| `cli/` | Console REPL |
| `web/server/` | HTTP server (REST, C++) |
| `web/frontend/` | Vue 3 + Vite |
| `docker/` | Backend/frontend Dockerfiles, nginx, compose |
| `include/` | Engine, `game/`, `board/`, `ai/` |
| `src/board/` | `Board8x8`, `Board12x12` |
| `test/` | GTest |
| `bin/` | Build / test / docker scripts |

## Tests

```bash
cd build && ctest --output-on-failure
cd web/frontend && npm run build
```

## For AI agents

See [AGENTS.md](AGENTS.md) — context, conventions, API, and next steps.

## Design principles

- Engine and UI are interchangeable (`ChessEngine<BoardType>`)
- CLI selects the board (`8x8` / `12x12`); the web API currently uses `Board8x8`
- Frontend: Vue 3 (Composition API) + Vite; production build served as static files behind nginx
