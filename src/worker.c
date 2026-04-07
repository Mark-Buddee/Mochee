#include <stdio.h>
#include <time.h>
#include <assert.h>
#include "defs.h"
#include "eval.h"
#include "tt.h"
#include "debug.h"
#include "search.h"
#include "gen.h"
#include "move.h"

int mock_alpha_beta(Board_s* const Board, int alpha, int beta, int depth, Move* rootBestMove) {
    
    assert(depth >= 0);
    
    int isRootNode = Board->hisPly == SearchInfo.ply;
    int pliesFromRoot = Board->hisPly - SearchInfo.ply;

    // Terminal node
    if(depth == 0) return Board->side == WHITE ? Board->staticEval : -Board->staticEval;
    // if(depth == 0) return quiesce(Board, alpha, beta, SearchInfo.ply);

    // Fifty move rule
    if(Board->hundredPly == HUNDRED_PLIES) return 0;
    
    // 3-fold repetition
    if(is_three_fold(Board, SearchInfo.ply)) return 0;    

    // Generate moves
    Move_s List[MAX_MOVES];
    Move_s* cur = List;
    Move_s* end = gen_legal(Board, List);
    
    // Checkmate or stalemate
    if(cur == end) return Board->checkers ? -INF + pliesFromRoot : 0;

    // Ensure a legal move is returned even if insufficient time to complete search
    if(isRootNode) {

        *rootBestMove = cur->move;

        if(end - cur == 1) return TIMEOUT; // if only one legal move, return immediately

    }
        
    score_moves(Board, List, end, NULL_MOVE);
    // partial_insertion_sort(List, end, INSERTION_SORT_MIN);

    while(cur != end) {
        
        // Recursive search
        do_move(Board, cur);
        int score = -mock_alpha_beta(Board, -beta, -alpha, depth - 1, rootBestMove);
        undo_move(Board);

        assert(abs(score) <= INF); // impossible condition, so long as static eval is clamped to [-INF, INF]
        
        // Timeout
        if(clock() >= SearchInfo.endtime) return TIMEOUT; // any value can be returned here, as it won't be used

        // Beta cutoff
        if(score >= beta) return beta;

        // Raise alpha
        if(score > alpha) {

            alpha = score;

            if(isRootNode) *rootBestMove = cur->move;

        }

        cur++;

    }

    return alpha;

}


int thread_func(void* arg) {

    (void)arg;

    Move bestMove = NULL_MOVE;
    
    // Iterative deepening
    clock_t starttime = clock();
    for(int depth = 1; depth <= MAX_DEPTH; depth++) {
        
        // assert(!endofgame);
        
        Move currentBestMove = NULL_MOVE;
        int score = mock_alpha_beta(&Board, -INF, INF, depth, &currentBestMove);

        assert(currentBestMove != NULL_MOVE);

        if(atomic_load(&SearchInfo.stop) || clock() >= SearchInfo.endtime) {

            if(depth == 1) bestMove = currentBestMove;

            assert(bestMove != NULL_MOVE);
            
            break;

        }
        
        // Output info for current iteration
        printf("info depth %d score cp %d time %ld pv ", depth, score, (long)(clock() - starttime) * 1000 / CLOCKS_PER_SEC);
        print_move(currentBestMove);
        printf("\n");
        
        bestMove = currentBestMove;

    }

    // Output best move
    printf("bestmove ");
    print_move(bestMove);
    printf("\n");

    return 0;

}
