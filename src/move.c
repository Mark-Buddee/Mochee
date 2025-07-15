#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "defs.h"
#include "move.h"
#include "board.h"
#include "gen.h"
#include "bitboard.h"
#include "eval.h"
#include "tt.h"
#include "console.h"

// // Add the current board state to the Transposition Table, or increment the position frequency if it already exists
// void inc_posFreq(Board_s* const Board) {

//     // Position already exists in transposition table
//     if(TT[Board->key % TT_ENTRIES].key == Board->key >> 32) { // This is --Almost-- the only key check of the whole program
//         TT[Board->key % TT_ENTRIES].posFreq++; // increment
//         return;
//     }

//     // Type-1 collsion between two nodes in the history tree
//     // We prioritise the existing node and discard this new one
//     // This rare edge case means we cannot assume that the current board state is always in the transposition
//     // table without making some concessions + (rare) errors
//     if(TT[Board->key % TT_ENTRIES].posFreq != 0) return;

//     // Position does not exist in transposition table
//     TTEntry_s NewEntry = {.key = Board->key >> 32, .move = NULL_MOVE, .scoreBound = BLANK_NODE, .posFreq = 1, .depth = 0, .age = 0};
//     TT[Board->key % TT_ENTRIES] = NewEntry;

//     return;
// }

// // Decrement the position frequency of the board state in the Transposition Table
// void dec_posFreq(Board_s* const Board) {

//     // Position may not exist in Transposition Table
//     if(TT[Board->key % TT_ENTRIES].key != Board->key >> 32) {
//         return;
//         // TTEntry_s NewEntry = {.key = Board->key >> 32, .move = NULL_MOVE, .scoreBound = BLANK_NODE, .posFreq = 1, .depth = 0, .age = 0};
//         // TT[Board->key % TT_ENTRIES] = NewEntry;
//     }

//     TT[Board->key % TT_ENTRIES].posFreq--; // decrement
//     return;
// }

void do_move(Board_s* const Board, const Move_s* cur) {
    assert(cur->move != NULL_MOVE);
    int src = SRC(cur->move);
    int dst = DST(cur->move);
    int ppt = PPT(cur->move);
    int spc = SPC(cur->move);
    int cpt  = Board->pieces[dst];
    int mvd  = Board->pieces[src];
    int side = Board->side;
    int originalCastlingRights = Board->castlingRights;

    assert(Board->hisPly < MAX_GAME_PLYS);

    // assert(cpt != KING); // Not a valid assert because of quiesce()
    // if(cpt == KING) {
    //     undo_move(Board);
    //     print_detailed(Board, Board->side);
    //     undo_move(Board);
    //     print_detailed(Board, Board->side);
    // }

    // Undo
    Undo_s Undo = {
        .move = cur->move, 
        .captured = cpt, 
        .castlingRights = Board->castlingRights, 
        .hundredPly = Board->hundredPly, 
        .enPas = Board->enPas, 
        .checkers = Board->checkers,
        .kingBlockers[WHITE] = Board->kingBlockers[WHITE],
        .kingBlockers[BLACK] = Board->kingBlockers[BLACK],
        .staticEval = Board->staticEval, 
        .key = Board->key
    };
    // for(int side = WHITE; side <= BLACK; side++) {
    //     Undo.kingBlockers[side] = Board->kingBlockers[side];
    //     for(int piece = PAWN; piece <= QUEEN; piece++) {
    //         Undo.checkSquares[side][piece] = Board->checkSquares[side][piece];
    //     }
    // }
    Board->Undos[Board->hisPly] = Undo;

    // Static evaluation
    // Board->staticEval += side == WHITE ? move_eval(Board, cur->move) : -move_eval(Board, cur->move);
    int staticScore = cur->positionScore + cur->materialScore;
    Board->staticEval += side == WHITE ? staticScore : -staticScore;

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
    update_check_data(Board, cur->move, mvd);

    // // Record frequency of this position for 3-fold repitition
    // // Record the NEW position in the TT
    // inc_posFreq(Board);
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

    // // Record frequency of this position for 3-fold repetition
    // // Remove the OLD position from the TT
    // dec_posFreq(Board);

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
    for(int side = WHITE; side <= BLACK; side++) {
        Board->kingBlockers[side] = Board->Undos[newHisPly].kingBlockers[side];
        // for(int piece = PAWN; piece <= QUEEN; piece++) {
        //     Board->checkSquares[side][piece] = Board->Undos[newHisPly].checkSquares[side][piece];
        // }
    }
    Board->staticEval = Board->Undos[newHisPly].staticEval;
    Board->side = !Board->side;
    Board->key = Board->Undos[newHisPly].key;
}
