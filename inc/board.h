#ifndef BOARD_H
#define BOARD_H

U64 piece(const Board_s* const Board, const int pieceType, const int side);

U64 attackers_to(const Board_s* const Board, int sq, U64 obstacles);

void init_check_data(Board_s* const Board);
void update_checkSquares(Board_s* const Board, const int side);
void update_check_data(Board_s* const Board, const Move move, const int mvd);

void add_piece(Board_s* const Board, const int pieceType, const int square, const int side);
void remove_piece(Board_s* const Board, const int pieceType, const int square, const int side);
void move_piece(Board_s* const Board, const int pieceType, const int src, const int dst, const int side);
int isLegal(const Board_s* const Board);

Board_s board_init(const char* fen);

#endif
