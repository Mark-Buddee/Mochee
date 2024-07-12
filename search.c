#include <stdlib.h>
#include <stdio.h>
#include <time.h>
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

int clamp(int lower, int upper, int score) {
    return score <= lower ? lower
         : score >= upper ? upper
         :                  score;
}

// Scores moves based on which are most likely to be good
void score_moves(Board_s* Board, Move_s* cur, Move_s* end, Move bestMove) {
    while(cur != end) {
        int src = SRC(cur->move);
        int dst = DST(cur->move);
        // int ppt = PPT(cur->move);
        int spc = SPC(cur->move);
        int cpt  = Board->pieces[dst];
        int mvd  = Board->pieces[src];

        // int moveVal = move_eval(Board, cur->move);
        int moveVal = 0;

        if(cur->move == bestMove) moveVal += TABLE_BEST_BIAS;
        if(cpt) {
            moveVal += cpt > mvd ? WINNING_CAPTURE_BIAS : LOSING_CAPTURE_BIAS; // This code is broken on promotions
            moveVal += cpt; // encourage capture of heavy pieces
            moveVal -= mvd; // encourage cheapest piece making the capture
        }
        if(spc == PROMOTION) moveVal += PROMOTION_BIAS;
        
        cur->score = moveVal;

        cur++;
    }
}

void partial_insertion_sort(Move_s* begin, Move_s* end, int limit) {

    // Copied from stockfish
    for (Move_s *sortedEnd = begin, *p = begin + 1; p < end; ++p)
        if (p->score >= limit) {
            Move_s tmp = *p, *q;
            *p = *++sortedEnd;
            for (q = sortedEnd; q != begin && (q - 1)->score < tmp.score; --q)
                *q = *(q - 1);
            *q = tmp;
        }
}

int quiesce(Board_s* const Board, int alpha, int beta) {
    Move bestMove = NULL_MOVE;
    Move_s List[MAX_MOVES];
    Move_s* cur = List;
    Move_s* end;

    int staticEval = Board->side == WHITE ? Board->staticEval : -Board->staticEval;

    if(Board->checkers) {
        end = gen_all(Board, List, Board->side, EVASIONS);
        if(cur == end) return alpha; // checkmate

    } else {
        if(staticEval >= beta) return beta;
        if(staticEval > alpha) alpha = staticEval;
        end = gen_all(Board, List, Board->side, CAPTURES);
    }

    score_moves(Board, List, end, bestMove);
    partial_insertion_sort(cur, end, INSERTION_SORT_MIN);

    while(cur != end) {

        if(!Board->checkers) {
            int cpt = SPC(cur->move) == PROMOTION  ? QUEEN_VAL
                    : SPC(cur->move) == EN_PASSANT ? PAWN_VAL
                    : get_value(Board->pieces[DST(cur->move)]);

            // assert(cpt);
            if(staticEval + cpt < alpha - DELTA_VAL) {
                cur++;
                continue;
            }
        }

        do_move(Board, cur->move);
        int score = -quiesce(Board, -beta, -alpha);
        undo_move(Board);

        if(score >= beta) return beta;
        if(score > alpha) alpha = score;
        cur++;
    }
    return alpha;
}

int alpha_beta(Board_s* const Board, int alpha, int beta, int depth, clock_t endTime) {
    
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
    if(depth == 0) return quiesce(Board, alpha, beta);
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
    partial_insertion_sort(cur, end, INSERTION_SORT_MIN);

    int nodeType = ALL_NODE;
    while(1) {

        do_move(Board, cur->move);
        int score = -alpha_beta(Board, -beta, -alpha, depth - 1, endTime);
        assert(abs(score) <= INF); // possible but unlikely for this condition to be broken
        undo_move(Board);

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
            if(nodeType == PV_NODE) add_entry(Board->key, bestMove, SCOREBOUND(alpha, ALL_NODE), depth); // Important to save best PV child of root node
            return beta; // this breaks all instantiations of alpha_beta without saving anything except for nodes that raised alpha
        }
    }

    add_entry(Board->key, bestMove, SCOREBOUND(alpha, nodeType), depth);
    return alpha;
}


void do_search(Board_s* const Board, int depth) {
    clock_t start, end;
    printf("    DEPTH  EVAL  TIME(s)  BEST\n");
    clock_t endTime = clock() + 999999*CLOCKS_PER_SEC;
    for(int i = 1; i <= depth; i++) {
        start = clock();
        int score = alpha_beta(Board, -INF, INF, i, endTime);
        end = clock();
        
        double dt = (double)(end-start) / CLOCKS_PER_SEC;

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
        alpha_beta(Board, -INF, INF, depth, endTime);
        // if(clock() > endTime) break;
        // if(clock() > endTime) printf("\n\n");
        // printf("\033[F"); // Line clear
        // printf("%4d ply  ", depth);
        // print_variation(Board, depth);
        // printf("\n");
        if(clock() > endTime) break;
    }

    assert(TT[Board->key % TT_ENTRIES].key == Board->key >> 32);
    return TT[Board->key % TT_ENTRIES].move;
}