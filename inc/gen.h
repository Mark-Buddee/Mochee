#ifndef GEN_H
#define GEN_H

int legal(const Board_s* Board, const Move move);

U64 gen_bishop_attacks(int src, U64 obstacles);

U64 gen_rook_attacks(int src, U64 obstacles);

U64 gen_queen_attacks(int src, U64 obstacles);

Move_s* gen_all(const Board_s* Board, Move_s* List, const int side, const int type);

Move_s* gen_legal_captures(const Board_s* const Board, Move_s* List);

Move_s* gen_legal(const Board_s* const Board, Move_s* List);

#endif
