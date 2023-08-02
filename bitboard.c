#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "inc/defs.h"
#include "inc/bitboard.h"
#include "inc/magic.h"

U64 pawn_attacks[NUM_SIDES][NUM_SQUARES];
U64 pseudo_attacks[NUM_PIECES][NUM_SQUARES]; // Don't use for ALL

int castling_to[NUM_CASTLING];
U64 castling_path[NUM_CASTLING];
int rook_src[NUM_CASTLING];
int rook_dst[NUM_CASTLING];
int corner_to_castle[NUM_SQUARES];

U64 enp_sq[NUM_SQUARES];
int enp_cpt[NUM_SQUARES];

U64 line[NUM_SQUARES][NUM_SQUARES];
U64 between[NUM_SQUARES][NUM_SQUARES];

int lsb(U64 bb) {
    assert(bb);
    return __builtin_ctzll(bb);
}

int pop_lsb(U64* bbPtr) {
    int out = lsb(*bbPtr);
    *bbPtr &= *bbPtr - 1;
    return out;
}

U64 file64(const int sq) { // TODO: this can be an array
    int file = FILE(sq);
    U64 tFile = FILE_ABB;
    for(int i = 0; i < file; i++) {
        tFile = shift(tFile, EAST);
    }
    return tFile;
}

int vert_dist(const int sq1, const int sq2) {
    assert(A1 <= sq1 && sq1 <= H8);
    assert(A1 <= sq2 && sq2 <= H8);
    int rank1 = RANK(sq1);
    int rank2 = RANK(sq2);
    return abs(rank2 - rank1);
}

U64 more_than_one(const U64 bb) {
    return bb & (bb - 1);
}

U64 aligned(const int sq1, const int sq2, const U64 bit) {
    return line[sq1][sq2] & bit;
}

U64 shift(const U64 bb, const int direction) {
    return direction == NORTH            ?  bb << 8                           : direction == SOUTH            ?  bb >> 8
         : direction == NORTH+NORTH      ?  bb << 16                          : direction == SOUTH+SOUTH      ?  bb >> 16
         : direction == EAST             ? (bb & ~FILE_HBB) << 1              : direction == WEST             ? (bb & ~FILE_ABB) >> 1
         : direction == NORTH_EAST       ? (bb & ~FILE_HBB) << 9              : direction == NORTH_WEST       ? (bb & ~FILE_ABB) << 7
         : direction == SOUTH_EAST       ? (bb & ~FILE_HBB) >> 7              : direction == SOUTH_WEST       ? (bb & ~FILE_ABB) >> 9
         : direction == NORTH+NORTH_EAST ? (bb & ~FILE_HBB) << 17             : direction == NORTH+NORTH_WEST ? (bb & ~FILE_ABB) << 15
         : direction == SOUTH+SOUTH_EAST ? (bb & ~FILE_HBB) >> 15             : direction == SOUTH+SOUTH_WEST ? (bb & ~FILE_ABB) >> 17
         : direction == EAST+NORTH_EAST  ? (bb & ~FILE_HBB & ~FILE_GBB) << 10 : direction == WEST+NORTH_WEST  ? (bb & ~FILE_ABB & ~FILE_BBB) << 6
         : direction == EAST+SOUTH_EAST  ? (bb & ~FILE_HBB & ~FILE_GBB) >> 6  : direction == WEST+SOUTH_WEST  ? (bb & ~FILE_ABB & ~FILE_BBB) >> 10
         : 0;
}

U64 gen_slides(const int src, const int directions[4][2], const U64 obstacles) {
    U64 dsts = 0;
    for(int i = 0; i < 4; i++) {
        int file = FILE(src);
        int rank = RANK(src);
        U64 currSquare64 = 0;
        while(!(currSquare64 & obstacles)) {  // TODO Make this a do while?
            file += directions[i][0];
            rank += directions[i][1];
            if((file < FILE_A) || (file > FILE_H) || (rank < RANK_1) || (rank > RANK_8)) {
                break;
            }
            int currSquare = FR(file, rank);
            currSquare64 = BIT(currSquare);
            dsts |= currSquare64;
        }
    }
    return dsts;
}

U64 gen_bishop_slides(const int src, const U64 obstacles) {
    const int directions[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    return gen_slides(src, directions, obstacles);
}

U64 gen_rook_slides(const int src, const U64 obstacles) {
    const int directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    return gen_slides(src, directions, obstacles);
}

U64 gen_queen_slides(const int src, const U64 obstacles) {
    return gen_bishop_slides(src, obstacles) | gen_rook_slides(src, obstacles);
}

U64 attacks(const int pieceType, const int src, const U64 obstacles) {
    switch(pieceType) {
        case PAWN:
            return pawn_attacks[WHITE][src] | pawn_attacks[BLACK][src]; // TODO: make side an argument or a different piece type for coloured pawns
        case KNIGHT:
            return pseudo_attacks[KNIGHT][src];
        case BISHOP:
            return gen_bishop_magic_attacks(src, obstacles);
        case ROOK:
            return gen_rook_magic_attacks(src, obstacles);
        case QUEEN:
            return gen_queen_magic_attacks(src, obstacles);
        default:
            printf("Invalid piece type in attacks(). Exiting...");
            exit(1);
    }
}