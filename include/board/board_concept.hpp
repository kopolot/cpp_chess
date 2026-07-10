#ifndef BOARD_CONCEPT_HPP
#define BOARD_CONCEPT_HPP

#include <concepts>
#include <optional>
#include <utility>

#include "board/board_common.hpp"

namespace board {

template <typename T>
concept PlayableBoard = requires(T board, int file, int rank,
                                 std::optional<Occupant> occupant) {
  { T::kPlayableSize } -> std::convertible_to<int>;
  { board.inPlayableBounds(file, rank) } -> std::convertible_to<bool>;
  { board.isEmpty(file, rank) } -> std::convertible_to<bool>;
  { board.get(file, rank) } -> std::same_as<std::optional<Occupant>>;
  { board.set(file, rank, occupant) } -> std::same_as<bool>;
  { board.clear() } -> std::same_as<void>;
  { board.toInternal(file, rank) } -> std::same_as<std::pair<int, int>>;
  { board.toPlayable(file, rank) } -> std::same_as<std::pair<int, int>>;
};

}  // namespace board

#endif
