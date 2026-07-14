# AGENTS.md — cpp_chess

Instrukcje dla agentów AI pracujących w tym repozytorium.

## Czym jest ten projekt

Modułowy **silnik szachowy w C++20** z wymiennymi front-endami:

- **Silnik** — logika gry, reprezentacja planszy, ruchy figur, szach/mat/pat
- **CLI** — konsolowy REPL
- **Web** — REST API (`cpp-httplib`) + frontend **Vue 3** (Vite) za proxy nginx
- Silnik i UI są **oddzielone** — front-endy tylko wołają API silnika / HTTP

## Aktualny etap (lipiec 2026)

| Obszar | Status |
|--------|--------|
| Toolchain (CMake, Conan, Doxygen, clang-format, clang-tidy) | Gotowe |
| CLI — REPL (`board`, `move`, `new`, `help`, `exit`) | Gotowe |
| Wybór planszy przy starcie (`--board 8x8\|12x12`) | Gotowe |
| Web API (`cpp-httplib`) + REST JSON | Gotowe |
| Web GUI (Vue 3 + Vite) | Gotowe |
| Docker Compose (nginx + backend + build Vue) | Gotowe |
| AI opponent (minimax, poziomy 1–3) | Gotowe |
| Online multiplayer (kolejka + losowy 1v1) | Gotowe |
| Online WebSocket + resign przy disconnect | Gotowe |
| `ChessEngine<BoardType>` — pełna logika gry | Gotowe |
| `Board8x8` + `Board12x12` (concept `PlayableBoard`) | Gotowe |
| Szach, mat, pat, roszada, en passant, promocja, remisy | Gotowe |
| Testy GTest | ~45 testów (`test_board`, `test_moves`, `test_ai`) |

**Następny logiczny krok:** notacja SAN / PGN, zegar, mocniejszy AI.

## Struktura katalogów

```
cpp_chess/
├── cli/main.cpp              # CLI (REPL + --board)
├── web/
│   ├── server/main.cpp       # REST API (cpp-httplib)
│   └── frontend/             # Vue 3 + Vite (src/, package.json)
├── docker/
│   ├── Dockerfile.backend    # obraz cpp_chess_web
│   ├── Dockerfile.frontend   # npm build → nginx
│   ├── docker-compose.yml    # web:8080 + backend:8081
│   └── nginx.conf            # / → Vue dist, /api/ → backend
├── include/
│   ├── chess_engine.hpp      # Szablon ChessEngine<BoardType>
│   ├── board/                # Board8x8, Board12x12, concept PlayableBoard
│   ├── game/                 # Logika gry + game_api.hpp (JSON)
│   ├── ai/                   # Ocena pozycji + minimax (poziomy trudności)
│   └── type/chess_piece.hpp
├── src/board/                # Implementacje plansz
├── test/                     # test_board.cpp, test_moves.cpp, test_ai.cpp
├── bin/                      # conan-install, cmake, docker-up, …
├── conanfile.txt             # fmt, gtest, cpp-httplib, nlohmann_json
├── CMakeLists.txt
├── STATUS.md                 # Krótkie podsumowanie etapu
└── note.md                   # Notatki autora
```

## Build i testy

Wymagania: **CMake ≥ 3.10**, **Conan 2**, **GCC/Clang z C++20**. Opcjonalnie: clang-format, clang-tidy, Docker.

```bash
# Pełna instalacja zależności + build Release (czyści build/)
./bin/conan-install

# Build Debug (gdy build/ już istnieje)
./bin/cmake

# Build Release
./bin/cmake-release

# Testy
cd build && ctest --output-on-failure

# Dokumentacja Doxygen
./bin/make-doc   # wynik w html/

# Web stack (nginx + backend)
./bin/docker-up  # http://localhost:8080
```

Binaria: `build/cpp_chess_cli`, `build/cpp_chess_web`, `build/tests`.

Opcja CMake `BUILD_WEB` (domyślnie ON) buduje serwer HTTP. Lokalnie: `./build/cpp_chess_web -p 8081`.

Frontend (dev):
```bash
cd web/frontend && npm install && npm run dev   # http://localhost:5173
```

**clangd:** `CMAKE_EXPORT_COMPILE_COMMANDS ON` → `build/compile_commands.json`. Symlink w korzeniu:
`ln -sf build/compile_commands.json compile_commands.json` (nie commituj symlinku).

## Web API (skrót)

| Metoda | Ścieżka | Body / wynik |
|--------|---------|--------------|
| GET | `/api/health` | `{ ok, service }` |
| GET | `/api/lobby` | statystyki kolejki |
| POST | `/api/matchmaking/join` | matchmaking; header `X-Player-Id` |
| GET | `/api/matchmaking/status` | status kolejki / mecz |
| POST | `/api/matchmaking/leave` | wyjdź z kolejki |
| POST | `/api/games` | body opcjonalne: `{ vsAi, difficulty, playerColor }` |
| GET | `/api/games/:id` | `{ gameId, state, meta }` |
| POST | `/api/games/:id/move` | `{ from, to, promotion? }` → może zwrócić `aiMove` |
| POST | `/api/games/:id/reset` | reset (nie w online) |

`state` zawiera m.in. `board`, `turn`, `result`, `gameOver`, `message`. Serializacja: `include/game/game_api.hpp`.

AI: `include/ai/` — material eval + alpha-beta. CLI: `--ai --difficulty 1|2|3`.

Online: wielu graczy, wiele sesji w pamięci, losowe kolory; sync gry przez **WebSocket `/ws`** (disconnect = poddanie).

## Konwencje kodu

- **Standard:** C++20
- **Formatowanie:** Google style (`.clang-format`)
- **Nagłówki:** `include/`, implementacje plansz w `src/board/`
- **Zależności:** tylko przez Conan (`conanfile.txt`)
- **Język komentarzy:** polski lub angielski — spójnie w pliku
- **Kolor figur:** `0` = biały, `1` = czarny (`chess_piece.hpp`)
- Nie dodawaj `build/`, `html/`, `latex/`, `compile_commands.json` do gita

## Decyzje architektoniczne

Reprezentacje planszy (oba spełniają `PlayableBoard`):

1. **`Board8x8`** — granice przez sprawdzanie indeksów 0..7
2. **`Board12x12`** — obramowanie wykrywane kolizją, offset 2

CLI: `--board 8x8` (domyślnie) lub `--board 12x12`. Web API obecnie używa `Board8x8`. Logika gry jest wspólna w `ChessEngine<BoardType>`.

Frontend: **Vue 3 + Vite** w `web/frontend/`. Dev: `npm run dev` (proxy `/api` → `:8081`). Produkcja: `Dockerfile.frontend` buduje `dist/` i serwuje przez nginx.

## Co robić / czego unikać

**Rób:**
- Małe, skupione zmiany zgodne z modularnością silnik ↔ UI
- Logikę gry w `include/game/` (szablony na `BoardType`) lub `.cpp` w `src/` gdy potrzeba
- Serializację HTTP w `game_api.hpp` / `web/server/` — nie w silniku
- UI w `web/frontend/` (Vue SFC); nie wracaj do vanilla `web/public/`
- Prawdziwe testy GTest dla ruchów, szachu, roszady itd.
- Uruchamiaj `./bin/cmake` po większych zmianach C++; `npm run build` po zmianach GUI

**Unikaj:**
- Mieszania logiki silnika z `cli/main.cpp` lub `web/server/main.cpp`
- Commitowania artefaktów builda / `compile_commands.json` / `node_modules` / `dist`
- Rozbudowywania CMake lub Vue bez potrzeby
- Dodawania ciężkich libów frontendowych (Vue Router/Pinia) bez wyraźnej potrzeby

## Cursor — reguły i hooki

- `.cursor/rules/` — reguły projektu (C++20, struktura, build)
- `.cursor/hooks/` — auto-format `clang-format` po edycji plików C++
- `.cursor/mcp.json` — serwery MCP projektu (obecnie puste; dodaj gdy potrzebne)

## Przydatne linki

- [ModernCppStarter](https://github.com/TheLartians/ModernCppStarter)
- [Modern CMake](https://cliutils.gitlab.io/modern-cmake/README.html)
- [Conan 2 docs](https://docs.conan.io/2/)
- [cpp-httplib](https://github.com/yhirose/cpp-httplib)
