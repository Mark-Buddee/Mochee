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
#include "board.h"

int mock_quiesce(Board_s* const Board, int alpha, int beta) {

    int rootPly = SearchInfo.ply;

    // Fifty move rule
    if(Board->hundredPly == HUNDRED_PLIES) return 0;

    // 3-fold repetition
    if(is_three_fold(Board, rootPly)) return 0;

    int staticEval = Board->side == WHITE ? Board->staticEval : -Board->staticEval;

    Move_s List[MAX_MOVES];
    Move_s* cur = List;
    Move_s* end;

    if(Board->checkers) {
        end = gen_legal(Board, List);
        if(cur == end) return -INF + (Board->hisPly - rootPly); // checkmate

    } else {
        if(staticEval >= beta) return beta; // futility pruning
        if(staticEval > alpha) {
            alpha = staticEval;
        }
        end = gen_all(Board, List, Board->side, CAPTURES);
    }

    score_moves(Board, List, end, NULL_MOVE);
    partial_insertion_sort(List, end, INSERTION_SORT_MIN);

    while(cur != end) {

        // Delta pruning
        if(!Board->checkers) {
            int cpt = SPC(cur->move) == PROMOTION  ? QUEEN_VAL
                    : SPC(cur->move) == EN_PASSANT ? PAWN_VAL
                    : get_value(Board->pieces[DST(cur->move)]);

            assert(cpt);
            if(staticEval + cpt + DELTA_VAL < alpha) {
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
        }
        cur++;
    }

    return alpha;

}

int mock_alpha_beta(Board_s* const Board, int alpha, int beta, int depth, Move* rootBestMove) {
    
    assert(depth >= 0);
    
    int isRootNode = Board->hisPly == SearchInfo.ply;
    int pliesFromRoot = Board->hisPly - SearchInfo.ply;
    
    // Terminal nodes
    if(Board->hundredPly == HUNDRED_PLIES)      return 0; // Fifty move rule
    if(is_three_fold(Board, SearchInfo.ply))    return 0; // Threefold repetition
    // if(bits(Board->byColour[ALL]) <= 6)         return probe_tablebase(); // Tablebase probe
    if(depth == 0)                              return mock_quiesce(Board, alpha, beta); // Quiescence search
    
    // Probe transposition table
    Move ttBestMove = NULL_MOVE;
    int ttScoreBound = probe_TT(Board, depth, &ttBestMove);
    // assert(!ttBestMove || isLegalMove(Board, ttBestMove)); // very possible, but unlikely. If condition is met, move it inside (if is pv node and isrootnode) and wait until retrigger
    if(ttBestMove && !isLegalMove(Board, ttBestMove)) ttBestMove = NULL_MOVE;

    // Sufficient score found?
    if(isRootNode) *rootBestMove = ttBestMove;
    if(IS_PV_NODE(ttScoreBound)) return SCORE(ttScoreBound);
    if(IS_CUT_NODE(ttScoreBound) && SCORE(ttScoreBound) >= beta) return beta;
    if(IS_ALL_NODE(ttScoreBound) && SCORE(ttScoreBound) <= alpha) return alpha;

    // Generate moves
    Move_s List[MAX_MOVES];
    Move_s* cur = List;
    Move_s* end = gen_legal(Board, List);
    
    // Checkmate or stalemate
    if(cur == end) return Board->checkers ? -INF + pliesFromRoot : 0;

    if(isRootNode) {

        if(!*rootBestMove) *rootBestMove = cur->move; // ensure a legal move is returned in all cases

        if(end - cur == 1) return TIMEOUT; // if only one legal move, return immediately

    }
    
    // Score and sort moves
    score_moves(Board, List, end, ttBestMove);
    partial_insertion_sort(List, end, INSERTION_SORT_MIN);

    // Recursive search on children
    int nodeType = ALL_NODE;
    Move bestMove = NULL_MOVE;
    while(cur != end) {
        
        do_move(Board, cur);
        int score = -mock_alpha_beta(Board, -beta, -alpha, depth - 1, rootBestMove);
        assert(abs(score) <= TIMEOUT); // impossible condition, so long as static eval is clamped to [-INF, INF]
        undo_move(Board);
        
        // Timeout
        if(clock() >= SearchInfo.endtime || atomic_load(&SearchInfo.stop)) return TIMEOUT; // any value can be returned here, as it won't be used

        // Beta cutoff
        if(score >= beta) {

            // Save to TT as cut node
            add_entry(Board->key, cur->move, SCOREBOUND(score, CUT_NODE), depth, SearchInfo.ply);
            return beta;
        
        }

        // Raise alpha
        if(score > alpha) {

            bestMove = cur->move;
            nodeType = PV_NODE;
            alpha = score;

            if(isRootNode) *rootBestMove = cur->move;

        }

        cur++;

    }

    // Save to TT - could be PV node or ALL node
    add_entry(Board->key, bestMove, SCOREBOUND(alpha, nodeType), depth, SearchInfo.ply);
    return alpha;
    
}

Move mock_iterative_deepening(void) {
    
    Move bestMove = NULL_MOVE;
    clock_t starttime = clock();

    long previousIterationTime = 0;
    
    for(int depth = 1; depth <= MAX_DEPTH; depth++) {
        
        Move currentBestMove = NULL_MOVE;
        clock_t iterationStartTime = clock();
        int score = mock_alpha_beta(&Board, -INF, INF, depth, &currentBestMove);
        clock_t iterationEndTime = clock();
        assert(currentBestMove != NULL_MOVE);
        
        if(clock() >= SearchInfo.endtime || atomic_load(&SearchInfo.stop)) {
            
            if(depth == 1) {
                
                fprintf(stderr, "Warning: Insufficient time to complete search\n");
                bestMove = currentBestMove;
                
            }
            
            assert(bestMove != NULL_MOVE);
            
            break;
            
        }
        
        // Output info for current iteration
        printf("info depth %d score cp %d time %ld pv ", depth, score, (long)(clock() - starttime) * 1000 / CLOCKS_PER_SEC);
        print_move(currentBestMove);
        printf("\n");
        
        bestMove = currentBestMove;
        
        // If insufficient remaining time for next iteration, break. No point wasting time on an unfinished search, when we already have a move to play.
        long iterationTime = (long)(iterationEndTime - iterationStartTime) * 1000 / CLOCKS_PER_SEC;
        long remainingTime = (long)(SearchInfo.endtime - iterationEndTime) * 1000 / CLOCKS_PER_SEC;
        printf("ply %d, depth %d, iteration time %ld ms, current/previous %.2f\n", SearchInfo.ply, depth, iterationTime, depth > 1 ? (double)iterationTime / (previousIterationTime ? previousIterationTime : 1) : 1);
        if(remainingTime < iterationTime *5.5) break; // not enough time for next iteration
        previousIterationTime = iterationTime;

    }

    return bestMove;

}


int thread_func(void* arg) {

    (void)arg;

    // Move bestMove = bits(Board.byColour[ALL]) <= 6 ? probe_tablebase() : mock_iterative_deepening();
    Move bestMove = mock_iterative_deepening();

    // Output best move
    printf("bestmove ");
    print_move(bestMove);
    printf("\n");

    return 0;

}
