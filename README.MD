# cpp_chess

Modułowy silnik szachowy w **C++20** z CLI i prostym **web GUI**.

## Status

MVP: pełna logika gry, dwie implementacje planszy, konsola oraz REST API + vanilla frontend za nginxiem. Szczegóły: [STATUS.md](STATUS.md).

## Wymagania

- CMake ≥ 3.10
- Conan 2
- Kompilator z C++20 (GCC/Clang)
- clang-format, clang-tidy (opcjonalnie)
- Docker + Compose (opcjonalnie, do web stacku)

## Szybki start — CLI

```bash
./bin/conan-install
./bin/cmake
./build/cpp_chess_cli
./build/cpp_chess_cli --board 12x12
./build/cpp_chess_cli --ai --difficulty medium
./build/cpp_chess_cli --help
```

Komendy w REPL: `board`, `move e2e4`, `move e2 e4`, `new`, `help`, `exit`.

## Szybki start — Web GUI

```bash
./bin/docker-up
# → http://localhost:8080  (nginx → static + /api → backend:8081)
```

Lokalnie bez Dockera:

```bash
./build/cpp_chess_web -p 8081
# frontend z web/public/ serwuj osobno albo przez compose
```

### REST API

| Metoda | Ścieżka | Opis |
|--------|---------|------|
| `GET` | `/api/health` | healthcheck |
| `POST` | `/api/games` | nowa gra (`vsAi`, `difficulty`, `playerColor`) |
| `GET` | `/api/games/:id` | stan gry |
| `POST` | `/api/games/:id/move` | `{ "from": "e2", "to": "e4", "promotion": "q" }` |
| `POST` | `/api/games/:id/reset` | reset pozycji |

Poziomy AI: `1` łatwy, `2` średni, `3` trudny (głębokość minimax 1/2/3).

## Struktura

| Katalog | Opis |
|---------|------|
| `cli/` | Konsolowy REPL |
| `web/server/` | Serwer HTTP (REST) |
| `web/public/` | Frontend (HTML/CSS/JS, bez frameworka) |
| `docker/` | Dockerfile backendu, nginx, compose |
| `include/` | Silnik, `game/`, `board/` |
| `src/board/` | `Board8x8`, `Board12x12` |
| `test/` | GTest |
| `bin/` | Skrypty build / test / docker |

## Testy

```bash
cd build && ctest --output-on-failure
```

## Dla agentów AI

Zobacz [AGENTS.md](AGENTS.md) — kontekst, konwencje, API i następne kroki.

## Założenia

- Silnik i UI są wymienne (`ChessEngine<BoardType>`)
- CLI wybiera planszę (`8x8` / `12x12`); web API na razie używa `Board8x8`
- Frontend web celowo bez Vue/React — mały, statyczny UI
