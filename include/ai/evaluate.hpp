#ifndef AI_EVALUATE_HPP
#define AI_EVALUATE_HPP

#include "board/board_common.hpp"
#include "board/board_concept.hpp"
#include "game/attack.hpp"
#include "type/chess_piece.hpp"

namespace ai {

inline constexpr int kMateScore = 100000;
inline constexpr int kStalemateScore = 0;

inline int pieceValue(PieceType type) {
  switch (type) {
    case PieceType::Pawn:
      return 100;
    case PieceType::Knight:
      return 320;
    case PieceType::Bishop:
      return 330;
    case PieceType::Rook:
      return 500;
    case PieceType::Queen:
      return 900;
    case PieceType::King:
      return 0;
  }
  return 0;
}

// Dodatnia ocena = korzystna dla białych.
template <board::PlayableBoard Board>
inline int evaluateMaterial(const Board& board) {
  int score = 0;
  for (int rank = 0; rank < board::kPlayableSize; ++rank) {
    for (int file = 0; file < board::kPlayableSize; ++file) {
      const auto piece = board.get(file, rank);
      if (!piece) {
        continue;
      }
      const int value = pieceValue(piece->type);
      score += (piece->color == 0) ? value : -value;
    }
  }
  return score;
}

template <board::PlayableBoard Board>
inline int evaluatePosition(const Board& board, int side_to_move) {
  int score = evaluateMaterial(board);
  if (game::isInCheck(board, side_to_move)) {
    score += (side_to_move == 0) ? -15 : 15;
  }
  return score;
}

}  // namespace ai

#endif
