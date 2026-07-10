#include <gtest/gtest.h>

#include "board/board_12x12.hpp"
#include "board/board_8x8.hpp"
#include "chess_engine.hpp"
#include "game/attack.hpp"
#include "game/game_context.hpp"
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
  game::GameContext context;
  this->board.set(4, 1, kWhitePawn);
  EXPECT_TRUE(game::isPseudoLegalMove(this->board, 4, 1, 4, 2, kWhitePawn, context));
  EXPECT_TRUE(game::isPseudoLegalMove(this->board, 4, 1, 4, 3, kWhitePawn, context));
  EXPECT_FALSE(game::isPseudoLegalMove(this->board, 4, 1, 4, 4, kWhitePawn, context));
}

TYPED_TEST(MoveValidationTest, PawnCapture) {
  game::GameContext context;
  this->board.set(4, 4, kWhitePawn);
  this->board.set(5, 5, kBlackPawn);
  EXPECT_TRUE(game::isPseudoLegalMove(this->board, 4, 4, 5, 5, kWhitePawn, context));
  EXPECT_FALSE(game::isPseudoLegalMove(this->board, 4, 4, 3, 5, kWhitePawn, context));
}

TYPED_TEST(MoveValidationTest, KnightJump) {
  game::GameContext context;
  this->board.set(1, 0, kWhiteKnight);
  EXPECT_TRUE(game::isPseudoLegalMove(this->board, 1, 0, 2, 2, kWhiteKnight, context));
  EXPECT_FALSE(game::isPseudoLegalMove(this->board, 1, 0, 2, 3, kWhiteKnight, context));
}

TYPED_TEST(MoveValidationTest, BishopRequiresClearPath) {
  game::GameContext context;
  this->board.set(2, 0, kWhiteBishop);
  this->board.set(4, 2, kBlackPawn);
  EXPECT_FALSE(game::isPseudoLegalMove(this->board, 2, 0, 6, 4, kWhiteBishop, context));
  this->board.set(3, 1, std::nullopt);
  EXPECT_TRUE(game::isPseudoLegalMove(this->board, 2, 0, 4, 2, kWhiteBishop, context));
}

TYPED_TEST(MoveValidationTest, RookBlockedByPiece) {
  game::GameContext context;
  this->board.set(0, 0, kWhiteRook);
  this->board.set(0, 3, kWhitePawn);
  EXPECT_FALSE(game::isPseudoLegalMove(this->board, 0, 0, 0, 7, kWhiteRook, context));
}

TYPED_TEST(MoveValidationTest, QueenMovesLikeRookOrBishop) {
  game::GameContext context;
  this->board.set(3, 3, kWhiteQueen);
  EXPECT_TRUE(game::isPseudoLegalMove(this->board, 3, 3, 3, 7, kWhiteQueen, context));
  EXPECT_TRUE(game::isPseudoLegalMove(this->board, 3, 3, 7, 7, kWhiteQueen, context));
}

TYPED_TEST(MoveValidationTest, KingMovesOneSquare) {
  game::GameContext context;
  this->board.set(4, 0, kWhiteKing);
  EXPECT_TRUE(game::isPseudoLegalMove(this->board, 4, 0, 5, 1, kWhiteKing, context));
  EXPECT_FALSE(game::isPseudoLegalMove(this->board, 4, 0, 6, 0, kWhiteKing, context));
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
  EXPECT_TRUE(engine.tryMoveNotation("a7a8q"));
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
  game::GameContext context;
  board.set(4, 0, kWhiteKing);
  board.set(4, 7, kBlackKing);
  board.set(4, 6, kBlackQueen);
  board.set(4, 1, kWhiteBishop);
  EXPECT_FALSE(game::isLegalMove(board, context, 4, 1, 3, 0, kWhiteBishop));
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
  game::GameContext context;
  engine.getBoard().clear();
  engine.getBoard().set(0, 7, kBlackKing);
  engine.getBoard().set(2, 5, kWhiteKing);
  engine.getBoard().set(1, 5, kWhiteQueen);

  EXPECT_EQ(game::evaluatePosition(engine.getBoard(), context, 1), game::GameResult::Stalemate);
  EXPECT_FALSE(game::hasAnyLegalMove(engine.getBoard(), context, 1));
}

TEST(GameStateTest, WhiteCastlingKingside) {
  ChessEngine<board::Board8x8> engine;
  engine.startGame();
  EXPECT_TRUE(engine.tryMoveNotation("e2e4"));
  EXPECT_TRUE(engine.tryMoveNotation("a7a6"));
  EXPECT_TRUE(engine.tryMoveNotation("f1c4"));
  EXPECT_TRUE(engine.tryMoveNotation("a6a5"));
  EXPECT_TRUE(engine.tryMoveNotation("g1f3"));
  EXPECT_TRUE(engine.tryMoveNotation("a5a4"));
  EXPECT_TRUE(engine.tryMoveNotation("e1g1"));
  EXPECT_EQ(engine.getBoard().get(6, 0)->type, PieceType::King);
  EXPECT_EQ(engine.getBoard().get(5, 0)->type, PieceType::Rook);
}

TEST(GameStateTest, EnPassantCapture) {
  ChessEngine<board::Board8x8> engine;
  engine.startGame();
  EXPECT_TRUE(engine.tryMoveNotation("e2e4"));
  EXPECT_TRUE(engine.tryMoveNotation("a7a6"));
  EXPECT_TRUE(engine.tryMoveNotation("e4e5"));
  EXPECT_TRUE(engine.tryMoveNotation("d7d5"));
  EXPECT_TRUE(engine.tryMoveNotation("e5d6"));
  EXPECT_TRUE(engine.getBoard().isEmpty(3, 4));
  const auto piece = engine.getBoard().get(3, 5);
  ASSERT_TRUE(piece.has_value());
  EXPECT_EQ(piece->type, PieceType::Pawn);
  EXPECT_EQ(piece->color, 0);
}

TEST(GameStateTest, PromotionToKnight) {
  ChessEngine<board::Board8x8> engine;
  engine.getBoard().clear();
  engine.getBoard().set(0, 6, kWhitePawn);
  engine.getBoard().set(1, 7, kBlackKing);
  engine.getBoard().set(7, 0, kWhiteKing);
  EXPECT_TRUE(engine.tryMoveNotation("a7a8n"));
  const auto piece = engine.getBoard().get(0, 7);
  ASSERT_TRUE(piece.has_value());
  EXPECT_EQ(piece->type, PieceType::Knight);
}

TEST(GameStateTest, FiftyMoveDraw) {
  board::Board8x8 board;
  game::GameContext context;
  board.set(4, 0, kWhiteKing);
  board.set(4, 7, kBlackKing);
  context.halfmove_clock = 100;
  EXPECT_EQ(game::evaluatePosition(board, context, 0), game::GameResult::Draw);
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
