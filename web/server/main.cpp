#include <httplib.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <random>
#include <string>
#include <unordered_map>

#include "board/board_8x8.hpp"
#include "chess_engine.hpp"
#include "game/game_api.hpp"

namespace {

constexpr const char* kDefaultHost = "0.0.0.0";
constexpr int kDefaultPort = 8081;

struct GameSession {
  ChessEngine<board::Board8x8> engine;
};

std::mutex g_games_mutex;
std::unordered_map<std::string, GameSession> g_games;

std::string makeGameId() {
  static thread_local std::mt19937 rng{std::random_device{}()};
  static constexpr char kAlphabet[] =
      "0123456789abcdef";
  std::uniform_int_distribution<int> dist(0, 15);

  std::string id;
  id.reserve(16);
  for (int i = 0; i < 16; ++i) {
    id.push_back(kAlphabet[dist(rng)]);
  }
  return id;
}

void setJson(httplib::Response& response, const nlohmann::json& body, int status = 200) {
  response.status = status;
  response.set_header("Content-Type", "application/json; charset=utf-8");
  response.body = body.dump();
}

void setError(httplib::Response& response, int status, const std::string& message) {
  setJson(response, nlohmann::json{{"error", message}}, status);
}

std::optional<std::reference_wrapper<GameSession>> findGame(const std::string& game_id) {
  std::lock_guard lock(g_games_mutex);
  const auto it = g_games.find(game_id);
  if (it == g_games.end()) {
    return std::nullopt;
  }
  return std::ref(it->second);
}

std::string createGame() {
  GameSession session;
  session.engine.startGame();

  const std::string game_id = makeGameId();
  std::lock_guard lock(g_games_mutex);
  g_games.emplace(game_id, std::move(session));
  return game_id;
}

void pruneOldGames() {
  constexpr std::size_t kMaxGames = 256;
  std::lock_guard lock(g_games_mutex);
  while (g_games.size() > kMaxGames) {
    g_games.erase(g_games.begin());
  }
}

int parsePort(int argc, char* argv[]) {
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if ((arg == "--port" || arg == "-p") && i + 1 < argc) {
      return std::stoi(argv[++i]);
    }
  }

  if (const char* env_port = std::getenv("CPP_CHESS_PORT")) {
    return std::stoi(env_port);
  }

  return kDefaultPort;
}

}  // namespace

int main(int argc, char* argv[]) {
  const int port = parsePort(argc, argv);
  httplib::Server server;

  server.set_default_headers({{"Access-Control-Allow-Origin", "*"},
                              {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
                              {"Access-Control-Allow-Headers", "Content-Type"}});

  server.Options(R"(/.*)", [](const httplib::Request&, httplib::Response& response) {
    response.status = 204;
  });

  server.Get("/api/health", [](const httplib::Request&, httplib::Response& response) {
    setJson(response, nlohmann::json{{"ok", true}, {"service", "cpp_chess_web"}});
  });

  server.Post("/api/games", [](const httplib::Request&, httplib::Response& response) {
    pruneOldGames();
    const std::string game_id = createGame();
    const auto session = findGame(game_id);
    if (!session) {
      setError(response, 500, "Nie udalo sie utworzyc gry.");
      return;
    }

    nlohmann::json payload{{"gameId", game_id}};
    payload["state"] = game::api::gameStateJson(session->get().engine);
    setJson(response, payload, 201);
  });

  server.Get(R"(/api/games/([A-Za-z0-9]+))", [](const httplib::Request& request,
                                                 httplib::Response& response) {
    const auto session = findGame(request.matches[1]);
    if (!session) {
      setError(response, 404, "Nie znaleziono gry.");
      return;
    }

    nlohmann::json payload{{"gameId", request.matches[1].str()}};
    payload["state"] = game::api::gameStateJson(session->get().engine);
    setJson(response, payload);
  });

  server.Post(R"(/api/games/([A-Za-z0-9]+)/move)",
              [](const httplib::Request& request, httplib::Response& response) {
                const auto session = findGame(request.matches[1]);
                if (!session) {
                  setError(response, 404, "Nie znaleziono gry.");
                  return;
                }

                nlohmann::json body;
                try {
                  body = nlohmann::json::parse(request.body);
                } catch (const nlohmann::json::exception&) {
                  setError(response, 400, "Niepoprawny JSON.");
                  return;
                }

                const auto move = game::api::parseMoveRequest(body);
                if (!move) {
                  setError(response, 400, "Wymagane pola: from, to.");
                  return;
                }

                auto& engine = session->get().engine;
                if (engine.isGameOver()) {
                  setError(response, 409, "Gra zakonczona.");
                  return;
                }

                if (!engine.tryMove(move->from, move->to, move->promotion)) {
                  if (!move->promotion) {
                    const auto from = game::parseSquare(move->from);
                    const auto to = game::parseSquare(move->to);
                    if (from && to &&
                        engine.needsPromotionForMove(from->first, from->second, to->first,
                                                     to->second)) {
                      setJson(response,
                              nlohmann::json{{"error", "Wymagana promocja."},
                                             {"needsPromotion", true}},
                              400);
                      return;
                    }
                  }

                  setError(response, 400, "Niepoprawny lub niedozwolony ruch.");
                  return;
                }

                nlohmann::json payload{{"gameId", request.matches[1].str()}, {"ok", true}};
                payload["state"] = game::api::gameStateJson(engine);
                setJson(response, payload);
              });

  server.Post(R"(/api/games/([A-Za-z0-9]+)/reset)",
              [](const httplib::Request& request, httplib::Response& response) {
                const auto session = findGame(request.matches[1]);
                if (!session) {
                  setError(response, 404, "Nie znaleziono gry.");
                  return;
                }

                session->get().engine.startGame();
                nlohmann::json payload{{"gameId", request.matches[1].str()}, {"ok", true}};
                payload["state"] = game::api::gameStateJson(session->get().engine);
                setJson(response, payload);
              });

  std::cout << "cpp_chess_web nasluchuje na " << kDefaultHost << ':' << port << '\n';
  if (!server.listen(kDefaultHost, port)) {
    std::cerr << "Nie udalo sie uruchomic serwera HTTP.\n";
    return 1;
  }

  return 0;
}
