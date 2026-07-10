#ifndef BOARD_12X12_HPP
#define BOARD_12X12_HPP

#include <array>
#include <optional>
#include <utility>

#include "board/board_common.hpp"

namespace board {

// Plansza 12x12 z obramowaniem — granice jako ściany wykrywane kolizją.
// Obszar gry 8x8 leży wewnątrz (offset 2), bez osobnego sprawdzania zakresu przy ruchu.
class Board12x12 {
 public:
  static constexpr int kPlayableSize = board::kPlayableSize;
  static constexpr int kInternalSize = 12;
  static constexpr int kOffset = 2;

  Board12x12();

  bool inPlayableBounds(int file, int rank) const;
  bool isEmpty(int file, int rank) const;
  std::optional<Occupant> get(int file, int rank) const;
  bool set(int file, int rank, std::optional<Occupant> occupant);
  void clear();

  std::pair<int, int> toInternal(int file, int rank) const;
  std::pair<int, int> toPlayable(int x, int y) const;

  bool isBorderCell(int x, int y) const;
  bool isBlockedInternal(int x, int y) const;

  // Krok w kierunku (dx, dy) aż do kolizji; zwraca ostatnie wolne pole.
  std::pair<int, int> stepUntilBlocked(int x, int y, int dx, int dy) const;

 private:
  void initBorders();

  std::array<std::array<bool, kInternalSize>, kInternalSize> borders_{};
  std::array<std::array<std::optional<Occupant>, kInternalSize>, kInternalSize>
      cells_{};
};

}  // namespace board

#endif
