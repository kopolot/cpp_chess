# STATUS — cpp_chess

> Ostatnia aktualizacja: 2026-07-14

## W skrócie

**Silnik szachowy C++20** z CLI, web GUI (nginx + REST) oraz przeciwnikiem AI na 3 poziomach trudności.

## Etap: MVP + web + AI

```
[███████████████████░] ~90%
```

| Zrobione | Do zrobienia |
|----------|--------------|
| CMake + Conan + GTest + fmt | GUI desktop (Qt/SDL) |
| `Board8x8` i `Board12x12` | Notacja SAN / PGN |
| Pełna logika gry | Zegar szachowy |
| CLI (`--board`, `--ai`, `--difficulty`) | Persystencja partii |
| Web API + vanilla GUI + Docker/nginx | Szybszy / głębszy AI |
| AI: łatwy / średni / trudny (minimax) | |
| ~45 testów GTest | |

## Uruchomienie

### CLI
```bash
./bin/conan-install
./bin/cmake
./build/cpp_chess_cli
./build/cpp_chess_cli --ai --difficulty 2
./build/cpp_chess_cli --ai --difficulty hard --ai-color white
```

### Web (Docker + nginx)
```bash
./bin/docker-up
# http://localhost:8080 — checkbox „Graj przeciw AI” + poziom
```

## API (skrót)

| Metoda | Ścieżka | Opis |
|--------|---------|------|
| GET | `/api/health` | healthcheck |
| POST | `/api/games` | `{ vsAi, difficulty, playerColor? }` |
| GET | `/api/games/:id` | stan + `meta` |
| POST | `/api/games/:id/move` | ruch gracza; odpowiedź może zawierać `aiMove` |
| POST | `/api/games/:id/reset` | reset |

## Następny krok

1. Notacja SAN / PGN  
2. Tablice bitboard / PST dla silniejszego AI  
3. `Board12x12` w web API  
