#ifndef BOARD_8X8_HPP
#define BOARD_8X8_HPP

#include <array>
#include <optional>
#include <utility>

#include "board/board_common.hpp"

namespace board {

// Plansza 8x8 — granice sprawdzane arytmetycznie (liczenie indeksów 0..7).
class Board8x8 {
 public:
  static constexpr int kPlayableSize = board::kPlayableSize;
  static constexpr int kSize = board::kPlayableSize;

  Board8x8();

  bool inPlayableBounds(int file, int rank) const;
  bool isEmpty(int file, int rank) const;
  std::optional<Occupant> get(int file, int rank) const;
  bool set(int file, int rank, std::optional<Occupant> occupant);
  void clear();

  // Współrzędne wewnętrzne = współrzędne gry (brak offsetu).
  std::pair<int, int> toInternal(int file, int rank) const;
  std::pair<int, int> toPlayable(int x, int y) const;

  // Czy pole jest zablokowane: poza planszą (border przez liczenie) lub zajęte.
  bool isBlocked(int file, int rank) const;

 private:
  std::array<std::array<std::optional<Occupant>, kSize>, kSize> cells_{};
};

}  // namespace board

#endif
