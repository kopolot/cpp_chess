#ifndef AI_SEARCH_HPP
#define AI_SEARCH_HPP

#include <algorithm>
#include <limits>
#include <optional>
#include <random>
#include <string_view>
#include <vector>

#include "ai/evaluate.hpp"
#include "board/board_concept.hpp"
#include "game/attack.hpp"
#include "game/game_context.hpp"
#include "game/game_state.hpp"
#include "game/move_list.hpp"

namespace ai {

enum class Difficulty { Easy = 1, Medium = 2, Hard = 3 };

inline int searchDepth(Difficulty difficulty) {
  switch (difficulty) {
    case Difficulty::Easy:
      return 1;
    case Difficulty::Medium:
      return 2;
    case Difficulty::Hard:
      return 3;
  }
  return 1;
}

inline const char* difficultyName(Difficulty difficulty) {
  switch (difficulty) {
    case Difficulty::Easy:
      return "easy";
    case Difficulty::Medium:
      return "medium";
    case Difficulty::Hard:
      return "hard";
  }
  return "easy";
}

inline std::optional<Difficulty> parseDifficulty(int value) {
  if (value == 1) {
    return Difficulty::Easy;
  }
  if (value == 2) {
    return Difficulty::Medium;
  }
  if (value == 3) {
    return Difficulty::Hard;
  }
  return std::nullopt;
}

inline std::optional<Difficulty> parseDifficulty(std::string_view name) {
  if (name == "1" || name == "easy" || name == "latwy" || name == "łatwy") {
    return Difficulty::Easy;
  }
  if (name == "2" || name == "medium" || name == "sredni" || name == "średni") {
    return Difficulty::Medium;
  }
  if (name == "3" || name == "hard" || name == "trudny") {
    return Difficulty::Hard;
  }
  return std::nullopt;
}

inline int scoreFromSide(int white_score, int side) {
  return side == 0 ? white_score : -white_score;
}

template <board::PlayableBoard Board>
inline int alphabeta(Board& board, game::GameContext& context, int side,
                     int depth, int alpha, int beta) {
  if (depth == 0) {
    return scoreFromSide(evaluatePosition(board, side), side);
  }

  const auto moves = game::collectLegalMoves(board, context, side);
  if (moves.empty()) {
    if (game::isInCheck(board, side)) {
      return -kMateScore;
    }
    return kStalemateScore;
  }

  int best = std::numeric_limits<int>::min() / 2;
  for (const auto& move : moves) {
    const auto piece = board.get(move.from_file, move.from_rank);
    if (!piece) {
      continue;
    }

    const game::GameContext snapshot = context;
    const auto delta =
        game::applyMove(board, move.from_file, move.from_rank, move.to_file,
                        move.to_rank, *piece, context, move.promotion);
    const int score =
        -alphabeta(board, context, 1 - side, depth - 1, -beta, -alpha);
    game::undoMove(board, move.from_file, move.from_rank, move.to_file,
                   move.to_rank, *piece, delta);
    context = snapshot;

    best = std::max(best, score);
    alpha = std::max(alpha, score);
    if (alpha >= beta) {
      break;
    }
  }
  return best;
}

struct ScoredMove {
  game::Move move;
  int score = 0;
};

template <board::PlayableBoard Board>
inline std::vector<ScoredMove> scoreRootMoves(Board board,
                                              game::GameContext context,
                                              int side, int depth) {
  std::vector<ScoredMove> scored;
  const auto moves = game::collectLegalMoves(board, context, side);
  scored.reserve(moves.size());

  for (const auto& move : moves) {
    const auto piece = board.get(move.from_file, move.from_rank);
    if (!piece) {
      continue;
    }

    const game::GameContext snapshot = context;
    const auto delta =
        game::applyMove(board, move.from_file, move.from_rank, move.to_file,
                        move.to_rank, *piece, context, move.promotion);
    const int score = -alphabeta(board, context, 1 - side, depth - 1,
                                 std::numeric_limits<int>::min() / 2,
                                 std::numeric_limits<int>::max() / 2);
    game::undoMove(board, move.from_file, move.from_rank, move.to_file,
                   move.to_rank, *piece, delta);
    context = snapshot;
    scored.push_back(ScoredMove{move, score});
  }

  std::stable_sort(scored.begin(), scored.end(),
                   [](const ScoredMove& a, const ScoredMove& b) {
                     return a.score > b.score;
                   });
  return scored;
}

template <board::PlayableBoard Board>
inline std::optional<game::Move> chooseMove(const Board& board,
                                            const game::GameContext& context,
                                            int side, Difficulty difficulty,
                                            std::mt19937& rng) {
  const int depth = searchDepth(difficulty);
  auto scored = scoreRootMoves(board, context, side, depth);
  if (scored.empty()) {
    return std::nullopt;
  }

  if (difficulty == Difficulty::Easy) {
    const int best = scored.front().score;
    std::vector<game::Move> candidates;
    for (const auto& entry : scored) {
      if (entry.score >= best - 80) {
        candidates.push_back(entry.move);
      }
      if (candidates.size() >= 8) {
        break;
      }
    }
    if (candidates.empty()) {
      return scored.front().move;
    }
    std::uniform_int_distribution<std::size_t> dist(0, candidates.size() - 1);
    return candidates[dist(rng)];
  }

  if (difficulty == Difficulty::Medium && scored.size() > 1) {
    std::uniform_int_distribution<int> coin(0, 4);
    if (coin(rng) == 0 && scored[1].score >= scored[0].score - 40) {
      return scored[1].move;
    }
  }

  return scored.front().move;
}

template <board::PlayableBoard Board>
inline std::optional<game::Move> chooseMove(const Board& board,
                                            const game::GameContext& context,
                                            int side, Difficulty difficulty) {
  thread_local std::mt19937 rng{std::random_device{}()};
  return chooseMove(board, context, side, difficulty, rng);
}

}  // namespace ai

#endif
