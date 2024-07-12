#include <stdlib.h>
#include <stdio.h>
#include "inc/defs.h"
#include "inc/eval.h"
#include "inc/board.h"
#include "inc/bitboard.h"

int get_value(int pieceType) {
    switch(pieceType) {
        case PAWN:
            return PAWN_VAL;
        case KNIGHT:
            return KNIGHT_VAL;
        case BISHOP:
            return BISHOP_VAL;
        case ROOK:
            return ROOK_VAL;
        case QUEEN:
            return QUEEN_VAL;
        case KING:
            return KING_VAL;
        default:
            printf("Unexpected input in get_value(). Exiting...\n");
            exit(1);
    }
}

int get_psqt(int pieceType, int side, int sq) {
    switch(pieceType) {
        case PAWN:
            return pawn_psqt[side][sq];
        case KNIGHT:
            return knight_psqt[side][sq];
        case BISHOP:
            return bishop_psqt[side][sq];
        case ROOK:
            return rook_psqt[side][sq];
        case QUEEN:
            return queen_psqt[side][sq];
        case KING:
            return king_psqt[side][sq];
        default:
            printf("Unexpected input in get_psqt(). Exiting...\n");
            exit(1);
    }
}

int static_eval(const Board_s* const Board) {
    int eval = 0;
    U64 srcs;

    for(int side = 0; side < 2; side++) {
        int side_eval = 0;

        srcs = piece(Board, PAWN, side);
        while(srcs) {
            int src = pop_lsb(&srcs);
            side_eval += PAWN_VAL;
            side_eval += pawn_psqt[side][src];
        }

        srcs = piece(Board, KNIGHT, side);
        while(srcs) {
            int src = pop_lsb(&srcs);
            side_eval += KNIGHT_VAL;
            side_eval += knight_psqt[side][src];
        }

        srcs = piece(Board, BISHOP, side);
        while(srcs) {
            int src = pop_lsb(&srcs);
            side_eval += BISHOP_VAL;
            side_eval += bishop_psqt[side][src];
        }
        srcs = piece(Board, ROOK, side);
        while(srcs) {
            int src = pop_lsb(&srcs);
            side_eval += ROOK_VAL;
            side_eval += rook_psqt[side][src];
        }
        srcs = piece(Board, QUEEN, side);
        while(srcs) {
            int src = pop_lsb(&srcs);
            side_eval += QUEEN_VAL;
            side_eval += queen_psqt[side][src];
        }
        srcs = piece(Board, KING, side);
        while(srcs) {
            int src = pop_lsb(&srcs);
            side_eval += KING_VAL;
            side_eval += king_psqt[side][src];
        }

        eval += side == WHITE ? side_eval : -side_eval;
    }
    
    return eval;
}

int move_eval(const Board_s* const Board, const Move move) {
    int src = SRC(move);
    int dst = DST(move);
    int ppt = PPT(move);
    int spc = SPC(move);
    int cpt  = Board->pieces[dst];
    int mvd  = Board->pieces[src];
    int side = Board->side;
    int eval = 0;

    eval -= get_psqt(mvd, side, src);
    if(spc == PROMOTION) {
        eval += get_psqt(KNIGHT + ppt, side, dst);
        eval -= PAWN_VAL;
        eval += get_value(KNIGHT + ppt);
    } else {
        eval += get_psqt(mvd, side, dst);
    }
    if(cpt) {
        eval += get_psqt(cpt, !side, dst);
        eval += get_value(cpt);
    }
    if(spc == EN_PASSANT) {
        eval += get_psqt(PAWN, !side, enp_cpt[dst]);
        eval += get_value(PAWN);
    } else if(spc == CASTLING) {
        int castle = src < dst ? side == WHITE ? WHITE_OO  : BLACK_OO
                               : side == WHITE ? WHITE_OOO : BLACK_OOO;
        eval -= get_psqt(ROOK, side, rook_src[castle]);
        eval += get_psqt(ROOK, side, rook_dst[castle]);
    }

    // return eval;
    return side == WHITE ? eval : -eval;
}
