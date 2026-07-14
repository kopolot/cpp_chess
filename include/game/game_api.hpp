#ifndef GAME_GAME_API_HPP
#define GAME_GAME_API_HPP

#include <nlohmann/json.hpp>
#include <optional>
#include <string>

#include "board/board_common.hpp"
#include "board/board_concept.hpp"
#include "chess_engine.hpp"
#include "game/board_display.hpp"
#include "game/square.hpp"

namespace game::api {

inline const char* pieceTypeName(PieceType type) {
  switch (type) {
    case PieceType::Pawn:
      return "pawn";
    case PieceType::Knight:
      return "knight";
    case PieceType::Bishop:
      return "bishop";
    case PieceType::Rook:
      return "rook";
    case PieceType::Queen:
      return "queen";
    case PieceType::King:
      return "king";
  }
  return "pawn";
}

inline const char* colorNameEn(int color) {
  return color == 0 ? "white" : "black";
}

inline const char* gameResultName(game::GameResult result) {
  switch (result) {
    case game::GameResult::InProgress:
      return "in_progress";
    case game::GameResult::Check:
      return "check";
    case game::GameResult::Checkmate:
      return "checkmate";
    case game::GameResult::Stalemate:
      return "stalemate";
    case game::GameResult::Draw:
      return "draw";
    case game::GameResult::Resignation:
      return "resignation";
  }
  return "in_progress";
}

inline nlohmann::json occupantJson(
    const std::optional<board::Occupant>& occupant) {
  if (!occupant) {
    return nullptr;
  }
  return nlohmann::json{
      {"color", colorNameEn(occupant->color)},
      {"type", pieceTypeName(occupant->type)},
      {"symbol", std::string(1, game::pieceToChar(*occupant))},
      {"unicode", game::pieceToUnicode(*occupant)}};
}

template <board::PlayableBoard Board>
inline nlohmann::json boardJson(const Board& board) {
  nlohmann::json ranks = nlohmann::json::array();
  for (int rank = board::kPlayableSize - 1; rank >= 0; --rank) {
    nlohmann::json files = nlohmann::json::array();
    for (int file = 0; file < board::kPlayableSize; ++file) {
      files.push_back(occupantJson(board.get(file, rank)));
    }
    ranks.push_back(files);
  }
  return ranks;
}

template <board::PlayableBoard Board>
inline nlohmann::json gameStateJson(const ChessEngine<Board>& engine) {
  nlohmann::json state{{"turn", engine.currentTurn()},
                       {"turnName", game::colorName(engine.currentTurn())},
                       {"result", gameResultName(engine.gameResult())},
                       {"message", engine.gameStatusMessage()},
                       {"gameOver", engine.isGameOver()},
                       {"board", boardJson(engine.getBoard())}};

  if (const auto hint = engine.enPassantHint()) {
    state["enPassantHint"] = *hint;
  } else {
    state["enPassantHint"] = nullptr;
  }

  return state;
}

struct MoveRequest {
  std::string from;
  std::string to;
  std::optional<PieceType> promotion;
};

inline std::optional<MoveRequest> parseMoveRequest(const nlohmann::json& body) {
  if (!body.contains("from") || !body.contains("to") ||
      !body["from"].is_string() || !body["to"].is_string()) {
    return std::nullopt;
  }

  MoveRequest request{body["from"].get<std::string>(),
                      body["to"].get<std::string>(), std::nullopt};

  if (body.contains("promotion") && !body["promotion"].is_null()) {
    if (!body["promotion"].is_string()) {
      return std::nullopt;
    }
    request.promotion =
        game::parsePromotionChar(body["promotion"].get<std::string>()[0]);
    if (!request.promotion) {
      return std::nullopt;
    }
  }

  return request;
}

}  // namespace game::api

#endif
