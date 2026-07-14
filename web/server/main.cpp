#include <httplib.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ai/search.hpp"
#include "board/board_8x8.hpp"
#include "chess_engine.hpp"
#include "game/game_api.hpp"
#include "game/square.hpp"

namespace {

constexpr const char* kDefaultHost = "0.0.0.0";
constexpr int kDefaultPort = 8081;

enum class GameMode { Local, VsAi, Online };

struct GameSession {
  ChessEngine<board::Board8x8> engine;
  GameMode mode = GameMode::Local;
  bool vs_ai = false;
  ai::Difficulty difficulty = ai::Difficulty::Medium;
  int player_color = 0;  // kolor lokalnego/AI-opponent human

  std::string white_player_id;
  std::string black_player_id;
  std::string white_name;
  std::string black_name;
  std::string last_move;
  int version = 0;
  std::chrono::steady_clock::time_point updated_at =
      std::chrono::steady_clock::now();
};

struct WaitingPlayer {
  std::string player_id;
  std::string name;
  std::chrono::steady_clock::time_point joined_at;
};

struct MatchTicket {
  std::string game_id;
  int color = 0;
  std::string opponent_name;
};

std::mutex g_mutex;
std::unordered_map<std::string, GameSession> g_games;
std::deque<WaitingPlayer> g_queue;
std::unordered_map<std::string, MatchTicket> g_tickets;  // player_id -> ticket

std::mt19937& rng() {
  static thread_local std::mt19937 engine{std::random_device{}()};
  return engine;
}

std::string makeId(int length = 16) {
  static constexpr char kAlphabet[] = "0123456789abcdef";
  std::uniform_int_distribution<int> dist(0, 15);
  std::string id;
  id.reserve(static_cast<std::size_t>(length));
  for (int i = 0; i < length; ++i) {
    id.push_back(kAlphabet[dist(rng())]);
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

std::string playerIdFrom(const httplib::Request& request,
                         const nlohmann::json& body = {}) {
  if (request.has_header("X-Player-Id")) {
    return request.get_header_value("X-Player-Id");
  }
  if (body.contains("playerId") && body["playerId"].is_string()) {
    return body["playerId"].get<std::string>();
  }
  if (request.has_param("playerId")) {
    return request.get_param_value("playerId");
  }
  return {};
}

const char* modeName(GameMode mode) {
  switch (mode) {
    case GameMode::Local:
      return "local";
    case GameMode::VsAi:
      return "ai";
    case GameMode::Online:
      return "online";
  }
  return "local";
}

nlohmann::json sessionMetaJson(const GameSession& session,
                               std::optional<int> viewer_color = std::nullopt) {
  nlohmann::json meta{
      {"mode", modeName(session.mode)},
      {"vsAi", session.vs_ai},
      {"difficulty", static_cast<int>(session.difficulty)},
      {"difficultyName", ai::difficultyName(session.difficulty)},
      {"version", session.version},
      {"lastMove", session.last_move.empty()
                       ? nullptr
                       : nlohmann::json(session.last_move)}};

  if (session.mode == GameMode::Online) {
    meta["whiteName"] = session.white_name;
    meta["blackName"] = session.black_name;
    if (viewer_color) {
      meta["playerColor"] = *viewer_color == 0 ? "white" : "black";
      meta["opponentName"] =
          *viewer_color == 0 ? session.black_name : session.white_name;
    }
  } else if (session.vs_ai) {
    meta["playerColor"] = session.player_color == 0 ? "white" : "black";
  } else if (viewer_color) {
    meta["playerColor"] = *viewer_color == 0 ? "white" : "black";
  }

  return meta;
}

nlohmann::json gamePayload(const std::string& game_id,
                           const GameSession& session,
                           std::optional<int> viewer_color = std::nullopt,
                           std::optional<std::string> ai_move = std::nullopt) {
  nlohmann::json payload{{"gameId", game_id}};
  payload["meta"] = sessionMetaJson(session, viewer_color);
  payload["state"] = game::api::gameStateJson(session.engine);
  payload["aiMove"] = ai_move ? nlohmann::json(*ai_move) : nullptr;
  return payload;
}

std::optional<int> colorForPlayer(const GameSession& session,
                                  const std::string& player_id) {
  if (session.mode != GameMode::Online || player_id.empty()) {
    return std::nullopt;
  }
  if (session.white_player_id == player_id) {
    return 0;
  }
  if (session.black_player_id == player_id) {
    return 1;
  }
  return std::nullopt;
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

  session.last_move = move->toNotation();
  ++session.version;
  session.updated_at = std::chrono::steady_clock::now();
  return session.last_move;
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

void pruneQueueLocked(std::chrono::steady_clock::time_point now) {
  constexpr auto kMaxWait = std::chrono::minutes(5);
  while (!g_queue.empty() && now - g_queue.front().joined_at > kMaxWait) {
    g_queue.pop_front();
  }
}

void pruneGamesLocked(std::chrono::steady_clock::time_point now) {
  constexpr auto kMaxAge = std::chrono::hours(2);
  constexpr std::size_t kMaxGames = 512;
  for (auto it = g_games.begin(); it != g_games.end();) {
    if (now - it->second.updated_at > kMaxAge) {
      it = g_games.erase(it);
    } else {
      ++it;
    }
  }
  while (g_games.size() > kMaxGames) {
    g_games.erase(g_games.begin());
  }
}

std::string createLocalOrAiGame(const CreateOptions& options) {
  GameSession session;
  session.engine.startGame();
  session.vs_ai = options.vs_ai;
  session.difficulty = options.difficulty;
  session.player_color = options.player_color;
  session.mode = options.vs_ai ? GameMode::VsAi : GameMode::Local;
  session.updated_at = std::chrono::steady_clock::now();

  const std::string game_id = makeId();
  std::lock_guard lock(g_mutex);
  pruneGamesLocked(session.updated_at);
  g_games.emplace(game_id, std::move(session));
  return game_id;
}

std::pair<std::string, MatchTicket> createOnlineMatch(const WaitingPlayer& a,
                                                      const WaitingPlayer& b) {
  GameSession session;
  session.engine.startGame();
  session.mode = GameMode::Online;
  session.vs_ai = false;
  session.updated_at = std::chrono::steady_clock::now();

  std::uniform_int_distribution<int> coin(0, 1);
  const bool a_is_white = coin(rng()) == 0;
  if (a_is_white) {
    session.white_player_id = a.player_id;
    session.black_player_id = b.player_id;
    session.white_name = a.name;
    session.black_name = b.name;
  } else {
    session.white_player_id = b.player_id;
    session.black_player_id = a.player_id;
    session.white_name = b.name;
    session.black_name = a.name;
  }

  const std::string game_id = makeId();
  g_games.emplace(game_id, std::move(session));

  MatchTicket ticket_a{game_id, a_is_white ? 0 : 1, b.name};
  MatchTicket ticket_b{game_id, a_is_white ? 1 : 0, a.name};
  g_tickets[a.player_id] = ticket_a;
  g_tickets[b.player_id] = ticket_b;
  return {game_id, ticket_a};
}

nlohmann::json ticketJson(const MatchTicket& ticket) {
  return nlohmann::json{{"status", "matched"},
                        {"gameId", ticket.game_id},
                        {"playerColor", ticket.color == 0 ? "white" : "black"},
                        {"opponentName", ticket.opponent_name}};
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
       {"Access-Control-Allow-Headers", "Content-Type, X-Player-Id"}});

  server.Options(R"(/.*)",
                 [](const httplib::Request&, httplib::Response& response) {
                   response.status = 204;
                 });

  server.Get("/api/health",
             [](const httplib::Request&, httplib::Response& response) {
               std::lock_guard lock(g_mutex);
               setJson(response, nlohmann::json{{"ok", true},
                                                {"service", "cpp_chess_web"},
                                                {"games", g_games.size()},
                                                {"waiting", g_queue.size()}});
             });

  server.Get("/api/lobby",
             [](const httplib::Request&, httplib::Response& response) {
               std::lock_guard lock(g_mutex);
               const auto now = std::chrono::steady_clock::now();
               pruneQueueLocked(now);
               pruneGamesLocked(now);

               int online_games = 0;
               int ai_games = 0;
               for (const auto& [_, game] : g_games) {
                 if (game.mode == GameMode::Online) {
                   ++online_games;
                 } else if (game.mode == GameMode::VsAi) {
                   ++ai_games;
                 }
               }

               setJson(response, nlohmann::json{{"waiting", g_queue.size()},
                                                {"games", g_games.size()},
                                                {"onlineGames", online_games},
                                                {"aiGames", ai_games}});
             });

  server.Post("/api/matchmaking/join", [](const httplib::Request& request,
                                          httplib::Response& response) {
    nlohmann::json body = nlohmann::json::object();
    if (!request.body.empty()) {
      try {
        body = nlohmann::json::parse(request.body);
      } catch (const nlohmann::json::exception&) {
        setError(response, 400, "Niepoprawny JSON.");
        return;
      }
    }

    std::string player_id = playerIdFrom(request, body);
    if (player_id.empty()) {
      player_id = makeId(12);
    }

    std::string name = "Gracz";
    if (body.contains("name") && body["name"].is_string()) {
      name = body["name"].get<std::string>();
      if (name.empty() || name.size() > 32) {
        setError(response, 400, "Niepoprawna nazwa (1-32 znaki).");
        return;
      }
    } else {
      name = "Gracz-" + player_id.substr(0, 4);
    }

    std::lock_guard lock(g_mutex);
    const auto now = std::chrono::steady_clock::now();
    pruneQueueLocked(now);
    pruneGamesLocked(now);

    if (const auto it = g_tickets.find(player_id); it != g_tickets.end()) {
      auto payload = ticketJson(it->second);
      payload["playerId"] = player_id;
      setJson(response, payload);
      return;
    }

    // już w kolejce?
    for (const auto& waiting : g_queue) {
      if (waiting.player_id == player_id) {
        setJson(response, nlohmann::json{{"status", "waiting"},
                                         {"playerId", player_id},
                                         {"waiting", g_queue.size()},
                                         {"name", waiting.name}});
        return;
      }
    }

    // znajdź przeciwnika (nie siebie)
    std::optional<WaitingPlayer> opponent;
    for (auto it = g_queue.begin(); it != g_queue.end(); ++it) {
      if (it->player_id != player_id) {
        opponent = *it;
        g_queue.erase(it);
        break;
      }
    }

    if (!opponent) {
      g_queue.push_back(WaitingPlayer{player_id, name, now});
      setJson(response,
              nlohmann::json{{"status", "waiting"},
                             {"playerId", player_id},
                             {"waiting", g_queue.size()},
                             {"name", name}},
              202);
      return;
    }

    const WaitingPlayer self{player_id, name, now};
    const auto [game_id, ticket] = createOnlineMatch(self, *opponent);
    auto payload = ticketJson(ticket);
    payload["playerId"] = player_id;
    setJson(response, payload, 201);
  });

  server.Get("/api/matchmaking/status", [](const httplib::Request& request,
                                           httplib::Response& response) {
    const std::string player_id = playerIdFrom(request);
    if (player_id.empty()) {
      setError(response, 400, "Wymagane playerId.");
      return;
    }

    std::lock_guard lock(g_mutex);
    pruneQueueLocked(std::chrono::steady_clock::now());

    if (const auto it = g_tickets.find(player_id); it != g_tickets.end()) {
      auto payload = ticketJson(it->second);
      payload["playerId"] = player_id;
      setJson(response, payload);
      return;
    }

    bool waiting = false;
    for (const auto& entry : g_queue) {
      if (entry.player_id == player_id) {
        waiting = true;
        break;
      }
    }

    setJson(response, nlohmann::json{{"status", waiting ? "waiting" : "idle"},
                                     {"playerId", player_id},
                                     {"waiting", g_queue.size()}});
  });

  server.Post("/api/matchmaking/leave", [](const httplib::Request& request,
                                           httplib::Response& response) {
    nlohmann::json body = nlohmann::json::object();
    if (!request.body.empty()) {
      try {
        body = nlohmann::json::parse(request.body);
      } catch (const nlohmann::json::exception&) {
        setError(response, 400, "Niepoprawny JSON.");
        return;
      }
    }

    const std::string player_id = playerIdFrom(request, body);
    if (player_id.empty()) {
      setError(response, 400, "Wymagane playerId.");
      return;
    }

    std::lock_guard lock(g_mutex);
    g_queue.erase(std::remove_if(g_queue.begin(), g_queue.end(),
                                 [&](const WaitingPlayer& p) {
                                   return p.player_id == player_id;
                                 }),
                  g_queue.end());
    g_tickets.erase(player_id);
    setJson(response, nlohmann::json{{"ok", true}, {"status", "idle"}});
  });

  server.Post("/api/games", [](const httplib::Request& request,
                               httplib::Response& response) {
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

    const std::string game_id = createLocalOrAiGame(*options);

    std::lock_guard lock(g_mutex);
    auto& session = g_games.at(game_id);
    const auto ai_move = maybePlayAi(session);
    setJson(response,
            gamePayload(game_id, session,
                        session.vs_ai ? std::optional(session.player_color)
                                      : std::nullopt,
                        ai_move),
            201);
  });

  server.Get(R"(/api/games/([A-Za-z0-9]+))",
             [](const httplib::Request& request, httplib::Response& response) {
               const std::string game_id = request.matches[1];
               const std::string player_id = playerIdFrom(request);

               std::lock_guard lock(g_mutex);
               const auto it = g_games.find(game_id);
               if (it == g_games.end()) {
                 setError(response, 404, "Nie znaleziono gry.");
                 return;
               }

               auto& session = it->second;
               const auto color = colorForPlayer(session, player_id);
               if (session.mode == GameMode::Online && !color) {
                 setError(response, 403, "Nie jestes graczem tej partii.");
                 return;
               }

               setJson(response, gamePayload(game_id, session, color));
             });

  server.Post(R"(/api/games/([A-Za-z0-9]+)/move)",
              [](const httplib::Request& request, httplib::Response& response) {
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

                const std::string game_id = request.matches[1];
                const std::string player_id = playerIdFrom(request, body);

                std::lock_guard lock(g_mutex);
                const auto it = g_games.find(game_id);
                if (it == g_games.end()) {
                  setError(response, 404, "Nie znaleziono gry.");
                  return;
                }

                auto& game = it->second;
                auto& engine = game.engine;
                if (engine.isGameOver()) {
                  setError(response, 409, "Gra zakonczona.");
                  return;
                }

                std::optional<int> viewer_color;
                if (game.mode == GameMode::Online) {
                  viewer_color = colorForPlayer(game, player_id);
                  if (!viewer_color) {
                    setError(response, 403, "Nie jestes graczem tej partii.");
                    return;
                  }
                  if (engine.currentTurn() != *viewer_color) {
                    setError(response, 409, "Teraz kolej przeciwnika.");
                    return;
                  }
                } else if (game.vs_ai) {
                  viewer_color = game.player_color;
                  if (engine.currentTurn() != game.player_color) {
                    setError(response, 409, "Teraz kolej AI.");
                    return;
                  }
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

                game.last_move = move->from + move->to;
                if (move->promotion) {
                  switch (*move->promotion) {
                    case PieceType::Queen:
                      game.last_move.push_back('q');
                      break;
                    case PieceType::Rook:
                      game.last_move.push_back('r');
                      break;
                    case PieceType::Bishop:
                      game.last_move.push_back('b');
                      break;
                    case PieceType::Knight:
                      game.last_move.push_back('n');
                      break;
                    default:
                      break;
                  }
                }
                ++game.version;
                game.updated_at = std::chrono::steady_clock::now();

                const auto ai_move = maybePlayAi(game);
                nlohmann::json payload =
                    gamePayload(game_id, game, viewer_color, ai_move);
                payload["ok"] = true;
                setJson(response, payload);
              });

  server.Post(R"(/api/games/([A-Za-z0-9]+)/reset)",
              [](const httplib::Request& request, httplib::Response& response) {
                nlohmann::json body = nlohmann::json::object();
                if (!request.body.empty()) {
                  try {
                    body = nlohmann::json::parse(request.body);
                  } catch (const nlohmann::json::exception&) {
                    setError(response, 400, "Niepoprawny JSON.");
                    return;
                  }
                }

                const std::string game_id = request.matches[1];
                const std::string player_id = playerIdFrom(request, body);

                std::lock_guard lock(g_mutex);
                const auto it = g_games.find(game_id);
                if (it == g_games.end()) {
                  setError(response, 404, "Nie znaleziono gry.");
                  return;
                }

                auto& game = it->second;
                if (game.mode == GameMode::Online) {
                  setError(response, 403, "Reset niedostepny w trybie online.");
                  return;
                }

                game.engine.startGame();
                game.last_move.clear();
                ++game.version;
                game.updated_at = std::chrono::steady_clock::now();
                const auto ai_move = maybePlayAi(game);
                nlohmann::json payload =
                    gamePayload(game_id, game,
                                game.vs_ai ? std::optional(game.player_color)
                                           : std::nullopt,
                                ai_move);
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
