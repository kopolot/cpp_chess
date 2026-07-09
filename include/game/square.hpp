#ifndef GAME_SQUARE_HPP
#define GAME_SQUARE_HPP

#include <cctype>
#include <optional>
#include <string>
#include <utility>

namespace game {

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

inline std::optional<std::pair<std::pair<int, int>, std::pair<int, int>>> parseMove(
    const std::string& notation) {
  if (notation.size() == 4) {
    const auto from = parseSquare(notation.substr(0, 2));
    const auto to = parseSquare(notation.substr(2, 2));
    if (!from || !to) {
      return std::nullopt;
    }
    return std::make_pair(*from, *to);
  }

  const auto space = notation.find(' ');
  if (space == std::string::npos) {
    return std::nullopt;
  }

  const auto from = parseSquare(notation.substr(0, space));
  const auto to = parseSquare(notation.substr(space + 1));
  if (!from || !to) {
    return std::nullopt;
  }
  return std::make_pair(*from, *to);
}

}  // namespace game

#endif
