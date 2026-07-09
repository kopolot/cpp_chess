#ifndef GAME_SETUP_HPP
#define GAME_SETUP_HPP

#include "board/board_concept.hpp"
#include "board/board_common.hpp"
#include "type/chess_piece.hpp"

namespace game {

template <board::PlayableBoard Board>
inline void setupInitialPosition(Board& board) {
  board.clear();

  constexpr PieceType back_rank[] = {PieceType::Rook,   PieceType::Knight, PieceType::Bishop,
                                   PieceType::Queen,  PieceType::King,   PieceType::Bishop,
                                   PieceType::Knight, PieceType::Rook};

  for (int file = 0; file < board::kPlayableSize; ++file) {
    board.set(file, 1, board::Occupant{0, PieceType::Pawn});
    board.set(file, 6, board::Occupant{1, PieceType::Pawn});
    board.set(file, 0, board::Occupant{0, back_rank[file]});
    board.set(file, 7, board::Occupant{1, back_rank[file]});
  }
}

}  // namespace game

#endif
