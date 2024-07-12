#ifndef FEN_H
#define FEN_H

int fen_piece(const char fenPiece);

int fen_colour(const char fenPiece);

int fen_side(const char* fenSide);

int fen_castlingRights(const char* fenCastlingRights);

int fen_hundredPly(const char* fenHalfMove);

int fen_hisPly(const char* fenFullMove, const int side);

U64 fen_enPas(const char* fenEnPas);

#endif
