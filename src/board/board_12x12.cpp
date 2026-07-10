#include "board/board_12x12.hpp"

namespace board {

Board12x12::Board12x12() {
  initBorders();
  clear();
}

void Board12x12::initBorders() {
  for (int y = 0; y < kInternalSize; ++y) {
    for (int x = 0; x < kInternalSize; ++x) {
      const bool top_or_bottom = y < kOffset || y >= kInternalSize - kOffset;
      const bool left_or_right = x < kOffset || x >= kInternalSize - kOffset;
      borders_[y][x] = top_or_bottom || left_or_right;
    }
  }
}

bool Board12x12::inPlayableBounds(int file, int rank) const {
  return file >= 0 && file < kPlayableSize && rank >= 0 && rank < kPlayableSize;
}

bool Board12x12::isEmpty(int file, int rank) const {
  if (!inPlayableBounds(file, rank)) {
    return false;
  }
  const auto [x, y] = toInternal(file, rank);
  return !isBlockedInternal(x, y);
}

std::optional<Occupant> Board12x12::get(int file, int rank) const {
  if (!inPlayableBounds(file, rank)) {
    return std::nullopt;
  }
  const auto [x, y] = toInternal(file, rank);
  if (borders_[y][x]) {
    return std::nullopt;
  }
  return cells_[y][x];
}

bool Board12x12::set(int file, int rank, std::optional<Occupant> occupant) {
  if (!inPlayableBounds(file, rank)) {
    return false;
  }
  const auto [x, y] = toInternal(file, rank);
  if (borders_[y][x]) {
    return false;
  }
  cells_[y][x] = occupant;
  return true;
}

void Board12x12::clear() {
  for (auto& row : cells_) {
    row.fill(std::nullopt);
  }
}

std::pair<int, int> Board12x12::toInternal(int file, int rank) const {
  return {file + kOffset, rank + kOffset};
}

std::pair<int, int> Board12x12::toPlayable(int x, int y) const {
  return {x - kOffset, y - kOffset};
}

bool Board12x12::isBorderCell(int x, int y) const {
  if (x < 0 || x >= kInternalSize || y < 0 || y >= kInternalSize) {
    return true;
  }
  return borders_[y][x];
}

bool Board12x12::isBlockedInternal(int x, int y) const {
  if (x < 0 || x >= kInternalSize || y < 0 || y >= kInternalSize) {
    return true;
  }
  return borders_[y][x] || cells_[y][x].has_value();
}

std::pair<int, int> Board12x12::stepUntilBlocked(int x, int y, int dx,
                                                 int dy) const {
  int current_x = x;
  int current_y = y;

  while (true) {
    const int next_x = current_x + dx;
    const int next_y = current_y + dy;
    if (isBlockedInternal(next_x, next_y)) {
      return {current_x, current_y};
    }
    current_x = next_x;
    current_y = next_y;
  }
}

}  // namespace board
