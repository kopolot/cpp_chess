#ifndef GAME_BOARD_DISPLAY_HPP
#define GAME_BOARD_DISPLAY_HPP

#include <cctype>
#include <sstream>
#include <string>
#include <string_view>

#include "board/board_common.hpp"
#include "board/board_concept.hpp"
#include "game/terminal_style.hpp"

namespace game {

inline char pieceToChar(const board::Occupant& piece) {
  char symbol = ' ';
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

inline const char* pieceToUnicode(const board::Occupant& piece) {
  static constexpr const char* kWhite[] = {"♙", "♘", "♗", "♖", "♕", "♔"};
  static constexpr const char* kBlack[] = {"♟", "♞", "♝", "♜", "♛", "♚"};
  const auto index = static_cast<std::size_t>(piece.type);
  return piece.color == 0 ? kWhite[index] : kBlack[index];
}

inline bool isLightSquare(int file, int rank) { return (file + rank) % 2 == 0; }

template <board::PlayableBoard Board>
inline std::string formatBoardCell(const Board& board, int file, int rank, bool styled) {
  const auto occupant = board.get(file, rank);
  if (!styled || !terminal::colorsEnabled()) {
    return occupant ? std::string(" ") + pieceToUnicode(*occupant) + ' ' : "   ";
  }

  const bool light = isLightSquare(file, rank);
  const auto background =
      light ? terminal::backgroundRgb(238, 238, 210) : terminal::backgroundRgb(181, 136, 99);
  const char* foreground = occupant ? (occupant->color == 0 ? terminal::kWhitePiece
                                                            : terminal::kBlackPiece)
                                    : "";
  const std::string piece = occupant ? pieceToUnicode(*occupant) : " ";
  return background + foreground + ' ' + piece + ' ' + terminal::kReset;
}

template <board::PlayableBoard Board>
inline std::string formatBoard(const Board& board, bool styled = false) {
  const bool use_unicode = styled;
  const char* top_left = use_unicode ? "┌" : "+";
  const char* top_tee = use_unicode ? "┬" : "+";
  const char* top_right = use_unicode ? "┐" : "+";
  const char* mid_left = use_unicode ? "├" : "+";
  const char* mid_tee = use_unicode ? "┼" : "+";
  const char* mid_right = use_unicode ? "┤" : "+";
  const char* bot_left = use_unicode ? "└" : "+";
  const char* bot_tee = use_unicode ? "┴" : "+";
  const char* bot_right = use_unicode ? "┘" : "+";
  const char* horizontal = use_unicode ? "───" : "---";
  const char* vertical = use_unicode ? "│" : "|";

  const bool color_border = styled && terminal::colorsEnabled();
  const std::string border_on = color_border ? terminal::kBorder : "";
  const std::string border_off = color_border ? terminal::kReset : "";
  const std::string label_on = color_border ? terminal::kLabel : "";
  const std::string label_off = color_border ? terminal::kReset : "";

  std::ostringstream out;

  out << label_on << "    a   b   c   d   e   f   g   h" << label_off << '\n';
  out << border_on << "  " << top_left;
  for (int file = 0; file < board::kPlayableSize; ++file) {
    out << horizontal << (file + 1 < board::kPlayableSize ? top_tee : top_right);
  }
  out << border_off << '\n';

  for (int rank = board::kPlayableSize - 1; rank >= 0; --rank) {
    out << label_on << (rank + 1) << border_off << border_on << ' ' << vertical;
    for (int file = 0; file < board::kPlayableSize; ++file) {
      if (styled) {
        out << formatBoardCell(board, file, rank, true) << border_on << vertical;
      } else {
        const auto occupant = board.get(file, rank);
        out << ' ' << (occupant ? pieceToChar(*occupant) : ' ') << ' ' << border_on << vertical;
      }
    }
    out << border_off << label_on << ' ' << (rank + 1) << label_off << '\n';
    out << border_on << "  " << (rank > 0 ? mid_left : bot_left);
    for (int file = 0; file < board::kPlayableSize; ++file) {
      out << horizontal << (file + 1 < board::kPlayableSize ? (rank > 0 ? mid_tee : bot_tee)
                                                            : (rank > 0 ? mid_right : bot_right));
    }
    out << border_off << '\n';
  }

  out << label_on << "    a   b   c   d   e   f   g   h" << label_off;
  return out.str();
}

template <board::PlayableBoard Board>
inline std::string formatBoardStyled(const Board& board) {
  return formatBoard(board, true);
}

inline std::string colorName(int color) { return color == 0 ? "biale" : "czarne"; }

}  // namespace game

#endif
