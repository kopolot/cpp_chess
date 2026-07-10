# AGENTS.md — cpp_chess

Instrukcje dla agentów AI pracujących w tym repozytorium.

## Czym jest ten projekt

Modułowy **silnik szachowy w C++20** z wymiennym interfejsem użytkownika. Docelowo:

- **Silnik** — logika gry, reprezentacja planszy, ruchy figur, szach/mat/pat
- **Interfejs** — na razie konsola CLI; później GUI lub Web API
- Silnik i UI są **oddzielone** — można podmieniać implementację planszy lub front-end bez ruszania reszty

## Aktualny etap (lipiec 2025)

| Obszar | Status |
|--------|--------|
| Toolchain (CMake, Conan, Doxygen, clang-format, clang-tidy) | Gotowe |
| CLI — pętla REPL (`exit`, echo komend) | Szkielet |
| `ChessEngine<BoardType>` — szablon silnika | Nagłówek, bez implementacji |
| `PieceType` + `board::Occupant` | Gotowe; logika w `include/game/` |
| Reprezentacja planszy (8×8, 64-lista, 12×12…) | **Nie wybrana** — patrz `note.md` |
| `src/`, `engine/` | Puste katalogi |
| Testy GTest | Placeholder (`dodaj(2,3)==5`), brak testów szachowych |

**Następny logiczny krok:** wybrać reprezentację planszy, zaimplementować `BoardType`, podłączyć silnik do CLI.

## Struktura katalogów

```
cpp_chess/
├── cli/main.cpp          # Punkt wejścia CLI (REPL)
├── include/
│   ├── chess_engine.hpp  # Szablon ChessEngine<BoardType>
│   └── type/chess_piece.hpp
├── src/                  # Implementacje (puste — tu trafi logika)
├── engine/               # Rezerwa na moduł silnika (puste)
├── test/test_main.cpp    # Testy GTest
├── bin/                  # Skrypty pomocnicze (patrz niżej)
├── conanfile.txt         # fmt, gtest
├── CMakeLists.txt
├── note.md               # Notatki autora + założenia projektu
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

Binaria po buildzie: `build/cpp_chess_cli`, `build/tests`.

**clangd / Go to Definition:** CMake generuje `build/compile_commands.json`. Symlink w korzeniu:
`ln -sf build/compile_commands.json compile_commands.json` — potem przeładuj okno edytora.

## Konwencje kodu

- **Standard:** C++20
- **Formatowanie:** Google style (`.clang-format`)
- **Nagłówki:** `include/`, implementacje w `src/` lub `engine/`
- **Zależności:** Conan (`fmt` w CLI, `gtest` w testach)
- **Język komentarzy:** polski lub angielski — zachować spójność w obrębie pliku
- **Kolor figur:** `0` = biały, `1` = czarny (`chess_piece.hpp`)
- Nie dodawaj `build/`, `html/`, `latex/` do gita (są w `.gitignore`)

## Decyzje architektoniczne (otwarte)

Z `note.md` — wybór reprezentacji planszy:

1. Tablica 8×8
2. Lista 64 pól
3. Tablica 12×12 (z obramowaniem — patrz `no_borders?`)
4. Lista ~140 elementów

`ChessEngine` jest szablonem na `BoardType` — wybór struktury planszy wpływa na cały silnik.

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
