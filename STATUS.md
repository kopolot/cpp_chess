# STATUS — cpp_chess

> Ostatnia aktualizacja: 2026-07-10

## W skrócie

**Silnik szachowy C++20** z konsolowym CLI. Działają dwie implementacje planszy (`Board8x8`, `Board12x12`), pełna logika MVP (ruchy, szach/mat/pat, roszada, en passant, promocja, remisy) oraz testy GTest.

## Etap: MVP CLI — funkcjonalny prototyp

```
[████████████████░░░░] ~80%
```

| Zrobione | Do zrobienia |
|----------|--------------|
| CMake + Conan + GTest + fmt | GUI / Web API |
| `Board8x8` i `Board12x12` (concept `PlayableBoard`) | Notacja SAN / PGN |
| `ChessEngine<BoardType>` — pełna logika gry | Silnik AI |
| CLI: `board`, `move`, `new`, `help`, `exit` | Zegar szachowy |
| Wybór planszy przy starcie (`--board 8x8\|12x12`) | Persystencja partii |
| Szach, mat, pat, roszada, en passant, promocja | |
| Remis: 50 ruchów, 3× powtórzenie | |
| Wyświetlanie unicode + kolory ANSI | |
| ~40 testów GTest (plansza + ruchy) | |

## Ostatni commit

`37dfe46` — *refactor: update project structure and add formatting hooks*

## Uruchomienie

```bash
./bin/conan-install    # pierwszy raz lub po zmianie conanfile.txt
./bin/cmake            # build Debug
./build/cpp_chess_cli                    # domyslnie Board8x8
./build/cpp_chess_cli --board 12x12      # Board12x12 z obramowaniem
./build/cpp_chess_cli --help
cd build && ctest --output-on-failure
```

## Następny krok (rekomendacja)

1. Notacja SAN i eksport/import PGN
2. Zegar (Fischer / blitz) w `GameContext`
3. Prosty silnik minimax jako osobny moduł
