#include <stdlib.h>
#include "inc/defs.h"
#include "inc/init.h"
#include "inc/gen.h"
#include "inc/bitboard.h"
#include "inc/magic.h"
#include "inc/tt.h"

void init_pawn_attacks(void) {
    for(int side = WHITE; side < NUM_SIDES; side++)
        for(int sq = A1; sq <= H8; sq++)
            pawn_attacks[side][sq] = side == WHITE ? shift(BIT(sq), NORTH_WEST) | shift(BIT(sq), NORTH_EAST)
                                                   : shift(BIT(sq), SOUTH_WEST) | shift(BIT(sq), SOUTH_EAST);
}

void init_pseudo_attacks(void) {
    for(int sq = A1; sq <= H8; sq++) {
        U64 bit = BIT(sq);
        pseudo_attacks[EMPTY ][sq] = 0;
        pseudo_attacks[PAWN  ][sq] = pawn_attacks[WHITE][sq] | pawn_attacks[BLACK][sq];
        pseudo_attacks[KNIGHT][sq] = shift(bit, NORTH+NORTH_EAST)
                                   | shift(bit, NORTH+NORTH_WEST)
                                   | shift(bit, EAST+NORTH_EAST)
                                   | shift(bit, WEST+NORTH_WEST)
                                   | shift(bit, EAST+SOUTH_EAST)
                                   | shift(bit, WEST+SOUTH_WEST)
                                   | shift(bit, SOUTH+SOUTH_EAST)
                                   | shift(bit, SOUTH+SOUTH_WEST);
        pseudo_attacks[BISHOP][sq] = gen_bishop_slides(sq, 0);
        pseudo_attacks[ROOK  ][sq] = gen_rook_slides(sq, 0);
        pseudo_attacks[QUEEN ][sq] = gen_queen_slides(sq, 0);
        pseudo_attacks[KING  ][sq] = shift(bit, NORTH)
                                   | shift(bit, NORTH_EAST)
                                   | shift(bit, NORTH_WEST)
                                   | shift(bit, EAST)
                                   | shift(bit, WEST)
                                   | shift(bit, SOUTH_EAST)
                                   | shift(bit, SOUTH_WEST)
                                   | shift(bit, SOUTH);
    }       
}

void init_castling_to(void) {
    for(int i = 0; i < NUM_CASTLING; i++)
        castling_to[i] = 0;

    castling_to[WHITE_OO]  = G1;
    castling_to[WHITE_OOO] = C1;
    castling_to[BLACK_OO]  = G8;
    castling_to[BLACK_OOO] = C8;
}

void init_castling_path(void) {
    for(int i = 0; i < NUM_CASTLING; i++)
        castling_path[i] = 0;

    castling_path[WHITE_OO]  = BIT(F1) | BIT(G1);
    castling_path[WHITE_OOO] = BIT(D1) | BIT(C1) | BIT(B1);
    castling_path[BLACK_OO]  = BIT(F8) | BIT(G8);
    castling_path[BLACK_OOO] = BIT(D8) | BIT(C8) | BIT(B8);
}

void init_rook_src(void) {
    for(int i = 0; i < NUM_CASTLING; i++)
        rook_src[i] = 0;

    rook_src[WHITE_OO]  = H1;
    rook_src[WHITE_OOO] = A1;
    rook_src[BLACK_OO]  = H8;
    rook_src[BLACK_OOO] = A8;
}

void init_rook_dst(void) {
    for(int i = 0; i < NUM_CASTLING; i++)
        rook_dst[i] = 0;

    rook_dst[WHITE_OO]  = F1;
    rook_dst[WHITE_OOO] = D1;
    rook_dst[BLACK_OO]  = F8;
    rook_dst[BLACK_OOO] = D8;
}

void init_corner_to_castle(void) {
    for(int sq = A1; sq <= H8; sq++)
        corner_to_castle[sq] = 0;

    corner_to_castle[A1] = WHITE_OOO;
    corner_to_castle[H1] = WHITE_OO;
    corner_to_castle[A8] = BLACK_OOO;
    corner_to_castle[H8] = BLACK_OO;
}

void init_enp_sq(void) {
    for(int sq = A1; sq <= H8; sq++)
        enp_sq[sq] = BIT(sq) & RANK_4BB ? BIT(sq + SOUTH) :
                     BIT(sq) & RANK_5BB ? BIT(sq + NORTH) :
                     0;
}

void init_enp_cpt(void) {
    for(int sq = A1; sq <= H8; sq++)
        enp_cpt[sq] = BIT(sq) & RANK_3BB ? sq + NORTH :
                      BIT(sq) & RANK_6BB ? sq + SOUTH :
                      0; 
}

void init_lines(U64* lines) {
    int i = 0;
    U64 tLine;

    // Diagonal lines towards north-east
    tLine = 0x8040201008040201; // A1 - H8
    for(int j = 0; j < 8; j++) {
        U64 tLineNorth = tLine;
        U64 tLineEast  = tLine;
        for(int k = 0; k < j; k++) {
            tLineNorth = shift(tLineNorth, NORTH);
            tLineEast  = shift(tLineEast,  EAST);
        }
        lines[i++] = tLineNorth;
        if(j) lines[i++] = tLineEast;
    }

    // Diagonal lines towards south-east
    tLine = 0x0102040810204080; // A8 - H1
    for(int j = 0; j < 8; j++) {
        U64 tLineNorth = tLine;
        U64 tLineWest  = tLine;
        for(int k = 0; k < j; k++) {
            tLineNorth = shift(tLineNorth, NORTH);
            tLineWest  = shift(tLineWest,  WEST);
        }
        lines[i++] = tLineNorth;
        if(j) lines[i++] = tLineWest;
    }

    // Vertical lines
    tLine = FILE_ABB;
    for(int j = 0; j < NUM_FILES; j++) {
        U64 tLineEast = tLine;
        for(int k = 0; k < j; k++)
            tLineEast = shift(tLineEast, EAST);
        lines[i++] = tLineEast;
    }

    // Horizontal lines
    tLine = RANK_1BB;
    for(int j = 0; j < NUM_RANKS; j++) {
        U64 tLineNorth = tLine;
        for(int k = 0; k < j; k++)
            tLineNorth = shift(tLineNorth, NORTH);
        lines[i++] = tLineNorth;
    }
}

void init_line(void) {
    for(int sq1 = A1; sq1 <= H8; sq1++)
        for(int sq2 = A1; sq2 <= H8; sq2++)
            line[sq1][sq2] = 0;

    U64 lines[NUM_LINES];
    init_lines(lines);

    for(int sq1 = A1; sq1 <= H8; sq1++) {
        for(int sq2 = A1; sq2 <= H8; sq2++) {
            if(sq1 == sq2) continue;
            U64 bit1 = BIT(sq1);
            U64 bit2 = BIT(sq2);
            for(int k = 0; k < NUM_LINES; k++) {
                if(lines[k] & bit1 && lines[k] & bit2) {
                    line[sq1][sq2] = lines[k];
                    break;
                }
            }
        }
    }
}

void init_between(void) {
    for(int sq1 = A1; sq1 <= H8; sq1++)
        for(int sq2 = A1; sq2 <= H8; sq2++)
            between[sq1][sq2] = 0;

    for(int sq1 = A1; sq1 <= H8; sq1++) {
        for(int sq2 = A1; sq2 <= H8; sq2++) {
            if(!line[sq1][sq2]) {
                between[sq1][sq2] = BIT(sq2);
                continue;
            }
            U64 bb = line[sq1][sq2];
            U64 endpoints = BIT(sq1) | BIT(sq2);
            U64 cutoff;

            cutoff = RANK_8BB;
            while(!(cutoff & endpoints)) {
                bb &= ~cutoff;
                cutoff = shift(cutoff, SOUTH);
            }
            cutoff = RANK_1BB;
            while(!(cutoff & endpoints)) {
                bb &= ~cutoff;
                cutoff = shift(cutoff, NORTH);
            }
            cutoff = FILE_ABB;
            while(!(cutoff & endpoints)) {
                bb &= ~cutoff;
                cutoff = shift(cutoff, EAST);
            }
            cutoff = FILE_HBB;
            while(!(cutoff & endpoints)) {
                bb &= ~cutoff;
                cutoff = shift(cutoff, WEST);
            }
            between[sq1][sq2] = bb & ~(BIT(sq1));
        }
    }
    
}

void init_all(void) {
    init_pawn_attacks();
    init_pseudo_attacks();
    init_castling_to();
    init_castling_path();
    init_rook_src();
    init_rook_dst();
    init_corner_to_castle();
    init_enp_sq();
    init_enp_cpt();
    init_line(); // must come before init_between
    init_between();

    init_magic();
    init_zobrist();
    init_tt();
}