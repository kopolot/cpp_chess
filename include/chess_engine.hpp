#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <string>

#include "board/board_concept.hpp"
#include "game/board_display.hpp"
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
  bool tryMove(int from_file, int from_rank, int to_file, int to_rank);
  bool tryMove(const std::string& from, const std::string& to);
  bool tryMoveNotation(const std::string& notation);

  int currentTurn() const { return current_turn_; }
  bool isGameOver() const { return game_over_; }
  game::GameResult gameResult() const { return game_result_; }
  std::string gameStatusMessage() const {
    return game::gameResultMessage(game_result_, current_turn_);
  }

  BoardType& getBoard() { return board_; }
  const BoardType& getBoard() const { return board_; }
  std::string formatBoard() const { return game::formatBoard(board_); }
  std::string currentPlayerName() const { return game::colorName(current_turn_); }

 private:
  void updateGameState();

  BoardType board_;
  int current_turn_ = 0;
  game::GameResult game_result_ = game::GameResult::InProgress;
  bool game_over_ = false;
};

template <typename BoardType>
  requires board::PlayableBoard<BoardType>
void ChessEngine<BoardType>::startGame() {
  game::setupInitialPosition(board_);
  current_turn_ = 0;
  game_over_ = false;
  game_result_ = game::GameResult::InProgress;
}

template <typename BoardType>
  requires board::PlayableBoard<BoardType>
void ChessEngine<BoardType>::updateGameState() {
  game_result_ = game::evaluatePosition(board_, current_turn_);
  game_over_ = game_result_ == game::GameResult::Checkmate ||
               game_result_ == game::GameResult::Stalemate;
}

template <typename BoardType>
  requires board::PlayableBoard<BoardType>
bool ChessEngine<BoardType>::tryMove(int from_file, int from_rank, int to_file,
                                     int to_rank) {
  if (game_over_) {
    return false;
  }

  const auto piece = board_.get(from_file, from_rank);
  if (!piece || piece->color != current_turn_) {
    return false;
  }
  if (!game::isLegalMove(board_, from_file, from_rank, to_file, to_rank, *piece)) {
    return false;
  }

  game::applyMove(board_, from_file, from_rank, to_file, to_rank, *piece);
  current_turn_ = 1 - current_turn_;
  updateGameState();
  return true;
}

template <typename BoardType>
  requires board::PlayableBoard<BoardType>
bool ChessEngine<BoardType>::tryMove(const std::string& from, const std::string& to) {
  const auto from_square = game::parseSquare(from);
  const auto to_square = game::parseSquare(to);
  if (!from_square || !to_square) {
    return false;
  }
  return tryMove(from_square->first, from_square->second, to_square->first,
                 to_square->second);
}

template <typename BoardType>
  requires board::PlayableBoard<BoardType>
bool ChessEngine<BoardType>::tryMoveNotation(const std::string& notation) {
  const auto parsed = game::parseMove(notation);
  if (!parsed) {
    return false;
  }
  return tryMove(parsed->first.first, parsed->first.second, parsed->second.first,
                 parsed->second.second);
}

#endif
