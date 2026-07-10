# AGENTS.md — cpp_chess

Instrukcje dla agentów AI pracujących w tym repozytorium.

## Czym jest ten projekt

Modułowy **silnik szachowy w C++20** z wymiennym interfejsem użytkownika. Docelowo:

- **Silnik** — logika gry, reprezentacja planszy, ruchy figur, szach/mat/pat
- **Interfejs** — na razie konsola CLI; później GUI lub Web API
- Silnik i UI są **oddzielone** — można podmieniać implementację planszy lub front-end bez ruszania reszty

## Aktualny etap (lipiec 2026)

| Obszar | Status |
|--------|--------|
| Toolchain (CMake, Conan, Doxygen, clang-format, clang-tidy) | Gotowe |
| CLI — REPL (`board`, `move`, `new`, `help`, `exit`) | Gotowe |
| Wybór planszy przy starcie (`--board 8x8\|12x12`) | Gotowe |
| Web API (`cpp-httplib`) + REST JSON | Gotowe |
| Web GUI (vanilla HTML/CSS/JS) | Gotowe |
| Docker Compose (nginx + backend) | Gotowe |
| `ChessEngine<BoardType>` — pełna logika gry | Gotowe |
| `Board8x8` + `Board12x12` (concept `PlayableBoard`) | Gotowe |
| Szach, mat, pat, roszada, en passant, promocja, remisy | Gotowe |
| Testy GTest | ~40 testów (`test_board`, `test_moves`) |

**Następny logiczny krok:** notacja SAN / PGN, zegar, silnik AI.

## Struktura katalogów

```
cpp_chess/
├── cli/main.cpp          # Punkt wejścia CLI (REPL + --board)
├── web/
│   ├── server/main.cpp   # Serwer HTTP REST API
│   └── public/           # Frontend statyczny
├── docker/               # nginx + Dockerfile + compose
├── include/
│   ├── chess_engine.hpp  # Szablon ChessEngine<BoardType>
│   ├── board/            # Board8x8, Board12x12, concept PlayableBoard
│   ├── game/             # Logika gry (ruchy, szach, roszada, …)
│   └── type/chess_piece.hpp
├── src/board/            # Implementacje plansz
├── test/                 # test_board.cpp, test_moves.cpp
├── bin/                  # Skrypty pomocnicze (patrz niżej)
├── conanfile.txt         # fmt, gtest, cpp-httplib, nlohmann_json
├── CMakeLists.txt
├── note.md               # Notatki autora + plany GUI
└── STATUS.md             # Krótkie podsumowanie etapu
```

## Build i testy

Wymagania: **CMake ≥ 3.10**, **Conan 2**, **GCC/Clang z C++20**, **clang-format**, **clang-tidy**.

```bash
# Pełna instalacja zależności + build Release (czyści build/)
./bin/conan-install

# Build Debug (gdy build/ już istnieje)
./bin/cmake

# Build Release
./bin/cmake-release

# Uruchom testy
cd build && ctest --output-on-failure

# Dokumentacja Doxygen
./bin/make-doc   # wynik w html/
```

Binaria po buildzie: `build/cpp_chess_cli`, `build/cpp_chess_web`, `build/tests`.

Web stack: `./bin/docker-up` → nginx `http://localhost:8080`, backend na porcie wewnętrznym `8081`.

**clangd / Go to Definition:** CMake generuje `build/compile_commands.json`. Symlink w korzeniu:
`ln -sf build/compile_commands.json compile_commands.json` — potem przeładuj okno edytora.

## Konwencje kodu

- **Standard:** C++20
- **Formatowanie:** Google style (`.clang-format`)
- **Nagłówki:** `include/`, implementacje plansz w `src/board/`
- **Zależności:** Conan (`fmt` w CLI, `gtest` w testach)
- **Język komentarzy:** polski lub angielski — zachować spójność w obrębie pliku
- **Kolor figur:** `0` = biały, `1` = czarny (`chess_piece.hpp`)
- Nie dodawaj `build/`, `html/`, `latex/` do gita (są w `.gitignore`)

## Decyzje architektoniczne

Zaimplementowane reprezentacje planszy (oba spełniają `PlayableBoard`):

1. **`Board8x8`** — granice przez sprawdzanie indeksów 0..7
2. **`Board12x12`** — obramowanie wykrywane kolizją, offset 2

CLI wybiera implementację flagą `--board 8x8` (domyślnie) lub `--board 12x12`. `ChessEngine` jest szablonem na `BoardType` — logika gry jest wspólna.

## Co robić / czego unikać

**Rób:**
- Małe, skupione zmiany zgodne z modularnością silnik ↔ UI
- Logikę gry w `include/game/` (szablony na `BoardType`) lub `.cpp` w `src/` gdy potrzeba
- Prawdziwe testy GTest dla ruchów, szachu, roszady itd.
- Uruchamiaj `./bin/cmake` po większych zmianach

**Unikaj:**
- Mieszania logiki silnika z `cli/main.cpp` (CLI tylko parsuje i wywołuje API silnika)
- Commitowania artefaktów builda
- Rozbudowywania CMake bez potrzeby

## Cursor — reguły i hooki

- `.cursor/rules/` — reguły projektu (C++20, struktura, build)
- `.cursor/hooks/` — auto-format `clang-format` po edycji plików C++
- `.cursor/mcp.json` — serwery MCP projektu (obecnie puste; dodaj gdy potrzebne)

## Przydatne linki (z note.md)

- [ModernCppStarter](https://github.com/TheLartians/ModernCppStarter)
- [Modern CMake](https://cliutils.gitlab.io/modern-cmake/README.html)
- [Conan 2 docs](https://docs.conan.io/2/)
