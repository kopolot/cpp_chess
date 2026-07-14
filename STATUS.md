# STATUS — cpp_chess

> Ostatnia aktualizacja: 2026-07-14

## W skrócie

**Silnik szachowy C++20** z CLI, web GUI (**Vue 3** + nginx + REST), AI oraz **multiplayer online** (kolejka + losowy przeciwnik).

## Etap: MVP + web + AI + multiplayer

```
[████████████████████] ~92%
```

| Zrobione | Do zrobienia |
|----------|--------------|
| CMake + Conan + GTest + fmt | GUI desktop (Qt/SDL) |
| `Board8x8` i `Board12x12` | Notacja SAN / PGN |
| Pełna logika gry | Zegar szachowy |
| CLI (`--board`, `--ai`, `--difficulty`) | Persystencja / auth |
| Web API + Vue GUI + Docker/nginx | WebSocket zamiast pollingu |
| AI: łatwy / średni / trudny | Szybszy AI |
| Online: kolejka, losowy 1v1, wiele sesji | |
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
| GET | `/api/lobby` | kolejka + liczba partii |
| POST | `/api/matchmaking/join` | wejdź do kolejki / sparuj (`name`, `X-Player-Id`) |
| GET | `/api/matchmaking/status` | `waiting` / `matched` |
| POST | `/api/matchmaking/leave` | opuść kolejkę |
| POST | `/api/games` | `{ vsAi, difficulty, playerColor? }` |
| GET | `/api/games/:id` | stan + `meta` (wymaga `X-Player-Id` w online) |
| POST | `/api/games/:id/move` | ruch gracza; online: tylko Twoja tura |
| POST | `/api/games/:id/reset` | reset (tylko local/AI) |

## Następny krok

1. Notacja SAN / PGN  
2. Tablice bitboard / PST dla silniejszego AI  
3. `Board12x12` w web API  
