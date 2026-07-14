#ifndef GAME_SQUARE_HPP
#define GAME_SQUARE_HPP

#include <cctype>
#include <optional>
#include <string>
#include <utility>

#include "type/chess_piece.hpp"

namespace game {

struct ParsedMove {
  std::pair<int, int> from;
  std::pair<int, int> to;
  std::optional<PieceType> promotion;
};

inline std::optional<std::pair<int, int>> parseSquare(const std::string& notation) {
  if (notation.size() != 2) {
    return std::nullopt;
  }

  const char file_char = static_cast<char>(std::tolower(notation[0]));
  const char rank_char = notation[1];

  if (file_char < 'a' || file_char > 'h' || rank_char < '1' || rank_char > '8') {
    return std::nullopt;
  }

  return std::make_pair(file_char - 'a', rank_char - '1');
}

inline std::string toNotation(int file, int rank) {
  return std::string(1, static_cast<char>('a' + file)) +
         std::string(1, static_cast<char>('1' + rank));
}

inline std::optional<PieceType> parsePromotionChar(char symbol) {
  switch (static_cast<char>(std::tolower(symbol))) {
    case 'q':
      return PieceType::Queen;
    case 'r':
      return PieceType::Rook;
    case 'b':
      return PieceType::Bishop;
    case 'n':
      return PieceType::Knight;
    default:
      return std::nullopt;
  }
}

inline std::optional<ParsedMove> parseMoveExtended(const std::string& notation) {
  const auto space = notation.find(' ');
  if (space != std::string::npos) {
    const auto from = parseSquare(notation.substr(0, space));
    std::string rest = notation.substr(space + 1);
    while (!rest.empty() && rest.front() == ' ') {
      rest.erase(0, 1);
    }

    const auto second_space = rest.find(' ');
    std::string to_notation = rest;
    std::optional<PieceType> promotion;

    if (second_space != std::string::npos) {
      to_notation = rest.substr(0, second_space);
      std::string promo_str = rest.substr(second_space + 1);
      while (!promo_str.empty() && promo_str.front() == ' ') {
        promo_str.erase(0, 1);
      }
      if (!promo_str.empty()) {
        promotion = parsePromotionChar(promo_str[0]);
        if (!promotion) {
          return std::nullopt;
        }
      }
    } else if (rest.size() == 3) {
      to_notation = rest.substr(0, 2);
      promotion = parsePromotionChar(rest[2]);
      if (!promotion) {
        return std::nullopt;
      }
    }

    const auto to = parseSquare(to_notation);
    if (!from || !to) {
      return std::nullopt;
    }
    return ParsedMove{*from, *to, promotion};
  }

  if (notation.size() == 4 || notation.size() == 5) {
    const auto from = parseSquare(notation.substr(0, 2));
    const auto to = parseSquare(notation.substr(2, 2));
    if (!from || !to) {
      return std::nullopt;
    }
    ParsedMove move{*from, *to, std::nullopt};
    if (notation.size() == 5) {
      move.promotion = parsePromotionChar(notation[4]);
      if (!move.promotion) {
        return std::nullopt;
      }
    }
    return move;
  }

  return std::nullopt;
}

inline std::optional<std::pair<std::pair<int, int>, std::pair<int, int>>> parseMove(
    const std::string& notation) {
  const auto parsed = parseMoveExtended(notation);
  if (!parsed) {
    return std::nullopt;
  }
  return std::make_pair(parsed->from, parsed->to);
}

inline bool needsPromotion(int from_rank, int to_rank, PieceType type, int color) {
  if (type != PieceType::Pawn) {
    return false;
  }
  const int promotion_rank = (color == 0) ? 7 : 0;
  return to_rank == promotion_rank;
}

}  // namespace game

#endif
