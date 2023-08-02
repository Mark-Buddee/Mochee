#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "inc/defs.h"
#include "inc/move.h"
#include "inc/board.h"
#include "inc/gen.h"
#include "inc/bitboard.h"
#include "inc/eval.h"
#include "inc/tt.h"
#include "inc/tgui.h"

void do_move(Board_s* const Board, const Move move) {
    int src = SRC(move);
    int dst = DST(move);
    int ppt = PPT(move);
    int spc = SPC(move);
    int cpt  = Board->pieces[dst];
    int mvd  = Board->pieces[src];
    int side = Board->side;
    int originalCastlingRights = Board->castlingRights;

    // Undo
    Undo_s Undo = {move, cpt, Board->castlingRights, Board->hundredPly, Board->enPas, Board->staticEval, Board->key};
    Board->Undos[Board->hisPly] = Undo;

    // Static evaluation
    Board->staticEval += move_eval(Board, move);

    // Regular piece movement
    if(cpt) remove_piece(Board, cpt, dst, !side);
    if(spc == PROMOTION) {
        remove_piece(Board, mvd, src, side);
        add_piece(Board, KNIGHT + ppt, dst, side);
    } else move_piece(Board, mvd, src, dst, side);
    
    // Special moves
    if(spc == EN_PASSANT) remove_piece(Board, PAWN, enp_cpt[dst], !side);
    else if(spc == CASTLING) {
        int castle = src < dst ? side == WHITE ? WHITE_OO  : BLACK_OO
                               : side == WHITE ? WHITE_OOO : BLACK_OOO;
        move_piece(Board, ROOK, rook_src[castle], rook_dst[castle], side);
        Board->castlingRights &= ~(side == WHITE ? WHITE_CASTLING : BLACK_CASTLING);
    }

    // Update metadata
    Board->hisPly++;
    Board->hundredPly = (cpt || mvd == PAWN) ? 0 : Board->hundredPly + 1;

    // TODO: Can add many conditions here
    if(mvd == KING)                               Board->castlingRights &= ~(side == WHITE ? WHITE_CASTLING : BLACK_CASTLING);
    else if(mvd == ROOK && corner_to_castle[src]) Board->castlingRights &= ~corner_to_castle[src];
    if(cpt == ROOK && corner_to_castle[dst])      Board->castlingRights &= ~corner_to_castle[dst];

    if(Board->enPas) Board->key ^= zobrist_enpSq[lsb(Board->enPas)];
    if(mvd == PAWN && vert_dist(dst, src) == 2) { // TODO: quicken >:)
        Board->enPas = enp_sq[dst];
        Board->key ^= zobrist_enpSq[dst];
    } else Board->enPas = 0ULL; // no zobrist key change assosciated with this

    Board->side = !Board->side;
    Board->key ^= zobrist_blackToPlay;

    if(Board->castlingRights != originalCastlingRights) {
        Board->key ^= zobrist_castle[originalCastlingRights];
        Board->key ^= zobrist_castle[Board->castlingRights];
    }

    // Update check metadata
    update_check_data(Board, move, mvd, 0);
}

void undo_move(Board_s* const Board) {
    int newHisPly = Board->hisPly - 1;
    Move move = Board->Undos[newHisPly].move;
    int cpt = Board->Undos[newHisPly].captured;
    int src = SRC(move);
    int dst = DST(move);
    int spc = SPC(move);
    int mvd = Board->pieces[dst];
    int side = Board->side;

    // Regular piece movement
    if(spc == PROMOTION) {
        remove_piece(Board, mvd, dst, !side);
        add_piece(Board, PAWN, src, !side);
    } else move_piece(Board, mvd, dst, src, !side);
    if(cpt) add_piece(Board, cpt, dst, side);

    // Special moves
    if(spc == EN_PASSANT) add_piece(Board, PAWN, enp_cpt[dst], side);
    else if(spc == CASTLING) {
        int castle = src < dst ? !side == WHITE ? WHITE_OO  : BLACK_OO
                               : !side == WHITE ? WHITE_OOO : BLACK_OOO;
        move_piece(Board, ROOK, rook_dst[castle], rook_src[castle], !side);
    }

    // Meta data
    Board->hisPly = newHisPly;
    Board->castlingRights = Board->Undos[newHisPly].castlingRights;
    Board->hundredPly = Board->Undos[newHisPly].hundredPly;
    Board->enPas = Board->Undos[newHisPly].enPas;
    Board->staticEval = Board->Undos[newHisPly].staticEval;
    Board->side = !Board->side;
    Board->key = Board->Undos[newHisPly].key;
    //Board->key ^= zobrist_blackToMove;

    // Update check metadata
    update_check_data(Board, move, mvd, 1);

    // if(finalCastlingRights != Board->castlingRights) {
    //     Board->key ^= zobrist_castle[finalCastlingRights];
    //     Board->key ^= zobrist_castle[Board->castlingRights];
    // }
    // if(finalEnPas) Board->key ^= zobrist_enpSq[lsb(finalEnPas)];
    // if(Board->enPas) Board->key ^= zobrist_enpSq[lsb(Board->enPas)];

    assert(Board->Undos[newHisPly].key == Board->key);
}



