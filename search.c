#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "inc/defs.h"
#include "inc/move.h"
#include "inc/eval.h"
#include "inc/gen.h"
#include "inc/search.h"
#include "inc/tgui.h"

int nega_max(Board_s* const Board, int depth) {
    int side = Board->side;
    Move_s List[MAX_GAME_PLYS];
    Move_s* cur = List;
    Move_s* end = gen_legal(Board, List);

    int eval = side == WHITE ? -INF : INF;
    while(cur != end) {
        int newEval;
        if(depth == 1) {
            newEval = Board->staticEval + move_eval(Board, cur->move);
        } else {
            do_move(Board, cur->move);
            newEval = nega_max(Board, depth - 1);
            undo_move(Board);
        }
        if((side == WHITE && (newEval > eval)) || (side == BLACK && (newEval < eval)))
            eval = newEval;
        cur++;
    }
    return eval;
}

int alpha_beta(Board_s* const Board, int alpha, int beta, int depth, const int maxDepth, int* bestMove) {
    // if(depth == 0) {
    //     int trueEval = Board->staticEval;
    //     return Board->side == WHITE ? trueEval : -trueEval;
    // }
    Move_s List[MAX_MOVES];
    Move_s* cur = List;
    Move_s* end = gen_legal(Board, List);
    while(cur != end) {
        int eval;
        if(depth == 1) {
            int trueEval = Board->staticEval + move_eval(Board, cur->move);
            eval = Board->side == WHITE ? trueEval : -trueEval;
        } else {
            do_move(Board, cur->move);
            eval = -alpha_beta(Board, -beta, -alpha, depth - 1, maxDepth, bestMove);
            undo_move(Board);
        }
        if(eval >= beta) return beta;
        if(eval > alpha) {
            alpha = eval;
            if(depth == maxDepth) *bestMove = cur->move;
        }
        cur++;
    }
    return alpha;
}

void do_search(Board_s* const Board, int depth) {
    clock_t start, end;
    printf("    DEPTH  EVAL  TIME(s)  BEST\n");
    int bestMove = NULL_MOVE;
    for(int i = 1; i <= depth; i++) {
        start = clock();
        // int eval = nega_max(Board, i);
        int eval = alpha_beta(Board, -INF, INF, i, i, &bestMove);
        // int eval = alpha_beta(Board, -INF, INF, i);
        int trueEval = Board->side == WHITE ? eval : -eval;
        end = clock();
        
        double dt = (double)(end-start) / CLOCKS_PER_SEC;
        char bestSrc[3];
        char bestDst[3];
        src2str(bestSrc, SRC(bestMove));
        src2str(bestDst, DST(bestMove));
        printf("   %2d ply  %4d  %7g  %s%s\n", i, trueEval, dt, bestSrc, bestDst);
    }
    // printf("\n");
}