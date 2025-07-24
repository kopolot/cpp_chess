#ifndef ENGINE_HPP
#define ENGINE_HPP

template <typename BoardType> class ChessEngine {
public:
  ChessEngine();
  ~ChessEngine();
  void initialize();
  void reset();

private:
  BoardType board;
};
#endif