#include <fmt/base.h>

#include <iostream>
#include <sstream>
#include <string>

#include "board/board_8x8.hpp"
#include "chess_engine.hpp"
#include "game/game_state.hpp"
#include "game/square.hpp"
#include "game/terminal_style.hpp"

namespace {

std::string styledStatusMessage(const ChessEngine<board::Board8x8>& engine) {
  const auto result = engine.gameResult();
  const int side = engine.currentTurn();
  const std::string side_name = game::colorName(side);
  const bool styled = game::terminal::colorsEnabled();

  switch (result) {
    case game::GameResult::InProgress:
      return "";
    case game::GameResult::Check:
      return styled ? std::string(game::terminal::kCheck) + "♚ Szach! (" + side_name +
                          " pod szachem)" + game::terminal::kReset
                    : "Szach! (" + side_name + " pod szachem)";
    case game::GameResult::Checkmate:
      return styled ? std::string(game::terminal::kMate) + "♔ Mat! Wygrywaja " +
                          game::colorName(1 - side) + '.' + game::terminal::kReset
                    : engine.gameStatusMessage();
    case game::GameResult::Stalemate:
      return styled ? std::string(game::terminal::kDraw) + "♔ Pat! Remis." + game::terminal::kReset
                    : engine.gameStatusMessage();
    case game::GameResult::Draw:
      return styled ? std::string(game::terminal::kDraw) +
                          "½ Remis (50 ruchow lub 3-krotne powtorzenie)." + game::terminal::kReset
                    : engine.gameStatusMessage();
  }
  return "";
}

void printHelp() {
  fmt::print(
      "Komendy:\n"
      "  help            - ta pomoc\n"
      "  board           - wyswietl szachownice\n"
      "  move e2e4       - wykonaj ruch (notacja algebraiczna)\n"
      "  move e7e8q      - ruch z promocja (q/r/b/n)\n"
      "  move e2 e4      - wykonaj ruch (z odstepami)\n"
      "  new             - nowa gra od pozycji startowej\n"
      "  exit            - zakoncz program\n");
}

void printBoard(const ChessEngine<board::Board8x8>& engine) {
  fmt::print("\n{}\n", engine.formatBoardStyled());
  if (engine.isGameOver()) {
    fmt::print("{}\n", styledStatusMessage(engine));
    fmt::print("Gra zakonczona. Wpisz 'new' aby zaczac od nowa.\n");
    return;
  }
  fmt::print("Tura: {}\n", engine.currentPlayerName());
  const auto status = styledStatusMessage(engine);
  if (!status.empty()) {
    fmt::print("{}\n", status);
  }
}

std::optional<PieceType> promptPromotion() {
  fmt::print("Promocja [q/r/b/n]: ");
  std::string input;
  if (!std::getline(std::cin, input) || input.empty()) {
    return std::nullopt;
  }
  return game::parsePromotionChar(input[0]);
}

bool tryMoveWithPromotion(ChessEngine<board::Board8x8>& engine, const game::ParsedMove& move) {
  if (engine.tryMove(move.from.first, move.from.second, move.to.first, move.to.second,
                     move.promotion)) {
    return true;
  }

  if (move.promotion) {
    return false;
  }

  if (!engine.needsPromotionForMove(move.from.first, move.from.second, move.to.first,
                                    move.to.second)) {
    return false;
  }

  const auto promotion = promptPromotion();
  if (!promotion) {
    fmt::print("Niepoprawna figura promocji.\n");
    return false;
  }

  return engine.tryMove(move.from.first, move.from.second, move.to.first, move.to.second,
                        promotion);
}

bool handleMove(ChessEngine<board::Board8x8>& engine, const std::string& args) {
  if (engine.isGameOver()) {
    fmt::print("Gra zakonczona. Wpisz 'new' aby zaczac od nowa.\n");
    return true;
  }
  if (args.empty()) {
    fmt::print("Uzycie: move e2e4  lub  move e2 e4  lub  move e7e8q\n");
    return true;
  }

  const auto parsed = game::parseMoveExtended(args);
  if (parsed && tryMoveWithPromotion(engine, *parsed)) {
    printBoard(engine);
    return true;
  }

  fmt::print("Niepoprawny lub niedozwolony ruch.\n");
  return true;
}

}  // namespace

int main() {
  ChessEngine<board::Board8x8> engine;
  engine.startGame();

  fmt::print("♟ cpp_chess CLI\n");
  printHelp();
  printBoard(engine);

  std::string line;
  while (true) {
    fmt::print("> ");
    if (!std::getline(std::cin, line)) {
      break;
    }

    std::istringstream stream(line);
    std::string command;
    stream >> command;

    if (command.empty()) {
      continue;
    }
    if (command == "exit" || command == "quit") {
      fmt::print("Koniec gry.\n");
      break;
    }
    if (command == "help" || command == "?") {
      printHelp();
      continue;
    }
    if (command == "board" || command == "b") {
      printBoard(engine);
      continue;
    }
    if (command == "new") {
      engine.startGame();
      fmt::print("Nowa gra.\n");
      printBoard(engine);
      continue;
    }
    if (command == "move" || command == "m") {
      std::string args;
      std::getline(stream, args);
      if (!args.empty() && args.front() == ' ') {
        args.erase(0, 1);
      }
      handleMove(engine, args);
      continue;
    }

    const auto parsed = game::parseMoveExtended(command);
    if (parsed && tryMoveWithPromotion(engine, *parsed)) {
      printBoard(engine);
      continue;
    }

    if (engine.isGameOver()) {
      fmt::print("Gra zakonczona. Wpisz 'new' aby zaczac od nowa.\n");
      continue;
    }

    fmt::print("Nieznana komenda: {}. Wpisz help.\n", command);
  }

  return 0;
}
