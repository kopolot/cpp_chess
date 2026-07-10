#ifndef GAME_EN_PASSANT_HPP
#define GAME_EN_PASSANT_HPP

#include <cmath>
#include <optional>
#include <string>
#include <utility>

#include "board/board_concept.hpp"
#include "board/board_common.hpp"
#include "game/game_context.hpp"
#include "game/square.hpp"

namespace game {

template <board::PlayableBoard Board>
inline std::optional<std::pair<int, int>> resolveEnPassantDestination(
    const Board& board, const GameContext& context, int from_file, int from_rank, int to_file,
    int to_rank, const board::Occupant& piece) {
  if (!context.en_passant || piece.type != PieceType::Pawn) {
    return std::nullopt;
  }

  const int direction = piece.color == 0 ? 1 : -1;
  const auto [ep_file, ep_rank] = *context.en_passant;

  if (to_file == ep_file && to_rank == ep_rank) {
    return std::nullopt;
  }

  const auto victim = board.get(to_file, to_rank);
  if (!victim || victim->type != PieceType::Pawn || victim->color == piece.color) {
    return std::nullopt;
  }

  if (ep_file != to_file || ep_rank != to_rank + direction) {
    return std::nullopt;
  }

  if (from_rank != to_rank || std::abs(from_file - to_file) != 1) {
    return std::nullopt;
  }

  return *context.en_passant;
}

template <board::PlayableBoard Board>
inline std::optional<std::string> enPassantHint(const Board& board, const GameContext& context,
                                                int side_to_move) {
  if (!context.en_passant) {
    return std::nullopt;
  }

  const int direction = side_to_move == 0 ? 1 : -1;
  const auto [ep_file, ep_rank] = *context.en_passant;
  const int victim_rank = ep_rank - direction;

  std::string examples;
  for (const int from_file : {ep_file - 1, ep_file + 1}) {
    if (!board.inPlayableBounds(from_file, victim_rank)) {
      continue;
    }
    const auto pawn = board.get(from_file, victim_rank);
    if (!pawn || pawn->color != side_to_move || pawn->type != PieceType::Pawn) {
      continue;
    }

    const std::string standard =
        toNotation(from_file, victim_rank) + toNotation(ep_file, ep_rank);
    const std::string alias =
        toNotation(from_file, victim_rank) + toNotation(ep_file, victim_rank);

    if (!examples.empty()) {
      examples += "; ";
    }
    examples += standard + " (alias " + alias + ")";
  }

  if (examples.empty()) {
    return std::nullopt;
  }

  return "Bicie na przelocie: " + examples + " — tylko w tym ruchu";
}

}  // namespace game

#endif
