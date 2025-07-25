#ifndef ENGINE_HPP
#define ENGINE_HPP

template <typename BoardType>
class ChessEngine {
 public:
  ChessEngine() = default;
  ~ChessEngine() = default;
  void startGame();

 private:
  BoardType board;
};
#endif