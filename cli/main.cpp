#include <fmt/base.h>

#include <iostream>
#include <string>

/**
 * @brief Funkcja główna programu.
 *
 * @return int
 */
int main() {
  // tutaj wydaje mi sie ze odpalamn se te rzeczy z src/cli
  // w src/cli odpalam instancje engine i dodaje obsługe komunikajci z reszta
  std::string command;
  while (true) {
    fmt::print("> ");
    std::getline(std::cin, command);
    if (command == "exit") {
      fmt::print("Koniec gry.\n");
      break;
    }
    // Tu możesz dodać obsługę komend, np. ruchów, wyświetlania planszy
    fmt::print("Wpisano: {}\n", command);
  }
  return 0;
}