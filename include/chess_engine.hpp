#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <optional>
#include <string>

#include "board/board_concept.hpp"
#include "game/board_display.hpp"
#include "game/game_context.hpp"
#include "game/game_state.hpp"
#include "game/move_validator.hpp"
#include "game/setup.hpp"
#include "game/square.hpp"

template <typename BoardType>
  requires board::PlayableBoard<BoardType>
class ChessEngine {
 public:
  ChessEngine() = default;
  ~ChessEngine() = default;

  void startGame();
  bool tryMove(int from_file, int from_rank, int to_file, int to_rank,
               std::optional<PieceType> promotion = std::nullopt);
  bool tryMove(const std::string& from, const std::string& to,
               std::optional<PieceType> promotion = std::nullopt);
  bool tryMoveNotation(const std::string& notation);
  bool needsPromotionForMove(int from_file, int from_rank, int to_file, int to_rank) const;

  int currentTurn() const { return current_turn_; }
  bool isGameOver() const { return game_over_; }
  game::GameResult gameResult() const { return game_result_; }
  const game::GameContext& getContext() const { return context_; }
  std::string gameStatusMessage() const {
    return game::gameResultMessage(game_result_, current_turn_);
  }

  BoardType& getBoard() { return board_; }
  const BoardType& getBoard() const { return board_; }
  std::string formatBoard() const { return game::formatBoard(board_); }
  std::string formatBoardStyled() const { return game::formatBoardStyled(board_); }
  std::string currentPlayerName() const { return game::colorName(current_turn_); }

 private:
  void recordPosition();
  void updateGameState();

  BoardType board_;
  game::GameContext context_;
  int current_turn_ = 0;
  game::GameResult game_result_ = game::GameResult::InProgress;
  bool game_over_ = false;
};

template <typename BoardType>
  requires board::PlayableBoard<BoardType>
void ChessEngine<BoardType>::startGame() {
  game::setupInitialPosition(board_);
  context_.reset();
  current_turn_ = 0;
  game_over_ = false;
  game_result_ = game::GameResult::InProgress;
  recordPosition();
}

template <typename BoardType>
  requires board::PlayableBoard<BoardType>
void ChessEngine<BoardType>::recordPosition() {
  context_.position_keys.push_back(
      game::makePositionKey(board_, current_turn_, context_));
}

template <typename BoardType>
  requires board::PlayableBoard<BoardType>
void ChessEngine<BoardType>::updateGameState() {
  game_result_ = game::evaluatePosition(board_, context_, current_turn_);
  game_over_ = game_result_ == game::GameResult::Checkmate ||
               game_result_ == game::GameResult::Stalemate ||
               game_result_ == game::GameResult::Draw;
}

template <typename BoardType>
  requires board::PlayableBoard<BoardType>
bool ChessEngine<BoardType>::needsPromotionForMove(int from_file, int from_rank, int to_file,
                                                   int to_rank) const {
  const auto piece = board_.get(from_file, from_rank);
  if (!piece) {
    return false;
  }
  return game::needsPromotion(from_rank, to_rank, piece->type, piece->color);
}

template <typename BoardType>
  requires board::PlayableBoard<BoardType>
bool ChessEngine<BoardType>::tryMove(int from_file, int from_rank, int to_file, int to_rank,
                                     std::optional<PieceType> promotion) {
  if (game_over_) {
    return false;
  }

  const auto piece = board_.get(from_file, from_rank);
  if (!piece || piece->color != current_turn_) {
    return false;
  }

  if (game::needsPromotion(from_rank, to_rank, piece->type, piece->color) && !promotion) {
    return false;
  }

  if (!game::isLegalMove(board_, context_, from_file, from_rank, to_file, to_rank, *piece,
                         promotion)) {
    return false;
  }

  game::applyMove(board_, from_file, from_rank, to_file, to_rank, *piece, context_, promotion);
  current_turn_ = 1 - current_turn_;
  recordPosition();
  updateGameState();
  return true;
}

template <typename BoardType>
  requires board::PlayableBoard<BoardType>
bool ChessEngine<BoardType>::tryMove(const std::string& from, const std::string& to,
                                     std::optional<PieceType> promotion) {
  const auto from_square = game::parseSquare(from);
  const auto to_square = game::parseSquare(to);
  if (!from_square || !to_square) {
    return false;
  }
  return tryMove(from_square->first, from_square->second, to_square->first, to_square->second,
                 promotion);
}

template <typename BoardType>
  requires board::PlayableBoard<BoardType>
bool ChessEngine<BoardType>::tryMoveNotation(const std::string& notation) {
  const auto parsed = game::parseMoveExtended(notation);
  if (!parsed) {
    return false;
  }
  return tryMove(parsed->from.first, parsed->from.second, parsed->to.first, parsed->to.second,
                 parsed->promotion);
}

#endif
