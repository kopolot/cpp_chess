#include <fmt/base.h>

#include <iostream>
#include <sstream>
#include <string>

#include "board/board_8x8.hpp"
#include "chess_engine.hpp"

namespace {

void printHelp() {
  fmt::print(
      "Komendy:\n"
      "  help          - ta pomoc\n"
      "  board         - wyswietl plansze\n"
      "  move e2e4     - wykonaj ruch (notacja algebraiczna)\n"
      "  move e2 e4    - wykonaj ruch (z odstepami)\n"
      "  new           - nowa gra od pozycji startowej\n"
      "  exit          - zakoncz program\n");
}

void printBoard(const ChessEngine<board::Board8x8>& engine) {
  fmt::print("\n{}\n", engine.formatBoard());
  if (engine.isGameOver()) {
    fmt::print("{}\n", engine.gameStatusMessage());
    fmt::print("Gra zakonczona. Wpisz 'new' aby zaczac od nowa.\n");
    return;
  }
  fmt::print("Tura: {}\n", engine.currentPlayerName());
  const auto status = engine.gameStatusMessage();
  if (!status.empty()) {
    fmt::print("{}\n", status);
  }
}

bool handleMove(ChessEngine<board::Board8x8>& engine, const std::string& args) {
  if (engine.isGameOver()) {
    fmt::print("Gra zakonczona. Wpisz 'new' aby zaczac od nowa.\n");
    return true;
  }
  if (args.empty()) {
    fmt::print("Uzycie: move e2e4  lub  move e2 e4\n");
    return true;
  }

  if (engine.tryMoveNotation(args)) {
    printBoard(engine);
    return true;
  }

  std::istringstream stream(args);
  std::string from;
  std::string to;
  stream >> from >> to;
  if (!from.empty() && !to.empty() && engine.tryMove(from, to)) {
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

  fmt::print("cpp_chess CLI (MVP)\n");
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

    if (engine.tryMoveNotation(command)) {
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
