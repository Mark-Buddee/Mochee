#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <assert.h>
#include <time.h>
#include "defs.h"
#include "console.h"
#include "board.h"
#include "gen.h"
#include "move.h"
#include "perft.h"
#include "search.h"
#include "eval.h"
#include "tt.h"
#include "magic.h"
#include "init.h"
#include "debug.h"

int str2src(char string[]) {
    // TODO: could add guard rails here
    int file = string[0] - 'a';
    int rank = atoi(++string) - 1;
    return FR(file, rank);
}

void src2str(char srcStr[], int src) {
    srcStr[0] = FILE(src) + 'a';
    srcStr[1] = RANK(src) + '1';
    srcStr[2] = '\0';
}

int get_ppt(char string[]) {
    string++;
    string++;
    string++;
    string++;

    switch(string[0]) {
        case 'n':
            return KNIGHT - KNIGHT;
        case 'b':
            return BISHOP - KNIGHT;
        case 'r':
            return ROOK - KNIGHT;
        case 'q':
            return QUEEN - KNIGHT;
    }
    return 0;
}

static void handle_reset(int flipped){
    init_tt();
    Board = board_init(START_FEN);
    print_board(&Board, flipped);
}

static void handle_help() {
    printf(
        "reset\n"
        "print\n"
        "board\n"
        "undo\n"
        "test\n"
        "flip\n"
        "end\n"
        "moves <depth>\n"
        "perft <depth>\n"
        "eval  <depth>\n"
        "play  <time (ms)>\n"
        "fen   <fen>\n"
        "<move>\n");
}

static void handle_test() {
    perft_unit_test();
}

static void handle_print(int flipped) {
    print_board(&Board, flipped);
}

static void handle_board(int flipped) {
    print_detailed(&Board, flipped);
}

static void handle_undo(int flipped) {
    // TODO: Don't allow undos beyond starting fen
    undo_move(&Board);
    inc_age();
    // dec_age();
    print_board(&Board, flipped);
}

static void handle_flip(int* flipped) {
    *flipped = !(*flipped);
    print_board(&Board, *flipped);
}

static void handle_moves(char* line) {

    int depth = atoi(line);
    if(depth < 1) depth = 1;

    Move_s List[MAX_MOVES];
    Move_s* cur = List;
    // Move_s* end = gen_all(&Board, List, Board.side, CAPTURES);
    Move_s* end = gen_legal(&Board, List);
    score_moves(&Board, List, end, NULL_MOVE);

    long long unsigned totalNodes = 0;
    while(cur != end) {
        do_move(&Board, cur);
        unsigned long long numNodes = num_nodes(&Board, depth);
        totalNodes += numNodes;
        print_move(cur->move);
        printf(" %llu\n", numNodes);
        // printf("\n");
        undo_move(&Board);
        cur++;
    }
    printf("\ntotal: %llu\n", totalNodes);
}

static void handle_perft(char* line) {

    int depth = atoi(line);
    if(depth < 1) depth = 1;
    
    perft(&Board, depth);
}

static void handle_eval(char* line) {

    int depth = atoi(line);
    if(depth < 1) depth = 1;

    do_search(&Board, depth);
}

static void handle_play(char* line, int flipped){

    double duration = atof(line);
    clock_t start, end;

    start = clock();
    Move bestMove = iterative_deepening(&Board, duration);
    end = clock();
    double dt = (double)(end - start) / CLOCKS_PER_SEC;

    Move_s cur = {.move = bestMove, .positionScore = move_position_eval(&Board, bestMove), .materialScore = move_material_eval(&Board, bestMove), .orderingBias = 0};
    do_move(&Board, &cur);

    // printf("doing %s%s\n", bestSrc, bestDst);
    // assert(TT[Board.key % TT_ENTRIES].key == Board.key >> 32);
    int eval = SCORE(TT[Board.key % TT_ENTRIES].scoreBound); // TODO: Explicitly cast this to int16_t 
    int trueEval = Board.side == WHITE ? eval : -eval;
    // printf("%d, %d, %d\n", TT[Board.key % TT_ENTRIES].scoreBound, eval, trueEval);
    printf("\nEval: %d bestMove: ", trueEval);
    print_move(bestMove);
    printf(" time:%7g\n", dt);
    print_board(&Board, flipped);
    inc_age(); // Important to be done after iterative deepening

}

static void handle_fen(char* line, int flipped) {
    init_tt();
    Board = board_init(line);
    print_board(&Board, flipped);
}

static void handle_move(char* line, int flipped) {
    int ppt = get_ppt(line);
    int src = str2src(line++);
    int dst = str2src(++line);

    Move_s List[MAX_MOVES];
    Move_s* cur = List;
    Move_s* end = gen_legal(&Board, List);
    score_moves(&Board, List, end, NULL_MOVE);

    Move_s* MoveToMake = NULL; 
    while(cur != end) {
        // printf("PPT: %d\n", PPT(cur->move));
            if(SRC(cur->move) == src && DST(cur->move) == dst && PPT(cur->move) == ppt) {
            MoveToMake = cur;
            // MoveSToMake->moveVal = move_eval(&Board, MoveSToMake->move);
            break;
        }
        cur++;
    }

    if(MoveToMake != NULL) {
        do_move(&Board, MoveToMake);
        inc_age();
        print_board(&Board, flipped);
    } else
        printf("Move not found\n");
}

void console(void) {

    printf("Entering console mode...\n");

    init_all();
    Board = board_init(START_FEN);
    int flipped = Board.side;

    printf("Type 'help' for a list of commands\n\n");
    print_board(&Board, flipped);
    printf("\n");

    char line[BUFF_SIZE];

    while(1) {

        // Detect checkmate
        Move_s List[MAX_MOVES];
        if(gen_legal(&Board, List) == List) {
            if(Board.checkers) printf("Checkmate!\n");
            else printf("Stalemate!");
        }

        if (!fgets(line, STREAM_BUFF_SIZE, stdin))  continue;

        line[strcspn(line, "\n")] = ' '; // removes newline character from end
        char* tok = strtok(line, " ");
        assert(tok != NULL);

        if      (!strcmp(line, "help"))  handle_help();
        else if (!strcmp(line, "reset")) handle_reset(flipped);
        else if (!strcmp(line, "test"))  handle_test();
        else if (!strcmp(line, "print")) handle_print(flipped);
        else if (!strcmp(line, "board")) handle_board(flipped);
        else if (!strcmp(line, "moves")) handle_moves(line + 6);
        else if (!strcmp(line, "undo"))  handle_undo(flipped);
        else if (!strcmp(line, "perft")) handle_perft(line + 6);
        else if (!strcmp(line, "eval"))  handle_eval(line + 5);
        else if (!strcmp(line, "play"))  handle_play(line + 5, flipped);
        else if (!strcmp(line, "flip"))  handle_flip(&flipped);
        else if (!strcmp(line, "fen"))   handle_fen(tok + 4, flipped);
        else if (!strcmp(line, "end"))   return;
        else                             handle_move(line, flipped);
        printf("\n");
    }
}
