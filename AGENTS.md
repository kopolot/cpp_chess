# AGENTS.md — cpp_chess

Instrukcje dla agentów AI pracujących w tym repozytorium.

## Czym jest ten projekt

Modułowy **silnik szachowy w C++20** z wymiennymi front-endami:

- **Silnik** — logika gry, reprezentacja planszy, ruchy figur, szach/mat/pat
- **CLI** — konsolowy REPL
- **Web** — REST API (`cpp-httplib`) + vanilla HTML/CSS/JS za proxy nginx
- Silnik i UI są **oddzielone** — front-endy tylko wołają API silnika / HTTP

## Aktualny etap (lipiec 2026)

| Obszar | Status |
|--------|--------|
| Toolchain (CMake, Conan, Doxygen, clang-format, clang-tidy) | Gotowe |
| CLI — REPL (`board`, `move`, `new`, `help`, `exit`) | Gotowe |
| Wybór planszy przy starcie (`--board 8x8\|12x12`) | Gotowe |
| Web API (`cpp-httplib`) + REST JSON | Gotowe |
| Web GUI (vanilla HTML/CSS/JS) | Gotowe |
| Docker Compose (nginx + backend) | Gotowe |
| AI opponent (minimax, poziomy 1–3) | Gotowe |
| Online multiplayer (kolejka + losowy 1v1) | Gotowe |
| `ChessEngine<BoardType>` — pełna logika gry | Gotowe |
| `Board8x8` + `Board12x12` (concept `PlayableBoard`) | Gotowe |
| Szach, mat, pat, roszada, en passant, promocja, remisy | Gotowe |
| Testy GTest | ~45 testów (`test_board`, `test_moves`, `test_ai`) |
| AI opponent (minimax, poziomy 1–3) | Gotowe |

**Następny logiczny krok:** notacja SAN / PGN, zegar, mocniejszy AI.

## Struktura katalogów

```
cpp_chess/
├── cli/main.cpp              # CLI (REPL + --board)
├── web/
│   ├── server/main.cpp       # REST API (cpp-httplib)
│   └── public/               # Frontend statyczny (index.html, app.js, style.css)
├── docker/
│   ├── Dockerfile.backend    # obraz cpp_chess_web
│   ├── docker-compose.yml    # nginx:8080 + backend:8081
│   └── nginx.conf            # / → static, /api/ → backend
├── include/
│   ├── chess_engine.hpp      # Szablon ChessEngine<BoardType>
│   ├── board/                # Board8x8, Board12x12, concept PlayableBoard
│   ├── game/                 # Logika gry + game_api.hpp (JSON)
│   ├── ai/                   # Ocena pozycji + minimax (poziomy trudności)
│   └── type/chess_piece.hpp
├── src/board/                # Implementacje plansz
├── test/                     # test_board.cpp, test_moves.cpp
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

Online: wielu graczy, wiele sesji w pamięci, losowe kolory przy sparowaniu, frontend polluje stan (~1.2 s).

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

Frontend web jest celowo bez frameworka (bez Vue/React) — mały, statyczny UI za nginxiem.

## Co robić / czego unikać

**Rób:**
- Małe, skupione zmiany zgodne z modularnością silnik ↔ UI
- Logikę gry w `include/game/` (szablony na `BoardType`) lub `.cpp` w `src/` gdy potrzeba
- Serializację HTTP w `game_api.hpp` / `web/server/` — nie w silniku
- Prawdziwe testy GTest dla ruchów, szachu, roszady itd.
- Uruchamiaj `./bin/cmake` po większych zmianach

**Unikaj:**
- Mieszania logiki silnika z `cli/main.cpp` lub `web/server/main.cpp`
- Commitowania artefaktów builda / `compile_commands.json`
- Ciężkich frameworków frontendowych bez wyraźnej potrzeby
- Rozbudowywania CMake bez potrzeby

## Cursor — reguły i hooki

- `.cursor/rules/` — reguły projektu (C++20, struktura, build)
- `.cursor/hooks/` — auto-format `clang-format` po edycji plików C++
- `.cursor/mcp.json` — serwery MCP projektu (obecnie puste; dodaj gdy potrzebne)

## Przydatne linki

- [ModernCppStarter](https://github.com/TheLartians/ModernCppStarter)
- [Modern CMake](https://cliutils.gitlab.io/modern-cmake/README.html)
- [Conan 2 docs](https://docs.conan.io/2/)
- [cpp-httplib](https://github.com/yhirose/cpp-httplib)
