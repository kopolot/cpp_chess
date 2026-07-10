#ifndef BOARD_COMMON_HPP
#define BOARD_COMMON_HPP

#include <cstdint>
#include <optional>

#include "type/chess_piece.hpp"

namespace board {

struct Occupant {
  int color;  // 0 - biały, 1 - czarny
  PieceType type;

  bool operator==(const Occupant& other) const {
    return color == other.color && type == other.type;
  }
};

inline constexpr int kPlayableSize = 8;

}  // namespace board

#endif
