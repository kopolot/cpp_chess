#ifndef GAME_GAME_STATE_HPP
#define GAME_GAME_STATE_HPP

#include <optional>
#include <string>
#include <utility>

#include "board/board_common.hpp"
#include "board/board_concept.hpp"
#include "game/attack.hpp"
#include "game/board_display.hpp"
#include "game/game_context.hpp"
#include "game/move_validator.hpp"
#include "game/square.hpp"

namespace game {

enum class GameResult {
  InProgress,
  Check,
  Checkmate,
  Stalemate,
  Draw,
  Resignation
};

template <board::PlayableBoard Board>
struct AppliedMove {
  std::optional<board::Occupant> captured;
  bool was_en_passant = false;
  bool was_castle = false;
  int rook_from_file = -1;
  int rook_to_file = -1;
  int rook_rank = -1;
};

inline void updateCastlingRightsForMove(
    CastlingRights& rights, int from_file, int from_rank, int to_file,
    int to_rank, const board::Occupant& piece,
    const std::optional<board::Occupant>& captured) {
  if (piece.type == PieceType::King) {
    if (piece.color == 0) {
      rights.white_kingside = false;
      rights.white_queenside = false;
    } else {
      rights.black_kingside = false;
      rights.black_queenside = false;
    }
  }

  if (piece.type == PieceType::Rook) {
    if (piece.color == 0 && from_rank == 0) {
      if (from_file == 0) {
        rights.white_queenside = false;
      }
      if (from_file == 7) {
        rights.white_kingside = false;
      }
    }
    if (piece.color == 1 && from_rank == 7) {
      if (from_file == 0) {
        rights.black_queenside = false;
      }
      if (from_file == 7) {
        rights.black_kingside = false;
      }
    }
  }

  if (captured && captured->type == PieceType::Rook) {
    if (captured->color == 0) {
      if (to_file == 0 && to_rank == 0) {
        rights.white_queenside = false;
      }
      if (to_file == 7 && to_rank == 0) {
        rights.white_kingside = false;
      }
    } else {
      if (to_file == 0 && to_rank == 7) {
        rights.black_queenside = false;
      }
      if (to_file == 7 && to_rank == 7) {
        rights.black_kingside = false;
      }
    }
  }

  if (piece.type == PieceType::King && std::abs(to_file - from_file) == 2) {
    if (piece.color == 0) {
      rights.white_kingside = false;
      rights.white_queenside = false;
    } else {
      rights.black_kingside = false;
      rights.black_queenside = false;
    }
  }
}

template <board::PlayableBoard Board>
inline AppliedMove<Board> applyMove(
    Board& board, int from_file, int from_rank, int to_file, int to_rank,
    const board::Occupant& piece, GameContext& context,
    std::optional<PieceType> promotion = std::nullopt) {
  AppliedMove<Board> delta;
  delta.captured = board.get(to_file, to_rank);

  const int direction = (piece.color == 0) ? 1 : -1;
  if (piece.type == PieceType::Pawn && board.isEmpty(to_file, to_rank) &&
      context.en_passant && context.en_passant->first == to_file &&
      context.en_passant->second == to_rank) {
    const int captured_rank = to_rank - direction;
    delta.captured = board.get(to_file, captured_rank);
    delta.was_en_passant = true;
    board.set(to_file, captured_rank, std::nullopt);
  }

  board.set(from_file, from_rank, std::nullopt);

  if (piece.type == PieceType::King && std::abs(to_file - from_file) == 2) {
    const int home_rank = from_rank;
    const bool kingside = kingsideCastle(from_file, to_file);
    const int rook_from = kingside ? 7 : 0;
    const int rook_to = kingside ? 5 : 3;
    const auto rook = board.get(rook_from, home_rank);
    board.set(rook_from, home_rank, std::nullopt);
    board.set(rook_to, home_rank, rook);
    delta.was_castle = true;
    delta.rook_from_file = rook_from;
    delta.rook_to_file = rook_to;
    delta.rook_rank = home_rank;
  }

  board::Occupant moved = piece;
  if (moved.type == PieceType::Pawn && (to_rank == 0 || to_rank == 7)) {
    moved.type = promotion.value_or(PieceType::Queen);
  }
  board.set(to_file, to_rank, moved);

  const bool pawn_move = piece.type == PieceType::Pawn;
  const bool capture = delta.captured.has_value();
  if (pawn_move || capture) {
    context.halfmove_clock = 0;
  } else {
    ++context.halfmove_clock;
  }

  context.en_passant = std::nullopt;
  if (piece.type == PieceType::Pawn && std::abs(to_rank - from_rank) == 2) {
    context.en_passant = std::make_pair(from_file, from_rank + direction);
  }

  updateCastlingRightsForMove(context.castling, from_file, from_rank, to_file,
                              to_rank, piece, delta.captured);

  return delta;
}

template <board::PlayableBoard Board>
inline void undoMove(Board& board, int from_file, int from_rank, int to_file,
                     int to_rank, const board::Occupant& piece,
                     const AppliedMove<Board>& delta) {
  if (delta.was_castle) {
    const auto rook = board.get(delta.rook_to_file, delta.rook_rank);
    board.set(delta.rook_to_file, delta.rook_rank, std::nullopt);
    board.set(delta.rook_from_file, delta.rook_rank, rook);
  }

  if (delta.was_en_passant) {
    const int direction = (piece.color == 0) ? 1 : -1;
    board.set(to_file, to_rank, std::nullopt);
    board.set(to_file, to_rank - direction, delta.captured);
    board.set(from_file, from_rank, piece);
    return;
  }

  board.set(to_file, to_rank, delta.captured);
  board.set(from_file, from_rank, piece);
}

template <board::PlayableBoard Board>
inline bool leavesKingInCheck(
    Board& board, GameContext& context, int from_file, int from_rank,
    int to_file, int to_rank, const board::Occupant& piece,
    std::optional<PieceType> promotion = std::nullopt) {
  GameContext snapshot = context;
  const auto delta = applyMove(board, from_file, from_rank, to_file, to_rank,
                               piece, snapshot, promotion);
  const bool in_check = isInCheck(board, piece.color);
  undoMove(board, from_file, from_rank, to_file, to_rank, piece, delta);
  return in_check;
}

template <board::PlayableBoard Board>
inline bool isLegalMove(const Board& board, const GameContext& context,
                        int from_file, int from_rank, int to_file, int to_rank,
                        const board::Occupant& piece,
                        std::optional<PieceType> promotion = std::nullopt) {
  Board copy = board;
  GameContext copy_context = context;
  if (!isPseudoLegalMove(copy, from_file, from_rank, to_file, to_rank, piece,
                         copy_context)) {
    return false;
  }
  if (piece.type == PieceType::Pawn &&
      needsPromotion(from_rank, to_rank, piece.type, piece.color) &&
      !promotion) {
    return false;
  }
  return !leavesKingInCheck(copy, copy_context, from_file, from_rank, to_file,
                            to_rank, piece, promotion);
}

template <board::PlayableBoard Board>
inline bool hasAnyLegalMove(const Board& board, const GameContext& context,
                            int color) {
  for (int from_rank = 0; from_rank < board::kPlayableSize; ++from_rank) {
    for (int from_file = 0; from_file < board::kPlayableSize; ++from_file) {
      const auto piece = board.get(from_file, from_rank);
      if (!piece || piece->color != color) {
        continue;
      }
      for (int to_rank = 0; to_rank < board::kPlayableSize; ++to_rank) {
        for (int to_file = 0; to_file < board::kPlayableSize; ++to_file) {
          if (isLegalMove(board, context, from_file, from_rank, to_file,
                          to_rank, *piece)) {
            return true;
          }
          if (piece->type == PieceType::Pawn &&
              needsPromotion(from_rank, to_rank, piece->type, piece->color)) {
            for (const auto promo : {PieceType::Queen, PieceType::Rook,
                                     PieceType::Bishop, PieceType::Knight}) {
              if (isLegalMove(board, context, from_file, from_rank, to_file,
                              to_rank, *piece, promo)) {
                return true;
              }
            }
          }
        }
      }
    }
  }
  return false;
}

template <board::PlayableBoard Board>
inline GameResult evaluatePosition(const Board& board,
                                   const GameContext& context,
                                   int side_to_move) {
  if (context.halfmove_clock >= 100) {
    return GameResult::Draw;
  }

  const std::string key = makePositionKey(board, side_to_move, context);
  if (countPositionRepetitions(context.position_keys, key) >= 3) {
    return GameResult::Draw;
  }

  const bool in_check = isInCheck(board, side_to_move);
  const bool has_moves = hasAnyLegalMove(board, context, side_to_move);
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
  switch (result) {
    case GameResult::InProgress:
      return "";
    case GameResult::Check:
      return "Szach! (" + colorName(side_to_move) + " pod szachem)";
    case GameResult::Checkmate:
      return "Mat! Wygrywaja " + colorName(1 - side_to_move) + ".";
    case GameResult::Stalemate:
      return "Pat! Remis.";
    case GameResult::Draw:
      return "Remis (regula 50 ruchow lub 3-krotne powtorzenie).";
    case GameResult::Resignation:
      return "Poddanie! Wygrywaja " + colorName(1 - side_to_move) + ".";
  }
  return "";
}

}  // namespace game

#endif
