# cpp_chess

Modułowy silnik szachowy w **C++20** z CLI i **Vue** web GUI.

## O projekcie

**Przykładowy projekt portfolio** na GitHub — demonstracja modułowej architektury w C++20, od silnika gry po kilka wymiennych front-endów. Kod jest publiczny i służy jako referencja: jak oddzielić logikę od UI, zbudować toolchain od zera i doprowadzić MVP do gry online.

### Co tu jest

- **Silnik** — szablon `ChessEngine<BoardType>`, dwie reprezentacje planszy (`Board8x8`, `Board12x12`), pełna logika FIDE (roszada, en passant, promocja, remisy)
- **CLI** — konsolowy REPL z wyborem planszy i trybem vs AI
- **Web** — REST API w C++ (`cpp-httplib`) + frontend **Vue 3** (Vite, Composition API)
- **AI** — minimax z alpha-beta i trzema poziomami trudności
- **Multiplayer online** — kolejka matchmakingu, losowy 1v1, synchronizacja przez WebSocket

### Stack technologiczny

| Warstwa | Technologie |
|---------|-------------|
| Język / standard | **C++20** (concepts, szablony) |
| Build | **CMake**, **Conan 2** |
| Testy | **Google Test** |
| Backend C++ | **fmt**, **cpp-httplib**, **nlohmann/json** |
| Frontend | **Vue 3**, **Vite** |
| Infrastruktura | **Docker**, **nginx** (reverse proxy) |
| Jakość kodu | clang-format, clang-tidy, Doxygen |

Inspiracja struktury projektu: [ModernCppStarter](https://github.com/TheLartians/ModernCppStarter).

### Cursor

Projekt powstał i rozwijał się w **[Cursor](https://cursor.com)** — IDE z agentami AI. Cursor pomagał m.in. przy:

- szkielecie CMake + Conan i konfiguracji toolchainu,
- implementacji logiki gry i testów GTest,
- REST API, WebSocket i integracji z frontendem Vue,
- Docker Compose, nginx i dokumentacji (`AGENTS.md`, reguły w `.cursor/rules/`).

To nie jest „wygenerowany jednym promptem” prototyp — to realny, budowany iteracyjnie projekt, w którym AI wspierało pisanie kodu, refaktoryzację i utrzymanie spójności architektury, a decyzje projektowe i review pozostawały po stronie autora.

## Status

MVP: pełna logika gry, AI, Vue frontend oraz **online multiplayer** (losowy przeciwnik). Szczegóły: [STATUS.md](STATUS.md).

## Wymagania

- CMake ≥ 3.10
- Conan 2
- Kompilator z C++20 (GCC/Clang)
- Node.js ≥ 20 + npm (frontend)
- clang-format, clang-tidy (opcjonalnie)
- Docker + Compose (opcjonalnie, do web stacku)

## Szybki start — CLI

```bash
./bin/conan-install
./bin/cmake
./build/cpp_chess_cli
./build/cpp_chess_cli --board 12x12
./build/cpp_chess_cli --ai --difficulty medium
./build/cpp_chess_cli --help
```

Komendy w REPL: `board`, `move e2e4`, `move e2 e4`, `new`, `help`, `exit`.

## Szybki start — Web GUI

### Docker (zalecane)

```bash
./bin/docker-up
# → http://localhost:8080
# nginx serwuje zbudowany Vue; /api → backend C++
```

### Lokalnie (dev)

```bash
./build/cpp_chess_web -p 8081
cd web/frontend && npm install && npm run dev
# → http://localhost:5173  (Vite proxy /api → :8081)
```

### REST API

| Metoda | Ścieżka | Opis |
|--------|---------|------|
| `GET` | `/api/health` | healthcheck |
| `GET` | `/api/lobby` | kolejka / aktywne partie |
| `POST` | `/api/matchmaking/join` | szukaj losowego przeciwnika |
| `GET` | `/api/matchmaking/status` | status matchmakingu |
| `POST` | `/api/matchmaking/leave` | anuluj szukanie |
| `POST` | `/api/games` | nowa gra AI/lokalna (`vsAi`, `difficulty`, `playerColor`) |
| `GET` | `/api/games/:id` | stan gry |
| `POST` | `/api/games/:id/move` | `{ "from": "e2", "to": "e4", "promotion": "q" }` |
| `POST` | `/api/games/:id/reset` | reset (AI/local) |

Poziomy AI: `1` łatwy, `2` średni, `3` trudny. Online: header `X-Player-Id`, kolory losowane; ruchy przez WebSocket `/ws` (rozłączenie = poddanie).

## Struktura

| Katalog | Opis |
|---------|------|
| `cli/` | Konsolowy REPL |
| `web/server/` | Serwer HTTP (REST, C++) |
| `web/frontend/` | Vue 3 + Vite |
| `docker/` | Dockerfile backendu/frontendu, nginx, compose |
| `include/` | Silnik, `game/`, `board/`, `ai/` |
| `src/board/` | `Board8x8`, `Board12x12` |
| `test/` | GTest |
| `bin/` | Skrypty build / test / docker |

## Testy

```bash
cd build && ctest --output-on-failure
cd web/frontend && npm run build
```

## Dla agentów AI

Zobacz [AGENTS.md](AGENTS.md) — kontekst, konwencje, API i następne kroki.

## Założenia

- Silnik i UI są wymienne (`ChessEngine<BoardType>`)
- CLI wybiera planszę (`8x8` / `12x12`); web API na razie używa `Board8x8`
- Frontend: Vue 3 (Composition API) + Vite; produkcja budowana do static files za nginxiem
