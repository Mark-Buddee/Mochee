#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "inc/defs.h"
#include "inc/board.h"
#include "inc/gen.h"
#include "inc/move.h"
#include "inc/perft.h"
#include "inc/tgui.h"

unsigned long long num_nodes(Board_s* const Board, int depth) {
    // if(depth == 0) return 0;
    Move_s List[MAX_MOVES];
    Move_s* cur = List;
    Move_s* end = gen_legal(Board, List);
    if(depth == 1) return end - List;
    unsigned long long numMoves = 0;
    while(cur != end) {
        do_move(Board, cur->move);
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
    FILE* fp = fopen("perft.csv", "r");
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
        U64 originalKey = Board.key;
        unsigned long long num = num_nodes(&Board, depth);
        end = clock();
        assert(originalKey == Board.key);
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
