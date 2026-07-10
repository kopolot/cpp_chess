#ifndef GAME_MOVE_VALIDATOR_HPP
#define GAME_MOVE_VALIDATOR_HPP

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "board/board_concept.hpp"
#include "board/board_common.hpp"
#include "game/attack.hpp"
#include "game/game_context.hpp"

namespace game {

template <board::PlayableBoard Board>
inline bool canCaptureOrLand(const Board& board, int file, int rank, int mover_color) {
  if (!board.inPlayableBounds(file, rank)) {
    return false;
  }
  const auto occupant = board.get(file, rank);
  if (!occupant) {
    return true;
  }
  return occupant->color != mover_color;
}

template <board::PlayableBoard Board>
inline bool isPathClear(const Board& board, int from_file, int from_rank, int to_file,
                        int to_rank) {
  const int delta_file = to_file - from_file;
  const int delta_rank = to_rank - from_rank;
  const int steps = std::max(std::abs(delta_file), std::abs(delta_rank));
  if (steps <= 1) {
    return true;
  }

  const int step_file = (delta_file == 0) ? 0 : delta_file / std::abs(delta_file);
  const int step_rank = (delta_rank == 0) ? 0 : delta_rank / std::abs(delta_rank);

  int file = from_file + step_file;
  int rank = from_rank + step_rank;
  for (int step = 1; step < steps; ++step) {
    if (!board.isEmpty(file, rank)) {
      return false;
    }
    file += step_file;
    rank += step_rank;
  }
  return true;
}

template <board::PlayableBoard Board>
inline bool isCastlePseudoLegal(const Board& board, int from_file, int from_rank, int to_file,
                                int to_rank, const board::Occupant& piece,
                                const GameContext& context) {
  if (piece.type != PieceType::King || from_file != 4) {
    return false;
  }

  const int home_rank = (piece.color == 0) ? 0 : 7;
  if (from_rank != home_rank || to_rank != home_rank || std::abs(to_file - from_file) != 2) {
    return false;
  }

  const bool kingside = kingsideCastle(from_file, to_file);
  if (piece.color == 0) {
    if (kingside && !context.castling.white_kingside) {
      return false;
    }
    if (!kingside && !context.castling.white_queenside) {
      return false;
    }
  } else {
    if (kingside && !context.castling.black_kingside) {
      return false;
    }
    if (!kingside && !context.castling.black_queenside) {
      return false;
    }
  }

  const int rook_file = kingside ? 7 : 0;
  const int rook_to = kingside ? 5 : 3;
  const auto rook = board.get(rook_file, home_rank);
  if (!rook || rook->type != PieceType::Rook || rook->color != piece.color) {
    return false;
  }

  const int step = kingside ? 1 : -1;
  for (int file = from_file + step; file != rook_file; file += step) {
    if (!board.isEmpty(file, home_rank)) {
      return false;
    }
  }
  if (!board.isEmpty(to_file, home_rank)) {
    return false;
  }

  if (isInCheck(board, piece.color)) {
    return false;
  }

  const int min_file = std::min(from_file, to_file);
  const int max_file = std::max(from_file, to_file);
  for (int file = min_file; file <= max_file; ++file) {
    if (isSquareAttacked(board, file, home_rank, 1 - piece.color)) {
      return false;
    }
  }

  return true;
}

template <board::PlayableBoard Board>
inline bool isPawnMoveLegal(const Board& board, int from_file, int from_rank, int to_file,
                            int to_rank, const board::Occupant& piece,
                            const GameContext& context) {
  const int direction = (piece.color == 0) ? 1 : -1;
  const int start_rank = (piece.color == 0) ? 1 : 6;
  const int delta_file = to_file - from_file;
  const int delta_rank = to_rank - from_rank;

  if (delta_file == 0 && delta_rank == direction && board.isEmpty(to_file, to_rank)) {
    return true;
  }

  if (delta_file == 0 && delta_rank == 2 * direction && from_rank == start_rank &&
      board.isEmpty(from_file, from_rank + direction) && board.isEmpty(to_file, to_rank)) {
    return true;
  }

  if (std::abs(delta_file) == 1 && delta_rank == direction) {
    const auto target = board.get(to_file, to_rank);
    if (target.has_value() && target->color != piece.color) {
      return true;
    }
    if (board.isEmpty(to_file, to_rank) && context.en_passant &&
        context.en_passant->first == to_file && context.en_passant->second == to_rank) {
      return true;
    }
  }

  return false;
}

template <board::PlayableBoard Board>
inline bool isKnightMoveLegal(const Board& board, int from_file, int from_rank, int to_file,
                              int to_rank, const board::Occupant& piece) {
  const int delta_file = std::abs(to_file - from_file);
  const int delta_rank = std::abs(to_rank - from_rank);
  if (!((delta_file == 2 && delta_rank == 1) || (delta_file == 1 && delta_rank == 2))) {
    return false;
  }
  return canCaptureOrLand(board, to_file, to_rank, piece.color);
}

template <board::PlayableBoard Board>
inline bool isKingMoveLegal(const Board& board, int from_file, int from_rank, int to_file,
                            int to_rank, const board::Occupant& piece,
                            const GameContext& context) {
  const int delta_file = to_file - from_file;
  const int delta_rank = to_rank - from_rank;
  if (std::abs(delta_file) == 2 && delta_rank == 0) {
    return isCastlePseudoLegal(board, from_file, from_rank, to_file, to_rank, piece, context);
  }

  if (std::abs(delta_file) > 1 || std::abs(delta_rank) > 1 || (delta_file == 0 && delta_rank == 0)) {
    return false;
  }
  return canCaptureOrLand(board, to_file, to_rank, piece.color);
}

template <board::PlayableBoard Board>
inline bool isBishopMoveLegal(const Board& board, int from_file, int from_rank, int to_file,
                              int to_rank, const board::Occupant& piece) {
  const int delta_file = to_file - from_file;
  const int delta_rank = to_rank - from_rank;
  if (std::abs(delta_file) != std::abs(delta_rank) || delta_file == 0) {
    return false;
  }
  if (!isPathClear(board, from_file, from_rank, to_file, to_rank)) {
    return false;
  }
  return canCaptureOrLand(board, to_file, to_rank, piece.color);
}

template <board::PlayableBoard Board>
inline bool isRookMoveLegal(const Board& board, int from_file, int from_rank, int to_file,
                            int to_rank, const board::Occupant& piece) {
  const int delta_file = to_file - from_file;
  const int delta_rank = to_rank - from_rank;
  if ((delta_file != 0 && delta_rank != 0) || (delta_file == 0 && delta_rank == 0)) {
    return false;
  }
  if (!isPathClear(board, from_file, from_rank, to_file, to_rank)) {
    return false;
  }
  return canCaptureOrLand(board, to_file, to_rank, piece.color);
}

template <board::PlayableBoard Board>
inline bool isQueenMoveLegal(const Board& board, int from_file, int from_rank, int to_file,
                             int to_rank, const board::Occupant& piece) {
  return isBishopMoveLegal(board, from_file, from_rank, to_file, to_rank, piece) ||
         isRookMoveLegal(board, from_file, from_rank, to_file, to_rank, piece);
}

template <board::PlayableBoard Board>
inline bool isPseudoLegalMove(const Board& board, int from_file, int from_rank, int to_file,
                              int to_rank, const board::Occupant& piece,
                              const GameContext& context) {
  if (from_file == to_file && from_rank == to_rank) {
    return false;
  }
  if (!board.inPlayableBounds(from_file, from_rank) ||
      !board.inPlayableBounds(to_file, to_rank)) {
    return false;
  }

  switch (piece.type) {
    case PieceType::Pawn:
      return isPawnMoveLegal(board, from_file, from_rank, to_file, to_rank, piece, context);
    case PieceType::Knight:
      return isKnightMoveLegal(board, from_file, from_rank, to_file, to_rank, piece);
    case PieceType::Bishop:
      return isBishopMoveLegal(board, from_file, from_rank, to_file, to_rank, piece);
    case PieceType::Rook:
      return isRookMoveLegal(board, from_file, from_rank, to_file, to_rank, piece);
    case PieceType::Queen:
      return isQueenMoveLegal(board, from_file, from_rank, to_file, to_rank, piece);
    case PieceType::King:
      return isKingMoveLegal(board, from_file, from_rank, to_file, to_rank, piece, context);
  }
  return false;
}

}  // namespace game

#endif
