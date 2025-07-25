#ifndef CHESS_PIECE_HPP
#define CHESS_PIECE_HPP

#include <utility>  // std::pair

enum class PieceType { Pawn, Knight, Bishop, Rook, Queen, King };

class ChessPiece {
 public:
  ChessPiece(int color, std::pair<int, int> position)
      : color(color), position(position) {};
  virtual int move(
      std::pair<int, int> to);  // Wirtualna metoda do wykonania ruchu
  int getColor() const {
    return color;
  };  // Zwraca kolor figury (0 - biały, 1 - czarny)
  //   virtual get
 protected:
  int color;                     // 0 - biały, 1 - czarny
  std::pair<int, int> position;  // Pozycja figury na planszy (x, y)
  virtual int validateMove(
      std::pair<int, int> to) const = 0;  // Sprawdza poprawność ruchu
};

class Pawn : public ChessPiece {
 public:
  Pawn(int color, std::pair<int, int> position) : ChessPiece(color, position) {}
  void Promote(PieceType newType);
};

class Knight : public ChessPiece {
 public:
  Knight(int color, std::pair<int, int> position)
      : ChessPiece(color, position) {}
};

class Bishop : public ChessPiece {
 public:
  Bishop(int color, std::pair<int, int> position)
      : ChessPiece(color, position) {}
};

class Rook : public ChessPiece {
 public:
  Rook(int color, std::pair<int, int> position) : ChessPiece(color, position) {}
};

class Queen : public ChessPiece {
 public:
  Queen(int color, std::pair<int, int> position)
      : ChessPiece(color, position) {}
};

class King : public ChessPiece {
 public:
  King(int color, std::pair<int, int> position) : ChessPiece(color, position) {}
  bool isInCheck() const;      // Sprawdza, czy król jest w szachu
  bool isInCheckmate() const;  // Sprawdza, czy król jest w szachu matowym
  bool isInStalemate() const;  // Sprawdza, czy król jest w patowej sytuacji
};

#endif