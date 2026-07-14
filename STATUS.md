# STATUS — cpp_chess

> Ostatnia aktualizacja: 2026-07-10

## W skrócie

**Silnik szachowy C++20** z konsolowym CLI i prostym **web GUI** (vanilla JS + REST API). Dwie implementacje planszy (`Board8x8`, `Board12x12`), pełna logika MVP oraz konteneryzacja z nginx.

## Etap: MVP + web GUI

```
[██████████████████░░] ~85%
```

| Zrobione | Do zrobienia |
|----------|--------------|
| CMake + Conan + GTest + fmt | GUI desktop (Qt/SDL) |
| `Board8x8` i `Board12x12` | Notacja SAN / PGN |
| Pełna logika gry (szach, roszada, en passant…) | Silnik AI |
| CLI z `--board 8x8\|12x12` | Zegar szachowy |
| HTTP API (`cpp-httplib`) | Persystencja partii (DB) |
| Web GUI (HTML/CSS/JS, bez frameworka) | |
| Docker Compose: nginx + backend | |
| ~40 testów GTest | |

## Uruchomienie

### CLI
```bash
./bin/conan-install
./bin/cmake
./build/cpp_chess_cli
./build/cpp_chess_cli --board 12x12
```

### Web (lokalnie)
```bash
./build/cpp_chess_web -p 8081
# osobno serwuj web/public lub użyj docker-compose
```

### Web (Docker + nginx)
```bash
./bin/docker-up
# http://localhost:8080
```

## API (skrót)

| Metoda | Ścieżka | Opis |
|--------|---------|------|
| GET | `/api/health` | healthcheck |
| POST | `/api/games` | nowa gra → `{ gameId, state }` |
| GET | `/api/games/:id` | stan gry |
| POST | `/api/games/:id/move` | `{ from, to, promotion? }` |
| POST | `/api/games/:id/reset` | reset pozycji |

## Następny krok (rekomendacja)

1. Podpiąć `Board12x12` także w web API (flaga przy tworzeniu gry)
2. Notacja SAN i eksport PGN
3. WebSocket zamiast pollingu (opcjonalnie)
