# cpp_chess

Modułowy silnik szachowy w **C++20** z konsolowym interfejsem.

## Status

Wczesny szkielet — toolchain działa, logika gry w trakcie planowania. Szczegóły: [STATUS.md](STATUS.md).

## Wymagania

- CMake ≥ 3.10
- Conan 2
- Kompilator z obsługą C++20 (GCC/Clang)
- clang-format, clang-tidy (opcjonalnie)

## Szybki start

```bash
./bin/conan-install
./build/cpp_chess_cli
```

## Struktura

| Katalog | Opis |
|---------|------|
| `cli/` | Punkt wejścia — pętla komend konsoli |
| `include/` | Nagłówki silnika i figur |
| `src/`, `engine/` | Implementacje (w przygotowaniu) |
| `test/` | Testy GTest |
| `bin/` | Skrypty build/test/doc |

## Dla agentów AI

Zobacz [AGENTS.md](AGENTS.md) — pełny kontekst projektu, konwencje i następne kroki.

## Założenia

- Silnik i UI są wymienne (modułowa architektura)
- Reprezentacja planszy jako parametr szablonu `ChessEngine<BoardType>`
- Docelowo: konsola → GUI / Web API
