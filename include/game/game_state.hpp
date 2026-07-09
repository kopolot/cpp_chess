#ifndef GAME_GAME_STATE_HPP
#define GAME_GAME_STATE_HPP

#include <array>
#include <optional>
#include <string>
#include <utility>

#include "board/board_common.hpp"
#include "board/board_concept.hpp"
#include "game/move_validator.hpp"

namespace game {

enum class GameResult { InProgress, Check, Checkmate, Stalemate };

template <board::PlayableBoard Board>
struct AppliedMove {
  std::optional<board::Occupant> captured;
};

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

template <board::PlayableBoard Board>
inline AppliedMove<Board> applyMove(Board& board, int from_file, int from_rank, int to_file,
                                    int to_rank, const board::Occupant& piece) {
  AppliedMove<Board> delta;
  delta.captured = board.get(to_file, to_rank);
  board.set(from_file, from_rank, std::nullopt);

  board::Occupant moved = piece;
  if (moved.type == PieceType::Pawn && (to_rank == 0 || to_rank == 7)) {
    moved.type = PieceType::Queen;
  }
  board.set(to_file, to_rank, moved);
  return delta;
}

template <board::PlayableBoard Board>
inline void undoMove(Board& board, int from_file, int from_rank, int to_file, int to_rank,
                     const board::Occupant& piece, const AppliedMove<Board>& delta) {
  board.set(from_file, from_rank, piece);
  board.set(to_file, to_rank, delta.captured);
}

template <board::PlayableBoard Board>
inline bool leavesKingInCheck(Board& board, int from_file, int from_rank, int to_file,
                              int to_rank, const board::Occupant& piece) {
  const auto delta = applyMove(board, from_file, from_rank, to_file, to_rank, piece);
  const bool in_check = isInCheck(board, piece.color);
  undoMove(board, from_file, from_rank, to_file, to_rank, piece, delta);
  return in_check;
}

template <board::PlayableBoard Board>
inline bool isLegalMove(const Board& board, int from_file, int from_rank, int to_file,
                        int to_rank, const board::Occupant& piece) {
  Board copy = board;
  if (!isPseudoLegalMove(copy, from_file, from_rank, to_file, to_rank, piece)) {
    return false;
  }
  return !leavesKingInCheck(copy, from_file, from_rank, to_file, to_rank, piece);
}

template <board::PlayableBoard Board>
inline bool hasAnyLegalMove(const Board& board, int color) {
  for (int from_rank = 0; from_rank < board::kPlayableSize; ++from_rank) {
    for (int from_file = 0; from_file < board::kPlayableSize; ++from_file) {
      const auto piece = board.get(from_file, from_rank);
      if (!piece || piece->color != color) {
        continue;
      }
      for (int to_rank = 0; to_rank < board::kPlayableSize; ++to_rank) {
        for (int to_file = 0; to_file < board::kPlayableSize; ++to_file) {
          if (isLegalMove(board, from_file, from_rank, to_file, to_rank, *piece)) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

template <board::PlayableBoard Board>
inline GameResult evaluatePosition(const Board& board, int side_to_move) {
  const bool in_check = isInCheck(board, side_to_move);
  const bool has_moves = hasAnyLegalMove(board, side_to_move);
  if (in_check && !has_moves) {
    return GameResult::Checkmate;
  }
  if (!in_check && !has_moves) {
    return GameResult::Stalemate;
  }
  if (in_check) {
    return GameResult::Check;
  }
  return GameResult::InProgress;
}

inline std::string gameResultMessage(GameResult result, int side_to_move) {
  const std::string side = colorName(side_to_move);
  switch (result) {
    case GameResult::InProgress:
      return "";
    case GameResult::Check:
      return "Szach! (" + side + " pod szachem)";
    case GameResult::Checkmate:
      return "Mat! Wygrywaja " + colorName(1 - side_to_move) + ".";
    case GameResult::Stalemate:
      return "Pat! Remis.";
  }
  return "";
}

}  // namespace game

#endif
