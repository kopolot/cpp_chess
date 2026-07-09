#ifndef GAME_BOARD_DISPLAY_HPP
#define GAME_BOARD_DISPLAY_HPP

#include <cctype>
#include <sstream>
#include <string>

#include "board/board_concept.hpp"
#include "board/board_common.hpp"

namespace game {

inline char pieceToChar(const board::Occupant& piece) {
  char symbol = '.';
  switch (piece.type) {
    case PieceType::Pawn:
      symbol = 'p';
      break;
    case PieceType::Knight:
      symbol = 'n';
      break;
    case PieceType::Bishop:
      symbol = 'b';
      break;
    case PieceType::Rook:
      symbol = 'r';
      break;
    case PieceType::Queen:
      symbol = 'q';
      break;
    case PieceType::King:
      symbol = 'k';
      break;
  }
  if (piece.color == 0) {
    symbol = static_cast<char>(std::toupper(symbol));
  }
  return symbol;
}

template <board::PlayableBoard Board>
inline std::string formatBoard(const Board& board) {
  std::ostringstream out;
  for (int rank = board::kPlayableSize - 1; rank >= 0; --rank) {
    out << (rank + 1) << ' ';
    for (int file = 0; file < board::kPlayableSize; ++file) {
      const auto occupant = board.get(file, rank);
      out << (occupant ? pieceToChar(*occupant) : '.') << ' ';
    }
    out << '\n';
  }
  out << "  a b c d e f g h\n";
  return out.str();
}

inline std::string colorName(int color) { return color == 0 ? "biale" : "czarne"; }

}  // namespace game

#endif
