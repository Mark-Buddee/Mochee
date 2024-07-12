#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "inc/defs.h"
#include "inc/gen.h"
#include "inc/tgui.h"
#include "inc/init.h"
#include "inc/board.h"
#include "inc/bitboard.h"
#include "inc/magic.h"

Move_s* make_move(Move_s* List, const int src, const int dst) {
    (List++)->move = MOVE(src, dst, KNIGHT, NORMAL);
    return List;
}

Move_s* make_enPas(Move_s* List, const int src, const int dst) {
    (List++)->move = MOVE(src, dst, KNIGHT, EN_PASSANT);
    return List;
}

Move_s* make_promotions(Move_s* List, const int dst, const int type, const int direction) {
    if(type == CAPTURES || type == EVASIONS || type == NON_EVASIONS)
        (List++)->move = MOVE((dst - direction), dst, QUEEN, PROMOTION);

    if(type == QUIETS || type == EVASIONS || type == NON_EVASIONS) {
        (List++)->move = MOVE((dst - direction), dst, ROOK, PROMOTION);
        (List++)->move = MOVE((dst - direction), dst, BISHOP, PROMOTION);
        (List++)->move = MOVE((dst - direction), dst, KNIGHT, PROMOTION);
    }
    return List;
}

Move_s* make_castling(Move_s* List, const int src, const int dst) {
    (List++)->move = MOVE(src, dst, KNIGHT, CASTLING);
    return List;
}

int legal(const Board_s* Board, const Move move) {
    int side = Board->side;
    int src = SRC(move);
    int dst = DST(move);
    int spc = SPC(move);

    // En passant is relatively uncommon. Simply check if the king is attacked
    if(spc == EN_PASSANT) {
        int side = Board->side;
        int ksq = lsb(piece(Board, KING, side));
        // assert(ksq);
        U64 obstacles = (Board->byType[ALL] ^ BIT(src) ^ BIT(enp_cpt[dst])) | BIT(dst);

        return !(gen_bishop_magic_attacks(ksq, obstacles) & (piece(Board, QUEEN, !side) | piece(Board, BISHOP, !side)))
            && !(  gen_rook_magic_attacks(ksq, obstacles) & (piece(Board, QUEEN, !side) | piece(Board, ROOK,   !side)));
    }

    // Ensure the king does not castle through check
    if(spc == CASTLING) {
        int step = dst > src ? WEST : EAST;
        U64 obstacles = Board->byType[ALL];

        for(int sq = dst; sq != src; sq += step)
            if(attackers_to(Board, sq, obstacles) & Board->byColour[!side])
                return false;
        return true;
    }
    
    // Ensure the king has not moved into an attacked square
    if(Board->pieces[src] == KING)
        return !(attackers_to(Board, dst, Board->byType[ALL] ^ BIT(src)) & Board->byColour[!side]);

    // A non-king move is legal iff it's not pinned or it only moves along the ray towards or away from the king
    return !(Board->kingBlockers[side] & BIT(src)) || aligned(src, dst, piece(Board, KING, side));
}

Move_s* gen_pawn_moves(const Board_s* Board, Move_s* List, U64 mask, const int side, const int type) {
    const U64 tRank7BB = (side == WHITE ? RANK_7BB   : RANK_2BB);
    const U64 tRank3BB = (side == WHITE ? RANK_3BB   : RANK_6BB);
    const int UP       = (side == WHITE ? NORTH      : SOUTH);
    const int UP_RIGHT = (side == WHITE ? NORTH_EAST : SOUTH_WEST);
    const int UP_LEFT  = (side == WHITE ? NORTH_WEST : SOUTH_EAST);

    const U64 emptySquares = ~Board->byType[ALL];
    const U64 enemies      =  type == EVASIONS ? Board->checkers
                                               : Board->byColour[!side];

    U64 pawnsOn7    = piece(Board, PAWN, side) &  tRank7BB;
    U64 pawnsNotOn7 = piece(Board, PAWN, side) & ~tRank7BB;
    
    // Single and double pawn pushes, no promotions
    if(type != CAPTURES) {
        U64 dsts1 = shift(pawnsNotOn7, UP)      & emptySquares;
        U64 dsts2 = shift(dsts1 & tRank3BB, UP) & emptySquares;

        if(type == EVASIONS) { // Consider only blocking squares
            dsts1 &= mask;
            dsts2 &= mask;
        }

        if(type == QUIET_CHECKS) {
            int enemyKsq = lsb(piece(Board, KING, !side));
            U64 dcCandidatePawns = Board->kingBlockers[!side] & ~file64(enemyKsq);
            dsts1 &= pawn_attacks[!side][enemyKsq] | shift(dcCandidatePawns, UP);
            dsts2 &= pawn_attacks[!side][enemyKsq] | shift(dcCandidatePawns, UP + UP);
        }

        while(dsts1) {
            int dst = pop_lsb(&dsts1);
            List = make_move(List, dst - UP, dst);
        }

        while(dsts2) {
            int dst = pop_lsb(&dsts2);
            List = make_move(List, dst - UP - UP, dst);
        }
    }

    // Promotions
    if(pawnsOn7) {
        U64 dsts1 = shift(pawnsOn7, UP_RIGHT) & enemies;
        U64 dsts2 = shift(pawnsOn7, UP_LEFT)  & enemies;
        U64 dsts3 = shift(pawnsOn7, UP)       & emptySquares;

        if(type == EVASIONS) dsts3 &= mask;

        while(dsts1) {
            int dst = pop_lsb(&dsts1);
            List = make_promotions(List, dst, type, UP_RIGHT);
        }
        while(dsts2) {
            int dst = pop_lsb(&dsts2);
            List = make_promotions(List, dst, type, UP_LEFT);
        }
        while(dsts3) {
            int dst = pop_lsb(&dsts3);
            List = make_promotions(List, dst, type, UP);
        }
    }

    // Standard and en passant captures
    if(type == CAPTURES || type == EVASIONS || type == NON_EVASIONS) {
        U64 dsts1 = shift(pawnsNotOn7, UP_RIGHT) & enemies;
        U64 dsts2 = shift(pawnsNotOn7, UP_LEFT)  & enemies;

        while(dsts1) {
            int dst = pop_lsb(&dsts1);
            List = make_move(List, dst - UP_RIGHT, dst);
        }
        while(dsts2) {
            int dst = pop_lsb(&dsts2);
            List = make_move(List, dst - UP_LEFT, dst);
        }

        if(Board->enPas)
            // An en passant capture cannot resolve a discovered check
            if(type == EVASIONS && (mask & shift(Board->enPas, UP))) return List;
        
        int enPasSq = Board->enPas ? lsb(Board->enPas) : 0;
        U64 srcs = pawnsNotOn7 & pawn_attacks[!side][enPasSq];

        while(srcs) {
            int src = pop_lsb(&srcs);
            List = make_enPas(List, src, lsb(Board->enPas));
        }
    }
    return List;
}

Move_s* gen_moves(const Board_s* Board, Move_s* List, const U64 mask, const int side, const int pieceType, const int quietChecks) {
    U64 srcs = piece(Board, pieceType, side);

    while(srcs) {
        int src = pop_lsb(&srcs);
        U64 dsts = attacks(pieceType, src, Board->byType[ALL]) & mask;

        // if(quietChecks && (pieceType == QUEEN || !(Board->kingBlockers[!side] & BIT(src))))
        //     dsts &= Board->checkSquares[side][pieceType];

        while(dsts) {
            int dst = pop_lsb(&dsts);
            List = make_move(List, src, dst);
        }
    }

    return List;
}

Move_s* gen_all(const Board_s* Board, Move_s* List, const int side, const int type) {
    const int quietChecks = type == QUIET_CHECKS;
    const U64 kingBit = piece(Board, KING, side);
    if(!kingBit) return List;

    const int ksq = lsb(kingBit);
    U64 mask;

    // Non-king moves
    if(type != EVASIONS || !more_than_one(Board->checkers)) {
        mask = type == EVASIONS     ?  between[ksq][lsb(Board->checkers)]
             : type == NON_EVASIONS ? ~Board->byColour[ side]
             : type == CAPTURES     ?  Board->byColour[!side]
             :                        ~Board->byType[ALL];

        List = gen_moves(Board, List, mask, side,  QUEEN, quietChecks);
        List = gen_moves(Board, List, mask, side,   ROOK, quietChecks);
        List = gen_moves(Board, List, mask, side, BISHOP, quietChecks);
        List = gen_moves(Board, List, mask, side, KNIGHT, quietChecks);
        List = gen_pawn_moves(Board, List, mask, side, type);
    }

    // King moves
    if(!quietChecks || Board->kingBlockers[!side] & ksq) {
        U64 dsts = pseudo_attacks[KING][ksq] & (type == EVASIONS ? ~Board->byColour[side] : mask);
        if(quietChecks) {
            int enemyKsq = lsb(piece(Board, KING, !side));
            dsts &= ~pseudo_attacks[QUEEN][enemyKsq];
        }

        while(dsts) {
            int dst = pop_lsb(&dsts);
            List = make_move(List, ksq, dst);
        }

        int castlingSide = side == WHITE ? WHITE_CASTLING : BLACK_CASTLING;
        if((type == QUIETS || type == NON_EVASIONS) && Board->castlingRights & castlingSide) {
            int castle = castlingSide & KING_SIDE;
            U64 impeded = Board->byType[ALL] & castling_path[castle];
            if(!impeded && Board->castlingRights & castle)
                List = make_castling(List, ksq, castling_to[castle]);

            castle = castlingSide & QUEEN_SIDE;
            impeded = Board->byType[ALL] & castling_path[castle];
            if(!impeded && Board->castlingRights & castle)
                List = make_castling(List, ksq, castling_to[castle]);
        }
    }
    return List;
}

/* Move Types
CAPTURES     Generates all pseudo-legal captures plus queen promotions
QUIETS       Generates all pseudo-legal non-captures and underpromotions
EVASIONS     Generates all pseudo-legal check evasions when the side to move is in check
QUIET_CHECKS Generates all pseudo-legal non-captures giving check, except castling and promotions
NON_EVASIONS Generates all pseudo-legal captures and non-captures
*/

Move_s* gen_legal(const Board_s* const Board, Move_s* List) {
    int side = Board->side;
    U64 pinned = Board->kingBlockers[side] & Board->byColour[side];
    Move_s* cur = List;

    List = Board->checkers ? gen_all(Board, List, side, EVASIONS)
                           : gen_all(Board, List, side, NON_EVASIONS);
                        
    while(cur != List) {
        int ksq = lsb(piece(Board, KING, side));
        if(((pinned & BIT(SRC(cur->move))) || SRC(cur->move) == ksq || SPC(cur->move) == EN_PASSANT) && !legal(Board, cur->move))
            cur->move = (--List)->move;
        else
            ++cur;
    }

    return List;
}
