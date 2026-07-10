# STATUS — cpp_chess

> Ostatnia aktualizacja: 2025-07-09

## W skrócie

**Silnik szachowy C++20** z konsolowym CLI. Projekt ma działający toolchain, ale **logika gry jeszcze nie istnieje** — są tylko szkielety nagłówków i pętla REPL.

## Etap: wczesny prototyp / szkielet

```
[████████░░░░░░░░░░░░] ~35%
```

| Zrobione | Do zrobienia |
|----------|--------------|
| CMake + Conan + GTest + fmt | Wybór struktury planszy |
| Doxygen, clang-format, VS Code tasks | Implementacja `BoardType` |
| Szablon `ChessEngine<BoardType>` | Logika ruchów figur |
| Klasy figur (nagłówki) | Szach, mat, pat, roszada, en passant |
| CLI REPL (exit + echo) | Podłączenie silnika do CLI |
| Placeholder testów | Testy szachowe |

## Ostatni commit

`d54afe2` — *cli entrypoint and chess pieces* (nagłówki figur + szkielet CLI)

## Następny krok (rekomendacja)

1. Zdecydować: tablica 8×8 vs 12×12 z borderami (`no_borders?` to szkic 12×12)
2. Dodać `include/board/...` + implementację w `src/`
3. Zaimplementować `ChessEngine::startGame()` i podstawowy stan gry
4. Podłączyć CLI: komendy `board`, `move e2e4`, `exit`

## Szybki start

```bash
./bin/conan-install    # pierwszy raz lub po zmianie conanfile.txt
./build/cpp_chess_cli  # uruchom CLI
```
