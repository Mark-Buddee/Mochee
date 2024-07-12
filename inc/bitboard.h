#ifndef BITBOARD_H
#define BITBOARD_H

extern U64 pawn_attacks[NUM_SIDES][NUM_SQUARES];
extern U64 pseudo_attacks[NUM_PIECES][NUM_SQUARES];

extern int castling_to[NUM_CASTLING];
extern U64 castling_path[NUM_CASTLING];
extern int rook_src[NUM_CASTLING];
extern int rook_dst[NUM_CASTLING];
extern int corner_to_castle[NUM_SQUARES];

extern U64 enp_sq[NUM_SQUARES];
extern int enp_cpt[NUM_SQUARES];

extern U64 line[NUM_SQUARES][NUM_SQUARES];
extern U64 between[NUM_SQUARES][NUM_SQUARES];

int lsb(U64 bb);
int pop_lsb(U64* bbPtr);
U64 file64(const int sq);
// int vert_dist(const int sq1, const int sq2);
U64 more_than_one(const U64 bb);
U64 aligned(const int sq1, const int sq2, const U64 bit);
U64 shift(const U64 bb, const int direction);
U64 gen_slide(const int src, const int directions[4][2], const U64 obstacles);
U64 gen_bishop_slides(const int src, const U64 obstacles);
U64 gen_rook_slides(const int src, const U64 obstacles);
U64 gen_queen_slides(const int src, const U64 obstacles);
U64 attacks(const int pieceType, const int src, const U64 obstacles);

#endif