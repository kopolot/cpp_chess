#include <fmt/base.h>

#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

#include "board/board_12x12.hpp"
#include "board/board_8x8.hpp"
#include "chess_engine.hpp"
#include "game/en_passant.hpp"
#include "game/game_state.hpp"
#include "game/square.hpp"
#include "game/terminal_style.hpp"

namespace {

enum class BoardChoice { k8x8, k12x12 };

std::optional<BoardChoice> parseBoardValue(std::string_view value) {
  if (value == "8x8" || value == "8") {
    return BoardChoice::k8x8;
  }
  if (value == "12x12" || value == "12") {
    return BoardChoice::k12x12;
  }
  return std::nullopt;
}

const char* boardLabel(BoardChoice choice) {
  return choice == BoardChoice::k12x12 ? "12x12 (obramowanie)" : "8x8";
}

void printUsage(const char* program) {
  fmt::print(
      "Uzycie: {} [opcje]\n"
      "\n"
      "Opcje:\n"
      "  --board <8x8|12x12>   implementacja planszy (domyslnie 8x8)\n"
      "  -b <8x8|12x12>        skrot --board\n"
      "  --help, -h            ta pomoc\n"
      "\n",
      program);
}

enum class ArgParseResult { kOk, kHelp, kError };

ArgParseResult parseArgs(int argc, char* argv[], BoardChoice& choice) {
  choice = BoardChoice::k8x8;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      printUsage(argv[0]);
      return ArgParseResult::kHelp;
    }
    if (arg == "--board" || arg == "-b") {
      if (i + 1 >= argc) {
        fmt::print("Brak wartosci po {}.\n", arg);
        printUsage(argv[0]);
        return ArgParseResult::kError;
      }
      const auto parsed = parseBoardValue(argv[++i]);
      if (!parsed) {
        fmt::print("Nieznana plansza: {}. Dostepne: 8x8, 12x12.\n", argv[i]);
        return ArgParseResult::kError;
      }
      choice = *parsed;
      continue;
    }
    if (arg.rfind("--board=", 0) == 0) {
      const auto parsed = parseBoardValue(arg.substr(8));
      if (!parsed) {
        fmt::print("Nieznana plansza: {}. Dostepne: 8x8, 12x12.\n", arg.substr(8));
        return ArgParseResult::kError;
      }
      choice = *parsed;
      continue;
    }

    fmt::print("Nieznany argument: {}.\n", arg);
    printUsage(argv[0]);
    return ArgParseResult::kError;
  }

  return ArgParseResult::kOk;
}

template <typename BoardType>
std::string styledStatusMessage(const ChessEngine<BoardType>& engine) {
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
      "  exit            - zakoncz program\n"
      "\n"
      "Bicie na przelocie:\n"
      "  Po ruchu piona o 2 pola mozesz zbic go pionem z sasiedniej kolumny.\n"
      "  Wpisz ruch na pole za bitym pionem (np. e5d6) lub na pole piona (e5d5).\n"
      "  Dostepne tylko przez jeden ruch — CLI pokaze podpowiedz.\n");
}

template <typename BoardType>
void printBoard(const ChessEngine<BoardType>& engine) {
  fmt::print("\n{}\n", engine.formatBoardStyled());
  if (engine.isGameOver()) {
    fmt::print("{}\n", styledStatusMessage(engine));
    fmt::print("Gra zakonczona. Wpisz 'new' aby zaczac od nowa.\n");
    return;
  }
  fmt::print("Tura: {}\n", engine.currentPlayerName());
  if (const auto hint = engine.enPassantHint()) {
    fmt::print("{}\n", *hint);
  }
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

template <typename BoardType>
bool tryMoveWithPromotion(ChessEngine<BoardType>& engine, const game::ParsedMove& move) {
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

template <typename BoardType>
bool handleMove(ChessEngine<BoardType>& engine, const std::string& args) {
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

  if (parsed && engine.getContext().en_passant) {
    fmt::print(
        "Niepoprawny ruch. Bicie na przelocie? Wpisz np. {} (pole za bitym pionem).\n",
        game::toNotation(parsed->from.first, parsed->from.second) +
            game::toNotation(engine.getContext().en_passant->first,
                             engine.getContext().en_passant->second));
  } else {
    fmt::print("Niepoprawny lub niedozwolony ruch.\n");
  }
  return true;
}

template <typename BoardType>
void runCli(BoardChoice board_choice) {
  ChessEngine<BoardType> engine;
  engine.startGame();

  fmt::print("♟ cpp_chess CLI (plansza: {})\n", boardLabel(board_choice));
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
}

}  // namespace

int main(int argc, char* argv[]) {
  BoardChoice board_choice = BoardChoice::k8x8;
  switch (parseArgs(argc, argv, board_choice)) {
    case ArgParseResult::kHelp:
      return 0;
    case ArgParseResult::kError:
      return 1;
    case ArgParseResult::kOk:
      break;
  }

  if (board_choice == BoardChoice::k12x12) {
    runCli<board::Board12x12>(board_choice);
  } else {
    runCli<board::Board8x8>(board_choice);
  }

  return 0;
}
