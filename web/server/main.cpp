#include <httplib.h>

#include <cstdlib>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>

#include "ai/search.hpp"
#include "board/board_8x8.hpp"
#include "chess_engine.hpp"
#include "game/game_api.hpp"
#include "game/square.hpp"

namespace {

constexpr const char* kDefaultHost = "0.0.0.0";
constexpr int kDefaultPort = 8081;

struct GameSession {
  ChessEngine<board::Board8x8> engine;
  bool vs_ai = false;
  ai::Difficulty difficulty = ai::Difficulty::Medium;
  int player_color = 0;
};

std::mutex g_games_mutex;
std::unordered_map<std::string, GameSession> g_games;

std::string makeGameId() {
  static thread_local std::mt19937 rng{std::random_device{}()};
  static constexpr char kAlphabet[] = "0123456789abcdef";
  std::uniform_int_distribution<int> dist(0, 15);

  std::string id;
  id.reserve(16);
  for (int i = 0; i < 16; ++i) {
    id.push_back(kAlphabet[dist(rng)]);
  }
  return id;
}

void setJson(httplib::Response& response, const nlohmann::json& body,
             int status = 200) {
  response.status = status;
  response.set_header("Content-Type", "application/json; charset=utf-8");
  response.body = body.dump();
}

void setError(httplib::Response& response, int status,
              const std::string& message) {
  setJson(response, nlohmann::json{{"error", message}}, status);
}

nlohmann::json sessionMetaJson(const GameSession& session) {
  return nlohmann::json{
      {"vsAi", session.vs_ai},
      {"difficulty", static_cast<int>(session.difficulty)},
      {"difficultyName", ai::difficultyName(session.difficulty)},
      {"playerColor", session.player_color == 0 ? "white" : "black"}};
}

nlohmann::json gamePayload(const std::string& game_id,
                           const GameSession& session,
                           std::optional<std::string> ai_move = std::nullopt) {
  nlohmann::json payload{{"gameId", game_id}};
  payload["meta"] = sessionMetaJson(session);
  payload["state"] = game::api::gameStateJson(session.engine);
  if (ai_move) {
    payload["aiMove"] = *ai_move;
  } else {
    payload["aiMove"] = nullptr;
  }
  return payload;
}

std::optional<std::reference_wrapper<GameSession>> findGame(
    const std::string& game_id) {
  std::lock_guard lock(g_games_mutex);
  const auto it = g_games.find(game_id);
  if (it == g_games.end()) {
    return std::nullopt;
  }
  return std::ref(it->second);
}

std::optional<std::string> maybePlayAi(GameSession& session) {
  if (!session.vs_ai || session.engine.isGameOver() ||
      session.engine.currentTurn() == session.player_color) {
    return std::nullopt;
  }

  const auto move =
      ai::chooseMove(session.engine.getBoard(), session.engine.getContext(),
                     session.engine.currentTurn(), session.difficulty);
  if (!move) {
    return std::nullopt;
  }

  if (!session.engine.tryMove(move->from_file, move->from_rank, move->to_file,
                              move->to_rank, move->promotion)) {
    return std::nullopt;
  }

  return move->toNotation();
}

struct CreateOptions {
  bool vs_ai = false;
  ai::Difficulty difficulty = ai::Difficulty::Medium;
  int player_color = 0;
};

std::optional<CreateOptions> parseCreateOptions(const nlohmann::json& body) {
  CreateOptions options;
  if (body.contains("vsAi")) {
    if (!body["vsAi"].is_boolean()) {
      return std::nullopt;
    }
    options.vs_ai = body["vsAi"].get<bool>();
  }

  if (body.contains("difficulty")) {
    if (body["difficulty"].is_number_integer()) {
      const auto parsed = ai::parseDifficulty(body["difficulty"].get<int>());
      if (!parsed) {
        return std::nullopt;
      }
      options.difficulty = *parsed;
      options.vs_ai = true;
    } else if (body["difficulty"].is_string()) {
      const auto parsed =
          ai::parseDifficulty(body["difficulty"].get<std::string>());
      if (!parsed) {
        return std::nullopt;
      }
      options.difficulty = *parsed;
      options.vs_ai = true;
    } else {
      return std::nullopt;
    }
  }

  if (body.contains("playerColor")) {
    if (!body["playerColor"].is_string()) {
      return std::nullopt;
    }
    const auto color = body["playerColor"].get<std::string>();
    if (color == "white") {
      options.player_color = 0;
    } else if (color == "black") {
      options.player_color = 1;
    } else {
      return std::nullopt;
    }
    options.vs_ai = true;
  }

  return options;
}

std::string createGame(const CreateOptions& options) {
  GameSession session;
  session.engine.startGame();
  session.vs_ai = options.vs_ai;
  session.difficulty = options.difficulty;
  session.player_color = options.player_color;

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

  server.set_default_headers(
      {{"Access-Control-Allow-Origin", "*"},
       {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
       {"Access-Control-Allow-Headers", "Content-Type"}});

  server.Options(R"(/.*)",
                 [](const httplib::Request&, httplib::Response& response) {
                   response.status = 204;
                 });

  server.Get(
      "/api/health", [](const httplib::Request&, httplib::Response& response) {
        setJson(response,
                nlohmann::json{{"ok", true}, {"service", "cpp_chess_web"}});
      });

  server.Post("/api/games", [](const httplib::Request& request,
                               httplib::Response& response) {
    pruneOldGames();

    nlohmann::json body = nlohmann::json::object();
    if (!request.body.empty()) {
      try {
        body = nlohmann::json::parse(request.body);
      } catch (const nlohmann::json::exception&) {
        setError(response, 400, "Niepoprawny JSON.");
        return;
      }
    }

    const auto options = parseCreateOptions(body);
    if (!options) {
      setError(response, 400,
               "Niepoprawne opcje gry (vsAi/difficulty/playerColor).");
      return;
    }

    const std::string game_id = createGame(*options);
    const auto session = findGame(game_id);
    if (!session) {
      setError(response, 500, "Nie udalo sie utworzyc gry.");
      return;
    }

    const auto ai_move = maybePlayAi(session->get());
    setJson(response, gamePayload(game_id, session->get(), ai_move), 201);
  });

  server.Get(R"(/api/games/([A-Za-z0-9]+))", [](const httplib::Request& request,
                                                httplib::Response& response) {
    const auto session = findGame(request.matches[1]);
    if (!session) {
      setError(response, 404, "Nie znaleziono gry.");
      return;
    }
    setJson(response, gamePayload(request.matches[1].str(), session->get()));
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

                auto& game = session->get();
                auto& engine = game.engine;
                if (engine.isGameOver()) {
                  setError(response, 409, "Gra zakonczona.");
                  return;
                }
                if (game.vs_ai && engine.currentTurn() != game.player_color) {
                  setError(response, 409, "Teraz kolej AI.");
                  return;
                }

                if (!engine.tryMove(move->from, move->to, move->promotion)) {
                  if (!move->promotion) {
                    const auto from = game::parseSquare(move->from);
                    const auto to = game::parseSquare(move->to);
                    if (from && to &&
                        engine.needsPromotionForMove(from->first, from->second,
                                                     to->first, to->second)) {
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

                const auto ai_move = maybePlayAi(game);
                nlohmann::json payload =
                    gamePayload(request.matches[1].str(), game, ai_move);
                payload["ok"] = true;
                setJson(response, payload);
              });

  server.Post(R"(/api/games/([A-Za-z0-9]+)/reset)",
              [](const httplib::Request& request, httplib::Response& response) {
                const auto session = findGame(request.matches[1]);
                if (!session) {
                  setError(response, 404, "Nie znaleziono gry.");
                  return;
                }

                auto& game = session->get();
                game.engine.startGame();
                const auto ai_move = maybePlayAi(game);
                nlohmann::json payload =
                    gamePayload(request.matches[1].str(), game, ai_move);
                payload["ok"] = true;
                setJson(response, payload);
              });

  std::cout << "cpp_chess_web nasluchuje na " << kDefaultHost << ':' << port
            << '\n';
  if (!server.listen(kDefaultHost, port)) {
    std::cerr << "Nie udalo sie uruchomic serwera HTTP.\n";
    return 1;
  }

  return 0;
}
