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

void do_move(Board_s* const Board, const Move Move) {
    assert(Move != NULL_MOVE);
    int src = SRC(Move);
    int dst = DST(Move);
    int ppt = PPT(Move);
    int spc = SPC(Move);
    int cpt  = Board->pieces[dst];
    int mvd  = Board->pieces[src];
    int side = Board->side;
    int originalCastlingRights = Board->castlingRights;

    // assert(cpt != KING);
    // if(cpt == KING) {
    //     undo_move(Board);
    //     print_detailed(Board, Board->side);
    //     undo_move(Board);
    //     print_detailed(Board, Board->side);
    // }

    // Undo
    Undo_s Undo = {Move, cpt, Board->castlingRights, Board->hundredPly, Board->enPas, Board->checkers, .kingBlockers[WHITE] = Board->kingBlockers[WHITE], .kingBlockers[BLACK] = Board->kingBlockers[BLACK], Board->staticEval, Board->key};
    Board->Undos[Board->hisPly] = Undo;

    // Static evaluation
    // assert(Move->)
    Board->staticEval += move_eval(Board, Move);

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
    if(mvd == PAWN && abs(dst - src) == 16) {
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
    update_check_data(Board, Move, mvd);
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
    Board->checkers = Board->Undos[newHisPly].checkers;
    Board->kingBlockers[WHITE] = Board->Undos[newHisPly].kingBlockers[WHITE];
    Board->kingBlockers[BLACK] = Board->Undos[newHisPly].kingBlockers[BLACK];
    Board->staticEval = Board->Undos[newHisPly].staticEval;
    Board->side = !Board->side;
    Board->key = Board->Undos[newHisPly].key;

    // // Update check metadata
    // U64 srcDst64 = BIT(src) | BIT(dst);
    // if(srcDst64 & Board->checkSquares[side][QUEEN])
    //     update_checkSquares(Board, side);
    // if(mvd == KING)
    //     update_checkSquares(Board, !side);
    // else if(srcDst64 & Board->checkSquares[!side][QUEEN])
    //     update_checkSquares(Board, !side);

}
