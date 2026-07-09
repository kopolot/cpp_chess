#include <gtest/gtest.h>

#include "board/board_12x12.hpp"
#include "board/board_8x8.hpp"
#include "board/board_concept.hpp"
#include "chess_engine.hpp"

namespace {

constexpr board::Occupant kWhiteRook{0, PieceType::Rook};
constexpr board::Occupant kBlackPawn{1, PieceType::Pawn};

template <typename BoardType>
void testSharedPlayableApi(BoardType& board) {
  EXPECT_TRUE(board.inPlayableBounds(0, 0));
  EXPECT_TRUE(board.inPlayableBounds(7, 7));
  EXPECT_FALSE(board.inPlayableBounds(-1, 0));
  EXPECT_FALSE(board.inPlayableBounds(8, 0));

  EXPECT_TRUE(board.isEmpty(3, 3));
  EXPECT_TRUE(board.set(3, 3, kWhiteRook));
  EXPECT_FALSE(board.isEmpty(3, 3));
  ASSERT_TRUE(board.get(3, 3).has_value());
  EXPECT_EQ(board.get(3, 3)->type, PieceType::Rook);
  EXPECT_EQ(board.get(3, 3)->color, 0);

  EXPECT_FALSE(board.set(9, 9, kWhiteRook));

  board.clear();
  EXPECT_TRUE(board.isEmpty(3, 3));
}

}  // namespace

static_assert(board::PlayableBoard<board::Board8x8>);
static_assert(board::PlayableBoard<board::Board12x12>);

TEST(Board8x8Test, BorderCountingBlocksOutOfRange) {
  board::Board8x8 board;
  EXPECT_FALSE(board.set(-1, 0, kWhiteRook));
  EXPECT_FALSE(board.set(0, 8, kWhiteRook));
  EXPECT_TRUE(board.isBlocked(-1, 0));
  EXPECT_TRUE(board.isBlocked(8, 4));
  EXPECT_FALSE(board.isBlocked(4, 4));
}

TEST(Board8x8Test, IdentityCoordinateMapping) {
  board::Board8x8 board;
  EXPECT_EQ(board.toInternal(4, 5), std::make_pair(4, 5));
  EXPECT_EQ(board.toPlayable(4, 5), std::make_pair(4, 5));
}

TEST(Board8x8Test, SharedApi) {
  board::Board8x8 board;
  testSharedPlayableApi(board);
}

TEST(Board12x12Test, InternalOffsetMapping) {
  board::Board12x12 board;
  EXPECT_EQ(board.toInternal(0, 0), std::make_pair(2, 2));
  EXPECT_EQ(board.toInternal(7, 7), std::make_pair(9, 9));
  EXPECT_EQ(board.toPlayable(5, 6), std::make_pair(3, 4));
}

TEST(Board12x12Test, BorderDetectedByCollision) {
  board::Board12x12 board;
  EXPECT_TRUE(board.isBorderCell(0, 0));
  EXPECT_TRUE(board.isBorderCell(1, 5));
  EXPECT_TRUE(board.isBorderCell(5, 1));
  EXPECT_TRUE(board.isBorderCell(11, 11));
  EXPECT_FALSE(board.isBorderCell(5, 5));

  EXPECT_TRUE(board.isBlockedInternal(0, 0));
  EXPECT_TRUE(board.isBlockedInternal(1, 7));
  EXPECT_FALSE(board.isBlockedInternal(5, 5));
}

TEST(Board12x12Test, StepUntilBorderCollision) {
  board::Board12x12 board;
  const auto [stop_x, stop_y] = board.stepUntilBlocked(5, 5, 1, 0);
  EXPECT_EQ(stop_x, 9);
  EXPECT_EQ(stop_y, 5);
  EXPECT_TRUE(board.isBorderCell(stop_x + 1, stop_y));
}

TEST(Board12x12Test, StepUntilPieceCollision) {
  board::Board12x12 board;
  board.set(4, 3, kBlackPawn);  // internal (6, 5) — ten sam rząd co start (5, 5)
  const auto [stop_x, stop_y] = board.stepUntilBlocked(5, 5, 1, 0);
  EXPECT_EQ(stop_x, 5);
  EXPECT_EQ(stop_y, 5);
}

TEST(Board12x12Test, SharedApi) {
  board::Board12x12 board;
  testSharedPlayableApi(board);
}

TEST(ChessEngineTest, WorksWithBoard8x8) {
  ChessEngine<board::Board8x8> engine;
  engine.startGame();
  EXPECT_FALSE(engine.getBoard().isEmpty(0, 0));
  EXPECT_FALSE(engine.getBoard().isEmpty(4, 1));
}

TEST(ChessEngineTest, WorksWithBoard12x12) {
  ChessEngine<board::Board12x12> engine;
  engine.startGame();
  EXPECT_FALSE(engine.getBoard().isEmpty(0, 0));
  EXPECT_FALSE(engine.getBoard().isEmpty(4, 6));
}
