#include <gtest/gtest.h>

#include <random>

#include "ai/evaluate.hpp"
#include "ai/search.hpp"
#include "board/board_8x8.hpp"
#include "chess_engine.hpp"
#include "game/move_list.hpp"
#include "game/setup.hpp"

TEST(MoveListTest, StartingPositionHasTwentyMoves) {
  board::Board8x8 board;
  game::GameContext context;
  game::setupInitialPosition(board);
  const auto moves = game::collectLegalMoves(board, context, 0);
  EXPECT_EQ(moves.size(), 20u);
}

TEST(AiEvalTest, MaterialIsSymmetricAtStart) {
  board::Board8x8 board;
  game::setupInitialPosition(board);
  EXPECT_EQ(ai::evaluateMaterial(board), 0);
}

TEST(AiSearchTest, ChoosesLegalMove) {
  ChessEngine<board::Board8x8> engine;
  engine.startGame();

  std::mt19937 rng(42);
  const auto move = ai::chooseMove(engine.getBoard(), engine.getContext(), 0,
                                   ai::Difficulty::Easy, rng);
  ASSERT_TRUE(move.has_value());
  EXPECT_TRUE(engine.tryMove(move->from_file, move->from_rank, move->to_file,
                             move->to_rank, move->promotion));
}

TEST(AiSearchTest, MediumReplyAfterE4IsLegal) {
  ChessEngine<board::Board8x8> engine;
  engine.startGame();
  ASSERT_TRUE(engine.tryMoveNotation("e2e4"));

  std::mt19937 rng(7);
  const auto move = ai::chooseMove(engine.getBoard(), engine.getContext(), 1,
                                   ai::Difficulty::Medium, rng);
  ASSERT_TRUE(move.has_value());
  EXPECT_TRUE(engine.tryMove(move->from_file, move->from_rank, move->to_file,
                             move->to_rank, move->promotion));
}

TEST(AiSearchTest, ParseDifficultyNames) {
  EXPECT_EQ(ai::parseDifficulty("easy"), ai::Difficulty::Easy);
  EXPECT_EQ(ai::parseDifficulty("2"), ai::Difficulty::Medium);
  EXPECT_EQ(ai::parseDifficulty("hard"), ai::Difficulty::Hard);
  EXPECT_FALSE(ai::parseDifficulty("insanity").has_value());
}
