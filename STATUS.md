# STATUS — cpp_chess

> Ostatnia aktualizacja: 2026-07-14

## W skrócie

**Silnik szachowy C++20** z CLI, web GUI (**Vue 3** + nginx), AI oraz **multiplayer online** (kolejka + WebSocket).

## Etap: MVP + web + AI + multiplayer WS

```
[████████████████████] ~93%
```

| Zrobione | Do zrobienia |
|----------|--------------|
| CMake + Conan + GTest + fmt | GUI desktop (Qt/SDL) |
| `Board8x8` i `Board12x12` | Notacja SAN / PGN |
| Pełna logika gry + resign | Zegar szachowy |
| CLI (`--board`, `--ai`, `--difficulty`) | Persystencja / auth |
| Web API + Vue + Docker/nginx | Szybszy AI |
| Online: kolejka, losowy 1v1 | |
| Online: WebSocket `/ws`, disconnect = poddanie | |
| ~45 testów GTest | |

## Uruchomienie

```bash
./bin/docker-up
# http://localhost:8080
```

## API (skrót)

| Metoda | Ścieżka | Opis |
|--------|---------|------|
| GET | `/api/lobby` | kolejka |
| POST | `/api/matchmaking/join` | matchmaking |
| WS | `/ws` | online: `hello`, `join_game`, `move`, `resign` |
| POST | `/api/games` | AI / lokalna |
| POST | `/api/games/:id/move` | tylko AI/local (online → WS) |

Rozłączenie WebSocket w trakcie partii online = **poddanie** (przeciwnik dostaje `opponent_left`).
