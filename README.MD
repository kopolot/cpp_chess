# cpp_chess

Modułowy silnik szachowy w **C++20** z konsolowym interfejsem.

## Status

MVP CLI — ruchy figur, szach/mat/pat, roszada, en passant, promocja, remisy. Dwie implementacje planszy do wyboru przy starcie. Szczegóły: [STATUS.md](STATUS.md).

## Wymagania

- CMake ≥ 3.10
- Conan 2
- Kompilator z obsługą C++20 (GCC/Clang)
- clang-format, clang-tidy (opcjonalnie)

## Szybki start

```bash
./bin/conan-install
./bin/cmake
./build/cpp_chess_cli
./build/cpp_chess_cli --board 12x12
```

## Struktura

| Katalog | Opis |
|---------|------|
| `cli/` | Punkt wejścia — pętla komend konsoli |
| `include/` | Silnik (`chess_engine.hpp`), logika gry (`game/`), plansze (`board/`) |
| `src/board/` | Implementacje `Board8x8` i `Board12x12` |
| `test/` | Testy GTest |
| `bin/` | Skrypty build/test/doc |

## Dla agentów AI

Zobacz [AGENTS.md](AGENTS.md) — pełny kontekst projektu, konwencje i następne kroki.

## Założenia

- Silnik i UI są wymienne (modułowa architektura)
- Reprezentacja planszy jako parametr szablonu `ChessEngine<BoardType>`
- Docelowo: konsola → GUI / Web API
