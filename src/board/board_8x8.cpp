#include "board/board_8x8.hpp"

namespace board {

Board8x8::Board8x8() { clear(); }

bool Board8x8::inPlayableBounds(int file, int rank) const {
  return file >= 0 && file < kSize && rank >= 0 && rank < kSize;
}

bool Board8x8::isEmpty(int file, int rank) const {
  if (!inPlayableBounds(file, rank)) {
    return false;
  }
  return !cells_[rank][file].has_value();
}

std::optional<Occupant> Board8x8::get(int file, int rank) const {
  if (!inPlayableBounds(file, rank)) {
    return std::nullopt;
  }
  return cells_[rank][file];
}

bool Board8x8::set(int file, int rank, std::optional<Occupant> occupant) {
  if (!inPlayableBounds(file, rank)) {
    return false;
  }
  cells_[rank][file] = occupant;
  return true;
}

void Board8x8::clear() {
  for (auto& row : cells_) {
    row.fill(std::nullopt);
  }
}

std::pair<int, int> Board8x8::toInternal(int file, int rank) const {
  return {file, rank};
}

std::pair<int, int> Board8x8::toPlayable(int x, int y) const {
  return {x, y};
}

bool Board8x8::isBlocked(int file, int rank) const {
  if (!inPlayableBounds(file, rank)) {
    return true;
  }
  return cells_[rank][file].has_value();
}

}  // namespace board
