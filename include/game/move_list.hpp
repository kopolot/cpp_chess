#ifndef GAME_MOVE_LIST_HPP
#define GAME_MOVE_LIST_HPP

#include <optional>
#include <string>
#include <vector>

#include "board/board_common.hpp"
#include "board/board_concept.hpp"
#include "game/game_state.hpp"
#include "game/square.hpp"
#include "type/chess_piece.hpp"

namespace game {

struct Move {
  int from_file = 0;
  int from_rank = 0;
  int to_file = 0;
  int to_rank = 0;
  std::optional<PieceType> promotion;

  std::string toNotation() const {
    std::string out = game::toNotation(from_file, from_rank) +
                      game::toNotation(to_file, to_rank);
    if (promotion) {
      switch (*promotion) {
        case PieceType::Queen:
          out.push_back('q');
          break;
        case PieceType::Rook:
          out.push_back('r');
          break;
        case PieceType::Bishop:
          out.push_back('b');
          break;
        case PieceType::Knight:
          out.push_back('n');
          break;
        default:
          break;
      }
    }
    return out;
  }
};

template <board::PlayableBoard Board>
inline std::vector<Move> collectLegalMoves(const Board& board,
                                           const GameContext& context,
                                           int color) {
  std::vector<Move> moves;
  moves.reserve(40);

  Board scratch = board;
  GameContext scratch_context = context;

  for (int from_rank = 0; from_rank < board::kPlayableSize; ++from_rank) {
    for (int from_file = 0; from_file < board::kPlayableSize; ++from_file) {
      const auto piece = board.get(from_file, from_rank);
      if (!piece || piece->color != color) {
        continue;
      }

      for (int to_rank = 0; to_rank < board::kPlayableSize; ++to_rank) {
        for (int to_file = 0; to_file < board::kPlayableSize; ++to_file) {
          if (!isPseudoLegalMove(scratch, from_file, from_rank, to_file,
                                 to_rank, *piece, scratch_context)) {
            continue;
          }

          if (piece->type == PieceType::Pawn &&
              needsPromotion(from_rank, to_rank, piece->type, piece->color)) {
            for (const auto promo : {PieceType::Queen, PieceType::Rook,
                                     PieceType::Bishop, PieceType::Knight}) {
              if (!leavesKingInCheck(scratch, scratch_context, from_file,
                                     from_rank, to_file, to_rank, *piece,
                                     promo)) {
                moves.push_back(
                    Move{from_file, from_rank, to_file, to_rank, promo});
              }
            }
            continue;
          }

          if (!leavesKingInCheck(scratch, scratch_context, from_file, from_rank,
                                 to_file, to_rank, *piece)) {
            moves.push_back(
                Move{from_file, from_rank, to_file, to_rank, std::nullopt});
          }
        }
      }
    }
  }

  return moves;
}

}  // namespace game

#endif
