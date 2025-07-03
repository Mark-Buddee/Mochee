#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#define NDEBUG
#include <assert.h>
#include "inc/defs.h"
#include "inc/move.h"
#include "inc/eval.h"
#include "inc/gen.h"
#include "inc/search.h"
#include "inc/tgui.h"
#include "inc/tt.h"
#include "inc/board.h"
#include "inc/bitboard.h"

extern unsigned long long tableHits;
extern unsigned long long tableUpdates;
extern unsigned long long tableOverwrites;

int is_three_fold(Board_s* Board, int rootPly) {

    // Only need to consider nodes at least 4 plies since the last irreversible move
    if(Board->hundredPly < 4) return 0;

    // Only look 2, 4, 6, ... plies in the past where the player to move was the same as right now
    // until the last irreversible move (pawn move or capture)
    // Undos[hisPly - 2], Undos[hisPly - 4] ... Undos[hisPly - hundredPly]
    int posFreq = 0;
    int deepInSearch = Board->hisPly - rootPly > 2;
    for(int i = Board->hisPly - 2; i >= Board->hisPly - Board->hundredPly; i = i - 2) {

        if(Board->Undos[i].key == Board->key) posFreq++;

        if(deepInSearch && posFreq) return 1; // two-fold repetition

        if(posFreq == 2) return 1; // three-fold repetition

    }

    return 0;

}

int clamp(int lower, int upper, int score) {
    return score <= lower ? lower
         : score >= upper ? upper
         :                  score;
}

// Scores moves based on which are most likely to be good
// Greater score -> more likely to be the best move
// That means that both White and Black's best moves are scored POSITIVELY
void score_moves(Board_s* Board, Move_s* cur, Move_s* end, Move bestMove) {
    while(cur != end) {
        int src = SRC(cur->move);
        int dst = DST(cur->move);
        // int ppt = PPT(cur->move);
        int spc = SPC(cur->move);
        int cpt  = Board->pieces[dst];
        int mvd  = Board->pieces[src];

        int positionScore = move_position_eval(Board, cur->move);
        int materialScore = move_material_eval(Board, cur->move);

        // TODO: Moving to a protected square is good - helps chase down king for mate 
        int orderingBias = 0;
        if(cur->move == bestMove) orderingBias += TABLE_BEST_BIAS;
        if(cpt) {
            orderingBias += CAPTURE_BIAS;
            if(cpt > mvd) orderingBias += WINNING_CAPTURE_BIAS; // This code is broken on promotions
            else if (cpt == mvd) orderingBias += EQUAL_CAPTURE_BIAS;
            orderingBias += CAPTURED_PIECE_MULT*get_value(cpt); // encourage capture of heavy pieces. get_value returns [100, 8191]
            orderingBias -= MOVED_PIECE_PENALTY_MULT*mvd; // encourage cheapest piece making the capture. mvd is [1, 6]
        }
        if(spc == PROMOTION) orderingBias += PROMOTION_BIAS;

        // TODO: Giving check is good
        if(Board->checkers) orderingBias += CHECK_BIAS;
        
        cur->positionScore = positionScore;
        cur->materialScore = materialScore;
        cur->orderingBias = orderingBias;

        cur++;
    }
}

void partial_insertion_sort(Move_s* begin, Move_s* end, int limit) { 
    // Runs slightly slower because of the repeated positionScore  + orderinBias sum

    // Copied from stockfish
    for (Move_s *sortedEnd = begin, *p = begin + 1; p < end; ++p)
        if (p->positionScore + p->orderingBias >= limit) {
            Move_s tmp = *p, *q;
            *p = *++sortedEnd;
            for (q = sortedEnd; q != begin && ((q - 1)->positionScore + (q - 1)->orderingBias) < (tmp.positionScore + tmp.orderingBias); --q)
                *q = *(q - 1);
            *q = tmp;
        }
}

int quiesce(Board_s* const Board, int alpha, int beta, int rootPly) {

    // Fifty move rule
    if(Board->hundredPly == HUNDRED_PLIES) return 0;

    // 3-fold repetition
    if(is_three_fold(Board, rootPly)) return 0;

    // static int maxDepth = 0;
    // if(Board->hisPly - startPly > maxDepth) {
    //     maxDepth = Board->hisPly - startPly;
    //     printf("%d\n", maxDepth);

    //     if(maxDepth == MAX_DEPTH - 1) {
    //         // print_pgn(Board);
    //         while(Board->hisPly > 0) {
    //             // print_detailed(Board, WHITE);
    //             // print_board(Board, WHITE);
    //             undo_move(Board);
    //         }
    //     }
    // }

    int raisedAlpha = 0;
    int staticEval = Board->side == WHITE ? Board->staticEval : -Board->staticEval;

    // Arbitrary depth limit
    // if(Board->hisPly == MAX_DEPTH)
    //     return staticEval >= beta ? beta
    //          : staticEval > alpha ? staticEval
    //          : alpha;

    Move bestMove = NULL_MOVE;
    Move_s List[MAX_MOVES];
    Move_s* cur = List;
    Move_s* end;

    if(Board->checkers) {
        end = gen_legal(Board, List);
        if(cur == end) return alpha; // checkmate

    } else {
        if(staticEval >= beta) return beta;
        if(staticEval > alpha) {
            alpha = staticEval;
            raisedAlpha = 1;
        }
        end = gen_all(Board, List, Board->side, CAPTURES);
    }

    score_moves(Board, List, end, bestMove);
    partial_insertion_sort(List, end, INSERTION_SORT_MIN);

    while(cur != end) {

        // Delta pruning
        if(!Board->checkers) {
            int cpt = SPC(cur->move) == PROMOTION  ? QUEEN_VAL
                    : SPC(cur->move) == EN_PASSANT ? PAWN_VAL
                    : get_value(Board->pieces[DST(cur->move)]);

            assert(cpt);
            if(staticEval + cpt < alpha - DELTA_VAL) {
                cur++;
                continue;
            }
        }

        do_move(Board, cur);
        int score = -quiesce(Board, -beta, -alpha, rootPly);
        undo_move(Board);

        if(score >= beta) return beta;
        if(score > alpha) {
            alpha = score;
            raisedAlpha = 1;
        }
        cur++;
    }

    // If alpha has been raised, or if all moves have been searched, there is no need to keep searching
    if(raisedAlpha || Board->checkers) return alpha; // Board->hisPly - startPly < MAX_QUIET_CHECK_PLIES
    return alpha;

    // Fix the following so the comment is not true!
    // We've already checked that staticEval <= alpha
    // if(staticEval < alpha) return alpha;
    // if(staticEval < alpha - QUIET_DELTA_VAL) return alpha; // Removing this condition creates segfault - endless checks are searched.
    // assert(staticEval == alpha);


    // If no captures raised alpha, we investigate quiet checks
    // printf("activate\n");
    cur = List;
    end = gen_all(Board, List, Board->side, QUIET_CHECKS);
    score_moves(Board, List, end, bestMove);
    partial_insertion_sort(List, end, INSERTION_SORT_MIN);

    while(cur != end) {

        if(cur->positionScore <= 0) { // Only allow quiet checks that IMPROVE the position
            cur++;
            continue;
        }

        do_move(Board, cur);
        int score = -quiesce(Board, -beta, -alpha, rootPly);
        undo_move(Board);

        if(score >= beta) return beta;
        if(score > alpha) alpha = score;
        cur++;
    }

    return alpha;
}

int alpha_beta(Board_s* const Board, int alpha, int beta, int depth, int rootPly, clock_t endTime) {

    // Fifty move rule
    if(Board->hundredPly == HUNDRED_PLIES) return 0;

    // 3-fold repetition
    if(is_three_fold(Board, rootPly)) return 0;
    
    Move bestMove = NULL_MOVE;
    TTEntry_s Entry = TT[Board->key % TT_ENTRIES];

    if(Entry.key == Board->key >> 32) {

        if(Entry.depth >= depth) {
            // tableHits++;
            if(IS_PV_NODE(Entry.scoreBound)) return SCORE(Entry.scoreBound); // exact score
            if(IS_CUT_NODE(Entry.scoreBound) && SCORE(Entry.scoreBound) >= beta) return beta; // lower bound exceeds beta
            if(IS_ALL_NODE(Entry.scoreBound) && SCORE(Entry.scoreBound) <= alpha) return alpha; // upper bound is below alpha
        }
        bestMove = Entry.move;

    }

    assert(depth >= 0);
    // Add quiesce entry to TT
    if(depth == 0) return quiesce(Board, alpha, beta, rootPly);
    // if(depth == 0) return Board->side == WHITE ? Board->staticEval : -Board->staticEval;

    Move_s List[MAX_MOVES];
    Move_s* cur = List;
    Move_s* end = gen_legal(Board, List);

    // if(cur == end) return Board->checkers ? alpha : 0;
    if(cur == end) {
        if(Board->checkers) {
            add_entry(Board->key, NULL_MOVE, SCOREBOUND(-INF, PV_NODE), MAX_DEPTH);
            return alpha; // checkmate
        }
        add_entry(Board->key, NULL_MOVE, SCOREBOUND(0, PV_NODE), MAX_DEPTH);
        return 0; // stalemate
    }

    score_moves(Board, List, end, bestMove);
    partial_insertion_sort(List, end, INSERTION_SORT_MIN);

    int nodeType = ALL_NODE;
    while(1) {

        do_move(Board, cur);
        int score = -alpha_beta(Board, -beta, -alpha, depth - 1, rootPly, endTime);
        undo_move(Board);

        assert(abs(score) <= INF); // possible but unlikely for this condition to be broken. Can be fixed by hardcapping evaluation scores

        if(score >= beta) {
            add_entry(Board->key, cur->move, SCOREBOUND(beta, CUT_NODE), depth);
            return beta;
        }

        if(score > alpha) {
            alpha = score;
            nodeType = PV_NODE;
            bestMove = cur->move;
        }

        cur++;
        if(cur == end) break;

        if(clock() > endTime) {
            // This breaks all instantiations of alpha_beta, only saving nodes that raised alpha
            if(nodeType == PV_NODE) add_entry(Board->key, bestMove, SCOREBOUND(alpha, ALL_NODE), depth); // Important to save best PV child of root node
            return beta;
        }
    }

    add_entry(Board->key, bestMove, SCOREBOUND(alpha, nodeType), depth);
    return alpha;
}


void do_search(Board_s* const Board, int depth) {
    clock_t start, end;
    printf("    DEPTH  EVAL  TIME(s)  BEST\n");
    clock_t endTime = clock() + 999999*CLOCKS_PER_SEC;
    // clock_t endTime = -1;
    for(int i = 1; i <= depth; i++) {
        
        start = clock();
        alpha_beta(Board, -INF, INF, i, Board->hisPly, endTime);
        end = clock();
        double dt = (double)(end-start) / CLOCKS_PER_SEC;

        assert(TT[Board->key % TT_ENTRIES].key == Board->key >> 32);
        assert(TT[Board->key % TT_ENTRIES].move != NULL_MOVE);
        int score = SCORE(TT[Board->key % TT_ENTRIES].scoreBound);
        int trueEval = Board->side == WHITE ? score : -score;

        printf("   %2d ply  %4d  %7g  ", i, trueEval, dt);
        print_variation(Board, i);
        printf("\n");

    }
}

Move iterative_deepening(Board_s* const Board, double duration) {
    clock_t endTime = clock() + duration;

    // printf("DEPTH  BEST\n");
    for(int depth = 1; depth < MAX_DEPTH; depth++) {
        // init_tt();
        alpha_beta(Board, -INF, INF, depth, Board->hisPly, endTime);
        // if(clock() > endTime) break;
        // if(clock() > endTime) printf("\n\n");
        // printf("\033[F"); // Line clear
        // printf("%4d ply  ", depth);
        // print_variation(Board, depth);
        // printf("\n");
        if(clock() > endTime) break;
    }

    assert(TT[Board->key % TT_ENTRIES].key == Board->key >> 32);
    assert(TT[Board->key % TT_ENTRIES].move != NULL_MOVE);
    return TT[Board->key % TT_ENTRIES].move;
}