#ifndef GAME_TERMINAL_STYLE_HPP
#define GAME_TERMINAL_STYLE_HPP

#include <cstdlib>
#include <string>

#ifdef _WIN32
#include <io.h>
#define CPP_CHESS_ISATTY _isatty
#define CPP_CHESS_STDOUT_FILENO _fileno(stdout)
#else
#include <unistd.h>
#define CPP_CHESS_ISATTY isatty
#define CPP_CHESS_STDOUT_FILENO STDOUT_FILENO
#endif

namespace game::terminal {

inline bool colorsEnabled() {
  if (const char* no_color = std::getenv("NO_COLOR"); no_color != nullptr && no_color[0] != '\0') {
    return false;
  }
  return CPP_CHESS_ISATTY(CPP_CHESS_STDOUT_FILENO) != 0;
}

inline std::string backgroundRgb(int red, int green, int blue) {
  return "\033[48;2;" + std::to_string(red) + ';' + std::to_string(green) + ';' +
         std::to_string(blue) + 'm';
}

inline constexpr const char* kReset = "\033[0m";
inline constexpr const char* kWhitePiece = "\033[1;97m";
inline constexpr const char* kBlackPiece = "\033[38;2;40;40;40m";
inline constexpr const char* kBorder = "\033[38;2;120;90;60m";
inline constexpr const char* kLabel = "\033[38;2;180;180;180m";
inline constexpr const char* kCheck = "\033[1;33m";
inline constexpr const char* kMate = "\033[1;31m";
inline constexpr const char* kDraw = "\033[1;36m";

}  // namespace game::terminal

#endif
