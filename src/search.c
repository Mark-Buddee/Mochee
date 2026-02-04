#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include "math.h"
#include "defs.h"
#include "move.h"
#include "eval.h"
#include "gen.h"
#include "console.h"
#include "tt.h"
#include "board.h"
#include "bitboard.h"
#include "search.h"
#include "debug.h"

extern unsigned long long tableHits;
extern unsigned long long tableUpdates;
extern unsigned long long tableOverwrites;
extern unsigned long long tableOverwriteDepthSum;

int is_three_fold(Board_s* Board, int rootPly) {

    // Repeition is possible only after at least 4 plies
    // Only look 4, 6, 8, ... plies in the past where the player to move was the same as right now
    // until the last irreversible move (pawn move or capture)
    int posFreq = 0;
    for(int curPly = Board->hisPly - 4; curPly >= Board->hisPly - Board->hundredPly; curPly -= 2) {

        int inSearch = curPly > rootPly; // are we currently searching this position?
        // int inSearch = 0;
        int keyMatch = Board->Undos[curPly].key == Board->key; // does the position match?

        if(keyMatch && ++posFreq + inSearch == 2) return 1; // found a two-fold repetition in the search or a three-fold overall

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
    // Runs slightly slower because of the repeated positionScore  + orderingBias sum

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
        if(cur == end) return -INF + (Board->hisPly - rootPly); // checkmate

    } else {
        if(staticEval >= beta) return beta; // Futility pruning
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
            raisedAlpha = 1;
        }
        cur++;
    }

    // If alpha has been raised, or if all moves have been searched, there is no need to keep searching
    // if(raisedAlpha || Board->checkers) return alpha; // Board->hisPly - startPly < MAX_QUIET_CHECK_PLIES

    // No capture suggests raising alpha, no checks, this is a truly quiet position
    // if(Board->hisPly > 16) print_detailed(Board, Board->side);
    return alpha;

    // // Fix the following so the comment is not true!
    // // We've already checked that staticEval <= alpha
    // // if(staticEval < alpha) return alpha;
    // // if(staticEval < alpha - QUIET_DELTA_VAL) return alpha; // Removing this condition creates segfault - endless checks are searched.
    // // assert(staticEval == alpha);


    // // If no captures raised alpha, we investigate quiet checks
    // // printf("activate\n");
    // cur = List;
    // end = gen_all(Board, List, Board->side, QUIET_CHECKS);
    // score_moves(Board, List, end, bestMove);
    // partial_insertion_sort(List, end, INSERTION_SORT_MIN);

    // while(cur != end) {

    //     if(cur->positionScore <= 0) { // Only allow quiet checks that IMPROVE the position
    //         cur++;
    //         continue;
    //     }

    //     do_move(Board, cur);
    //     int score = -quiesce(Board, -beta, -alpha, rootPly);
    //     undo_move(Board);

    //     if(score >= beta) return beta;
    //     if(score > alpha) alpha = score;
    //     cur++;
    // }

    // return alpha;
}

// TODO: change return type to int16_t
int alpha_beta(Board_s* const Board, int alpha, int beta, int depth, int rootPly, clock_t endTime, Move* rootBestMove) {

    #ifndef NDEBUG
        nodesSearched++;
    #endif

    // TODO: Fix all return values
    // TODO: Fix all calls to alpha_beta
    
    // Fifty move rule
    if(Board->hundredPly == HUNDRED_PLIES) return 0;
    
    // 3-fold repetition
    if(is_three_fold(Board, rootPly)) return 0;

    Move bestMove = NULL_MOVE;
    int isRootNode = Board->hisPly == rootPly;
    
    TTEntry_s Entry = TT[Board->key % TTEntries];
    if(Entry.key == KEY_TOP(Board->key)) {

        // int foundMate = IS_PV_NODE(Entry.scoreBound) && abs(SCORE(Entry.scoreBound)) >= INF - MAX_DEPTH;
        if(Entry.depth >= depth) {
        // if(Entry.depth >= depth || foundMate) { // sufficient depth or mate score is valid

            #ifndef NDEBUG
                TTStats.hits++;
            #endif

            // if(isRootNode) assert(Entry.move != NULL_MOVE);

            if(IS_PV_NODE(Entry.scoreBound)) {
                assert(!isRootNode || Entry.move != NULL_MOVE);
                if (isRootNode) *rootBestMove = Entry.move;
                if(!isRootNode && (SCORE(Entry.scoreBound) < beta || SCORE(Entry.scoreBound) > alpha)) {
                    return SCORE(Entry.scoreBound); // exact score
                }
            } else if(IS_CUT_NODE(Entry.scoreBound) && SCORE(Entry.scoreBound) >= beta) {
                assert(!isRootNode || Entry.move != NULL_MOVE);
                assert(!isRootNode);
                // if(isRootNode) *rootBestMove = Entry.move;
                return beta; // lower bound exceeds beta
            } else if(IS_ALL_NODE(Entry.scoreBound) && SCORE(Entry.scoreBound) <= alpha) {
                assert(!isRootNode || Entry.move != NULL_MOVE);
                assert(!isRootNode);
                // if(isRootNode) *rootBestMove = Entry.move;
                return alpha; // upper bound is below alpha
            }

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

    // if(cur == end) return Board->checkers ? -INF + (Board->hisPly - rootPly) : 0;
    if(cur == end) {
        if(Board->checkers) {
            add_entry(Board->key, NULL_MOVE, SCOREBOUND(-INF + (Board->hisPly - rootPly), PV_NODE), 0, rootPly); // This is probably a waste of time
            return -INF + (Board->hisPly - rootPly); // checkmate
        }
        add_entry(Board->key, NULL_MOVE, SCOREBOUND(0, PV_NODE), 0, rootPly); // This is probably a waste of time
        return 0; // stalemate
    }

    // If only one legal move, no need to search further
    if(isRootNode && (end - cur) == 1) {
        add_entry(Board->key, cur->move, SCOREBOUND(0, PV_NODE), 0, rootPly); // This is probably a waste of time
        *rootBestMove = cur->move;
        return 0; // TODO: Evaluate properly
    }

    score_moves(Board, List, end, bestMove);
    partial_insertion_sort(List, end, INSERTION_SORT_MIN);
    
    // in case all moves fail to improve alpha (checkmate against us)
    if(bestMove == NULL_MOVE) bestMove = cur->move;
    // or insufficient time to complete search
    if(isRootNode && *rootBestMove == NULL_MOVE) *rootBestMove = bestMove;
    
    int nodeType = ALL_NODE;
    while(1) {

        do_move(Board, cur);
        int score = -alpha_beta(Board, -beta, -alpha, depth - 1, rootPly, endTime, rootBestMove);
        undo_move(Board);

        assert(abs(score) <= INF); // possible but unlikely for this condition to be broken. Can be fixed by hardcapping evaluation scores

        // Beta cutoff
        if(score >= beta) {
            if(isRootNode) return beta;
            if(isRootNode) *rootBestMove = cur->move;
            add_entry(Board->key, cur->move, SCOREBOUND(beta, CUT_NODE), depth, rootPly);
            return beta;
        }

        // Raise alpha
        if(score > alpha) {

            alpha = score;
            nodeType = PV_NODE;
            bestMove = cur->move;

            if(isRootNode) *rootBestMove = bestMove;

        }

        cur++;
        if(cur == end) break; // all children searched

        // Timeout
        // if(clock() > endTime && (depth > 1 || *rootBestMove != NULL_MOVE)) {
        if(clock() >= endTime) {

            assert(*rootBestMove != NULL_MOVE); // TODO: This instead of the above second half of condition
            assert(bestMove != NULL_MOVE);

            if(nodeType == PV_NODE) add_entry(Board->key, bestMove, SCOREBOUND(alpha, CUT_NODE), depth, rootPly); // save lower bounds where possible
            return beta; // don't searching siblings
        
        }
    }

    if(isRootNode && nodeType != PV_NODE) return alpha; // Fail low
    add_entry(Board->key, bestMove, SCOREBOUND(alpha, nodeType), depth, rootPly);
    return alpha;
}


void do_search(Board_s* const Board, int depth) {
    clock_t start, end;
    printf("    DEPTH  EVAL  TIME(s)  BEST\n");
    clock_t endTime = clock() + 999999*CLOCKS_PER_SEC;
    // clock_t endTime = -1;
    for(int i = 1; i <= depth; i++) {
        Move bestMove;
        
        start = clock();
        alpha_beta(Board, -INF, INF, i, Board->hisPly, endTime, &bestMove);
        end = clock();
        double dt = (double)(end-start) / CLOCKS_PER_SEC;

        assert(TT[Board->key % TTEntries].key == Board->key >> 48);
        assert(TT[Board->key % TTEntries].move != NULL_MOVE);
        int score = SCORE(TT[Board->key % TTEntries].scoreBound);
        int trueEval = Board->side == WHITE ? score : -score;

        printf("   %2d ply  %4d  %7g  ", i, trueEval, dt);
        print_variation(Board, i);
        printf("\n");

    }
}

Move iterative_deepening(Board_s* const Board, double maxDuration) {

    clock_t endTime = clock() + maxDuration;

    Move bestMove = NULL_MOVE;
    for(int depth = 1; depth < MAX_DEPTH; depth++) {

        Move currentBestMove = NULL_MOVE;
        
        clock_t iterationStartTime = clock();
        clock_t iterationEndTime;
        

        // int bounds[2];
        // get_aspiration_window(prevScores, depth, &bounds[LOWER], &bounds[UPPER]);
        
        int score = -INF;
        // int reps = 0;
        while(1) {

            // printf("Aspiration window for depth %d: [%d, %d]\n", depth, bounds[LOWER], bounds[UPPER]);
            // currentBestMove = NULL_MOVE;
            // score = alpha_beta(Board, bounds[LOWER], bounds[UPPER], depth, Board->hisPly, endTime, &currentBestMove);
            score = alpha_beta(Board, -INF, INF, depth, Board->hisPly, endTime, &currentBestMove);
            iterationEndTime = clock();

            // assert(score >= bounds[LOWER] && score <= bounds[UPPER]);

            // if(iterationEndTime >= endTime || (score > bounds[LOWER] && score < bounds[UPPER])) {
            if(iterationEndTime >= endTime) {
                break; // score is within aspiration window or time's up
            }

            // reps++;
            // printf("Aspiration window failed at depth %d with score %d (bounds [%d, %d]), retrying (%d)\n", depth, score, bounds[LOWER], bounds[UPPER], reps);
            // if(reps >= 4) {
            //     // Give up on aspiration windows
            //     bounds[LOWER] = -INF;
            //     bounds[UPPER] = INF;
            //     continue;
            // }

            // int mean = (bounds[LOWER] + bounds[UPPER]) / 2;
            // printf("lower %d, mean %d, upper %d \n", bounds[LOWER], mean, bounds[UPPER]);
            // if(score == bounds[LOWER]) bounds[LOWER] = mean - (mean - bounds[LOWER]) * 4;
            // else if(score == bounds[UPPER]) bounds[UPPER] = mean + (bounds[UPPER] - mean) * 4;

        }
        // prevScores[depth] = score;

        #ifndef NDEBUG
            printf("depth: %2d, score: %3d, iterationDuration: %4g s, bestMove: ", depth, score, (double)(iterationEndTime - iterationStartTime) / CLOCKS_PER_SEC);
            print_move(currentBestMove);
            printf(" Variation: ");
            print_variation(Board, depth);
            printf("\n");
        #endif

        
        if(iterationEndTime >= endTime) {

            if(depth == 1) bestMove = currentBestMove; // ensure at least depth 1 is searched
            
            #ifndef NDEBUG
                printf("Time's up!\n");
            #endif
            if(bestMove == NULL_MOVE) printf("Error: bestMove is NULL_MOVE! 1 \n");
            assert(bestMove != NULL_MOVE);

            break;
        }

        bestMove = currentBestMove;
        
        if(abs(score) >= INF - MAX_DEPTH) {
            #ifndef NDEBUG
                printf("Mate found!\n");
            #endif
            if(bestMove == NULL_MOVE) printf("Error: bestMove is NULL_MOVE! 2 \n");
            assert(bestMove != NULL_MOVE);

            break;
        }
        
        double remainingTime = (double)(endTime - iterationEndTime) / CLOCKS_PER_SEC;
        double iterationDuration = (double)(iterationEndTime - iterationStartTime) / CLOCKS_PER_SEC;

        // There may be still a deep enough result in the TT! ignore the following code then.
        // if(depth >= 6 && iterationDuration == 0 && remainingTime < pow(3.9, depth-8)) {
        //     #ifndef NDEBUG
        //     printf("Insufficient time (lots of table hits)\n");
        //     #endif
        //     break; // We've had lots of table hits, but not enough time for next iteration
        // }

        if(remainingTime < iterationDuration * 5.5) {
            #ifndef NDEBUG
                printf("Insufficient time (prediction)\n");
            #endif
            if(bestMove == NULL_MOVE) printf("Error: bestMove is NULL_MOVE! 3 \n");
            break; // not enough time for next iteration
        }

    }

    return bestMove;

}
