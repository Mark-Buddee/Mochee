#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "defs.h"
#include "eval.h"
#include "board.h"
#include "gen.h"
#include "move.h"
#include "perft.h"
#include "console.h"
#include "search.h"
#include "worker.h"
#include "debug.h"

unsigned long long num_nodes(Board_s* const Board, int depth) {

    // if(depth == 0) return 0;

    Move_s List[MAX_MOVES];
    Move_s* cur = List;
    Move_s* end = gen_legal(Board, List);
    score_moves(Board, List, end, NULL_MOVE);

    if(depth == 1) return end - List;

    unsigned long long numMoves = 0;
    while(cur != end) {
        do_move(Board, cur);
        numMoves += num_nodes(Board, depth - 1);
        undo_move(Board);
        cur++;
    }
    return numMoves;
}

void perft(Board_s* const Board, const int depth) {
    clock_t start, end;
    start = clock();
    for(int i = 1; i <= depth; i++) {
        unsigned long long num = num_nodes(Board, i);
        end = clock();
        double dt = (double)(end - start) / CLOCKS_PER_SEC;
        // printf("depth %d ply: %11llu nodes in unknown seconds\n", i, num);
        printf("depth %d ply: %11llu nodes in %.3g seconds\n", i, num, dt); // TODO: change to 6.3g
    } 
}

void perft_unit_test(void) {
    FILE* fp = fopen("test/perft.csv", "r");
    char line[STREAM_BUFF_SIZE];
    printf("PERFT UNIT TEST\n");
    printf("    PHASE    TEST TYPE  DEPTH  NODES        RESULT\n");

    int failed = false;
    double totalTime = 0;
    while(!feof(fp)) {
        fgets(line, STREAM_BUFF_SIZE, fp);

        // Parse
        char* fen = strtok(line, ",");
        int depth = atoi(strtok(NULL, ","));
        unsigned long long actualNum = atoi(strtok(NULL, ","));
        char* phase = strtok(NULL, ",");
        char* testType = strtok(NULL, ",");

        // Complete test
        printf("    %7s  %-9s  %d ply  %9llu    ", phase, testType, depth, actualNum);
        clock_t start, end;
        start = clock();
        Board_s Board = board_init(fen);
        // U64 originalKey = Board.key;
        unsigned long long num = num_nodes(&Board, depth);
        end = clock();
        // assert(originalKey == Board.key);
        double dt = (double)(end - start) / CLOCKS_PER_SEC;
        totalTime += dt;

        // Display result
        if(num == actualNum) {
            printf("\033[92mPASSED\033[0m in %4.4g seconds\n", dt);
        } else {
            printf("\033[91mFAILED\033[0m\n");
            failed = true;
        }
    }
    printf("\n    SUMMARY: ");
    failed ? printf("\033[91mFAIL\033[0m ") : printf("\033[92mPASS\033[0m ");
    printf("in %4.4g total seconds\n\n", totalTime);
    fclose(fp);
}

int bench_iterative_deepening(Board_s* const Board, int d, Move* rootBestMove) {

    int score = TIMEOUT;
    Move bestMove = NULL_MOVE;
    
    // Iterative deepening
    for(int depth = 1; depth <= d; depth++) {
        
        Move currentBestMove = NULL_MOVE;
        int currentScore = alpha_beta(Board, -INF, INF, depth, &currentBestMove);

        assert(currentBestMove != NULL_MOVE);

        if(clock() >= SearchInfo.endtime || atomic_load(&SearchInfo.stop)) {

            if(depth == 1) {
                bestMove = currentBestMove;
                score = currentScore;
            }

            assert(bestMove != NULL_MOVE);
            
            break;

        }
        
        bestMove = currentBestMove;
        score = currentScore;

    }

    *rootBestMove = bestMove;
    return score;

}

void bench(void) {

    FILE* fp = fopen("test/perft.csv", "r");
    if(!fp) {
        perror("test/perft.csv");
        return;
    }

    char line[STREAM_BUFF_SIZE];
    printf("BENCHMARK\n");
    printf("     %7s   %-9s  %7s %6s  %6s  %10s %10s\n", "PHASE", "TEST TYPE", "TIME", "SCORE", "BEST", "TRUE_SCORE", "TRUE_BEST");

    while(fgets(line, STREAM_BUFF_SIZE, fp)) {
        if(line[0] == '\n' || line[0] == '\0') continue;

        char* fen = strtok(line, ",");
        (void)strtok(NULL, ",");
        (void)strtok(NULL, ",");
        char* phase = strtok(NULL, ",");
        char* testType = strtok(NULL, ",");
        char* trueScore = strtok(NULL, ",");
        char* trueBest = strtok(NULL, ",\n");

        if(!fen || !phase || !testType) continue;

        Board_s Board = board_init(fen);

        SearchInfo.ply = Board.hisPly;
        atomic_store(&SearchInfo.stop, false);
        int searchtime = 500; // 500 milliseconds
        SearchInfo.endtime = clock() + searchtime * CLOCKS_PER_SEC / 1000;

        Move rootBestMove = NULL_MOVE;

        clock_t start = clock();
        int score = bench_iterative_deepening(&Board, MAX_DEPTH, &rootBestMove);
        clock_t end = clock();
        double dt = (double)(end - start) / CLOCKS_PER_SEC;

        printf("    %7s  %-9s   %5.1gs %6d    ", phase, testType, dt, score);
        print_move(rootBestMove);
        printf("%6s  %10s \n", trueScore, trueBest);
    }

    fclose(fp);
}
