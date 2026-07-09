#ifndef GAME_CONTEXT_HPP
#define GAME_CONTEXT_HPP

#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "board/board_common.hpp"
#include "board/board_concept.hpp"

namespace game {

struct CastlingRights {
  bool white_kingside = true;
  bool white_queenside = true;
  bool black_kingside = true;
  bool black_queenside = true;
};

struct GameContext {
  CastlingRights castling{};
  std::optional<std::pair<int, int>> en_passant;
  int halfmove_clock = 0;
  std::vector<std::string> position_keys;

  void reset() {
    castling = CastlingRights{};
    en_passant = std::nullopt;
    halfmove_clock = 0;
    position_keys.clear();
  }
};

template <board::PlayableBoard Board>
inline std::string makePositionKey(const Board& board, int side_to_move,
                                   const GameContext& context) {
  std::ostringstream key;
  key << side_to_move << '|';
  key << context.castling.white_kingside << context.castling.white_queenside
      << context.castling.black_kingside << context.castling.black_queenside << '|';
  if (context.en_passant) {
    key << context.en_passant->first << ',' << context.en_passant->second;
  }
  key << '|';
  for (int rank = 0; rank < board::kPlayableSize; ++rank) {
    for (int file = 0; file < board::kPlayableSize; ++file) {
      const auto occupant = board.get(file, rank);
      if (!occupant) {
        key << '.';
      } else {
        key << occupant->color << static_cast<int>(occupant->type);
      }
    }
  }
  return key.str();
}

inline int countPositionRepetitions(const std::vector<std::string>& keys,
                                    const std::string& key) {
  int count = 0;
  for (const auto& stored : keys) {
    if (stored == key) {
      ++count;
    }
  }
  return count;
}

inline bool kingsideCastle(int from_file, int to_file) { return to_file > from_file; }

}  // namespace game

#endif
