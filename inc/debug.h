#ifndef DEBUG_H
#define DEBUG_H

void print_bitBoard(U64 bitBoard);
void print_detailed(const Board_s* const Board, int flipped);
void print_pgn(const Board_s* const Board);
void print_board(const Board_s* const Board, int flipped);
void print_variation(Board_s* const Board, int maxDepth);

void print_move(Move move);

#endif