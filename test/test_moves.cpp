#include <gtest/gtest.h>

#include "board/board_12x12.hpp"
#include "board/board_8x8.hpp"
#include "chess_engine.hpp"
#include "game/game_state.hpp"
#include "game/move_validator.hpp"
#include "game/setup.hpp"

namespace {

constexpr board::Occupant kWhitePawn{0, PieceType::Pawn};
constexpr board::Occupant kBlackPawn{1, PieceType::Pawn};
constexpr board::Occupant kWhiteKnight{0, PieceType::Knight};
constexpr board::Occupant kWhiteBishop{0, PieceType::Bishop};
constexpr board::Occupant kWhiteRook{0, PieceType::Rook};
constexpr board::Occupant kWhiteQueen{0, PieceType::Queen};
constexpr board::Occupant kWhiteKing{0, PieceType::King};
constexpr board::Occupant kBlackKing{1, PieceType::King};
constexpr board::Occupant kBlackQueen{1, PieceType::Queen};

template <typename BoardType>
class MoveValidationTest : public ::testing::Test {
 protected:
  BoardType board;
};

using BoardTypes = ::testing::Types<board::Board8x8, board::Board12x12>;
TYPED_TEST_SUITE(MoveValidationTest, BoardTypes);

TYPED_TEST(MoveValidationTest, PawnSingleAndDoublePush) {
  this->board.set(4, 1, kWhitePawn);
  EXPECT_TRUE(game::isPseudoLegalMove(this->board, 4, 1, 4, 2, kWhitePawn));
  EXPECT_TRUE(game::isPseudoLegalMove(this->board, 4, 1, 4, 3, kWhitePawn));
  EXPECT_FALSE(game::isPseudoLegalMove(this->board, 4, 1, 4, 4, kWhitePawn));
}

TYPED_TEST(MoveValidationTest, PawnCapture) {
  this->board.set(4, 4, kWhitePawn);
  this->board.set(5, 5, kBlackPawn);
  EXPECT_TRUE(game::isPseudoLegalMove(this->board, 4, 4, 5, 5, kWhitePawn));
  EXPECT_FALSE(game::isPseudoLegalMove(this->board, 4, 4, 3, 5, kWhitePawn));
}

TYPED_TEST(MoveValidationTest, KnightJump) {
  this->board.set(1, 0, kWhiteKnight);
  EXPECT_TRUE(game::isPseudoLegalMove(this->board, 1, 0, 2, 2, kWhiteKnight));
  EXPECT_FALSE(game::isPseudoLegalMove(this->board, 1, 0, 2, 3, kWhiteKnight));
}

TYPED_TEST(MoveValidationTest, BishopRequiresClearPath) {
  this->board.set(2, 0, kWhiteBishop);
  this->board.set(4, 2, kBlackPawn);
  EXPECT_FALSE(game::isPseudoLegalMove(this->board, 2, 0, 6, 4, kWhiteBishop));
  this->board.set(3, 1, std::nullopt);
  EXPECT_TRUE(game::isPseudoLegalMove(this->board, 2, 0, 4, 2, kWhiteBishop));
}

TYPED_TEST(MoveValidationTest, RookBlockedByPiece) {
  this->board.set(0, 0, kWhiteRook);
  this->board.set(0, 3, kWhitePawn);
  EXPECT_FALSE(game::isPseudoLegalMove(this->board, 0, 0, 0, 7, kWhiteRook));
}

TYPED_TEST(MoveValidationTest, QueenMovesLikeRookOrBishop) {
  this->board.set(3, 3, kWhiteQueen);
  EXPECT_TRUE(game::isPseudoLegalMove(this->board, 3, 3, 3, 7, kWhiteQueen));
  EXPECT_TRUE(game::isPseudoLegalMove(this->board, 3, 3, 7, 7, kWhiteQueen));
}

TYPED_TEST(MoveValidationTest, KingMovesOneSquare) {
  this->board.set(4, 0, kWhiteKing);
  EXPECT_TRUE(game::isPseudoLegalMove(this->board, 4, 0, 5, 1, kWhiteKing));
  EXPECT_FALSE(game::isPseudoLegalMove(this->board, 4, 0, 6, 0, kWhiteKing));
}

TEST(ChessEngineMoveTest, OpeningMoveSwitchesTurn) {
  ChessEngine<board::Board8x8> engine;
  engine.startGame();
  EXPECT_EQ(engine.currentTurn(), 0);
  EXPECT_TRUE(engine.tryMoveNotation("e2e4"));
  EXPECT_EQ(engine.currentTurn(), 1);
  EXPECT_FALSE(engine.getBoard().isEmpty(4, 3));
}

TEST(ChessEngineMoveTest, RejectsIllegalMove) {
  ChessEngine<board::Board8x8> engine;
  engine.startGame();
  EXPECT_FALSE(engine.tryMoveNotation("e2e5"));
  EXPECT_EQ(engine.currentTurn(), 0);
}

TEST(ChessEngineMoveTest, PawnPromotesToQueen) {
  ChessEngine<board::Board8x8> engine;
  engine.startGame();
  engine.getBoard().clear();
  engine.getBoard().set(0, 6, kWhitePawn);
  EXPECT_TRUE(engine.tryMove("a7", "a8"));
  const auto piece = engine.getBoard().get(0, 7);
  ASSERT_TRUE(piece.has_value());
  EXPECT_EQ(piece->type, PieceType::Queen);
}

TEST(ChessEngineMoveTest, WorksOnBoard12x12) {
  ChessEngine<board::Board12x12> engine;
  engine.startGame();
  EXPECT_TRUE(engine.tryMoveNotation("g1f3"));
  EXPECT_EQ(engine.currentTurn(), 1);
}

TEST(GameStateTest, DetectsCheck) {
  board::Board8x8 board;
  board.set(4, 0, kWhiteKing);
  board.set(4, 7, kBlackKing);
  board.set(4, 6, kWhiteQueen);
  EXPECT_TRUE(game::isInCheck(board, 1));
  EXPECT_FALSE(game::isInCheck(board, 0));
}

TEST(GameStateTest, RejectsMoveThatLeavesKingInCheck) {
  board::Board8x8 board;
  board.set(4, 0, kWhiteKing);
  board.set(4, 7, kBlackKing);
  board.set(4, 6, kBlackQueen);
  board.set(4, 1, kWhiteBishop);
  EXPECT_FALSE(game::isLegalMove(board, 4, 1, 3, 0, kWhiteBishop));
}

TEST(GameStateTest, ScholarMate) {
  ChessEngine<board::Board8x8> engine;
  engine.startGame();
  EXPECT_TRUE(engine.tryMoveNotation("e2e4"));
  EXPECT_TRUE(engine.tryMoveNotation("e7e5"));
  EXPECT_TRUE(engine.tryMoveNotation("f1c4"));
  EXPECT_TRUE(engine.tryMoveNotation("b8c6"));
  EXPECT_TRUE(engine.tryMoveNotation("d1h5"));
  EXPECT_TRUE(engine.tryMoveNotation("g8f6"));
  EXPECT_TRUE(engine.tryMoveNotation("h5f7"));
  EXPECT_TRUE(engine.isGameOver());
  EXPECT_EQ(engine.gameResult(), game::GameResult::Checkmate);
}

TEST(GameStateTest, Stalemate) {
  ChessEngine<board::Board8x8> engine;
  engine.getBoard().clear();
  engine.getBoard().set(0, 7, kBlackKing);
  engine.getBoard().set(2, 5, kWhiteKing);
  engine.getBoard().set(1, 5, kWhiteQueen);

  EXPECT_EQ(game::evaluatePosition(engine.getBoard(), 1), game::GameResult::Stalemate);
  EXPECT_FALSE(game::hasAnyLegalMove(engine.getBoard(), 1));
}

TEST(GameStateTest, BlocksMovesAfterCheckmate) {
  ChessEngine<board::Board8x8> engine;
  engine.startGame();
  EXPECT_TRUE(engine.tryMoveNotation("f2f3"));
  EXPECT_TRUE(engine.tryMoveNotation("e7e5"));
  EXPECT_TRUE(engine.tryMoveNotation("g2g4"));
  EXPECT_TRUE(engine.tryMoveNotation("d8h4"));
  EXPECT_TRUE(engine.isGameOver());
  EXPECT_FALSE(engine.tryMoveNotation("e2e4"));
}

}  // namespace
