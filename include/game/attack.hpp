#ifndef GAME_ATTACK_HPP
#define GAME_ATTACK_HPP

#include <array>
#include <optional>
#include <utility>

#include "board/board_common.hpp"
#include "board/board_concept.hpp"

namespace game {

template <board::PlayableBoard Board>
inline std::optional<std::pair<int, int>> findKing(const Board& board, int color) {
  for (int rank = 0; rank < board::kPlayableSize; ++rank) {
    for (int file = 0; file < board::kPlayableSize; ++file) {
      const auto occupant = board.get(file, rank);
      if (occupant && occupant->color == color && occupant->type == PieceType::King) {
        return std::make_pair(file, rank);
      }
    }
  }
  return std::nullopt;
}

template <board::PlayableBoard Board>
inline bool isSquareAttacked(const Board& board, int file, int rank, int attacker_color) {
  const int pawn_rank = rank + (attacker_color == 0 ? -1 : 1);
  if (board.inPlayableBounds(file, pawn_rank)) {
    for (const int delta_file : {-1, 1}) {
      const int pawn_file = file + delta_file;
      if (!board.inPlayableBounds(pawn_file, pawn_rank)) {
        continue;
      }
      const auto occupant = board.get(pawn_file, pawn_rank);
      if (occupant && occupant->color == attacker_color &&
          occupant->type == PieceType::Pawn) {
        return true;
      }
    }
  }

  constexpr std::array<std::pair<int, int>, 8> kKnightOffsets = {
      {{1, 2}, {2, 1}, {2, -1}, {1, -2}, {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}}};
  for (const auto& [delta_file, delta_rank] : kKnightOffsets) {
    const int knight_file = file + delta_file;
    const int knight_rank = rank + delta_rank;
    if (!board.inPlayableBounds(knight_file, knight_rank)) {
      continue;
    }
    const auto occupant = board.get(knight_file, knight_rank);
    if (occupant && occupant->color == attacker_color &&
        occupant->type == PieceType::Knight) {
      return true;
    }
  }

  constexpr std::array<std::pair<int, int>, 8> kKingOffsets = {
      {{0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}}};
  for (const auto& [delta_file, delta_rank] : kKingOffsets) {
    const int king_file = file + delta_file;
    const int king_rank = rank + delta_rank;
    if (!board.inPlayableBounds(king_file, king_rank)) {
      continue;
    }
    const auto occupant = board.get(king_file, king_rank);
    if (occupant && occupant->color == attacker_color && occupant->type == PieceType::King) {
      return true;
    }
  }

  constexpr std::array<std::pair<int, int>, 4> kDiagonalDirs = {
      {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}}};
  for (const auto& [delta_file, delta_rank] : kDiagonalDirs) {
    int ray_file = file + delta_file;
    int ray_rank = rank + delta_rank;
    while (board.inPlayableBounds(ray_file, ray_rank)) {
      const auto occupant = board.get(ray_file, ray_rank);
      if (occupant) {
        if (occupant->color == attacker_color &&
            (occupant->type == PieceType::Bishop || occupant->type == PieceType::Queen)) {
          return true;
        }
        break;
      }
      ray_file += delta_file;
      ray_rank += delta_rank;
    }
  }

  constexpr std::array<std::pair<int, int>, 4> kStraightDirs = {
      {{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
  for (const auto& [delta_file, delta_rank] : kStraightDirs) {
    int ray_file = file + delta_file;
    int ray_rank = rank + delta_rank;
    while (board.inPlayableBounds(ray_file, ray_rank)) {
      const auto occupant = board.get(ray_file, ray_rank);
      if (occupant) {
        if (occupant->color == attacker_color &&
            (occupant->type == PieceType::Rook || occupant->type == PieceType::Queen)) {
          return true;
        }
        break;
      }
      ray_file += delta_file;
      ray_rank += delta_rank;
    }
  }

  return false;
}

template <board::PlayableBoard Board>
inline bool isInCheck(const Board& board, int color) {
  const auto king = findKing(board, color);
  if (!king) {
    return false;
  }
  return isSquareAttacked(board, king->first, king->second, 1 - color);
}

}  // namespace game

#endif
