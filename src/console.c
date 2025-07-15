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

void src2str(char srcStr[], int src) {
    srcStr[0] = FILE(src) + 'a';
    srcStr[1] = RANK(src) + '1';
    srcStr[2] = '\0';
}

void console(void) {

    printf("Entering console mode. ");
    printf("Type 'help' for a list of commands\n\n");
    init_all();

    Board = board_init(START_FEN);
    int flipped = Board.side;
    print_board(&Board, flipped);
    printf("\n");

    char line[BUFF_SIZE];

    while(1) {
        // // Detect checkmate
        // Move_s List[MAX_MOVES];
        // if(gen_legal(&Board, List) == List) {
        //     if(Board.checkers) printf("Checkmate!\n");
        //     else printf("Stalemate!");
        //     // exit(1);
        // }

        fgets(line, BUFF_SIZE, stdin);
        // printf("%s%s%s", LINE_START, empty_s, LINE_START); // removes previous line, resets cursor
        line[strcspn(line, "\n")] = ' '; // removes newline character from end
        char* tok = strtok(line, " ");
        assert(tok != NULL);

        if(strcmp(tok, "help") == 0) {
            printf(
                " help\n"
                " reset\n"
                " print\n"
                " board\n"
                " undo\n"
                " test\n"
                " flip\n"
                " end\n"
                " moves <depth>\n"
                " perft <depth>\n"
                " eval  <depth>\n"
                " play  <time (ms)>\n"
                " fen   <fen>\n"
                " <move>\n");

        } else if (strcmp(tok, "reset") == 0) {
            // strcpy(start_fen_s, START_FEN);
            init_tt();
            // Board = board_init(start_fen_s);
            Board = board_init(START_FEN);
            print_board(&Board, flipped);

        } else if (strcmp(tok, "test") == 0) {
            perft_unit_test();

        } else if (strcmp(tok, "print") == 0) {
            print_board(&Board, flipped);

        } else if (strcmp(tok, "board") == 0) {
            print_detailed(&Board, flipped);

        } else if (strcmp(tok, "moves") == 0) {
            tok = strtok(NULL, " ");
            int depth = tok == NULL ? 1 : atoi(tok);
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

        } else if (strcmp(tok, "undo") == 0) {
            // TODO: Don't allow undos beyond starting fen
            undo_move(&Board);
            inc_age();
            // dec_age();
            print_board(&Board, flipped);

        } else if (strcmp(tok, "perft") == 0) {
            tok = strtok(NULL, " ");
            assert(tok != NULL);
            int depth = atoi(tok);
            perft(&Board, depth);

        } else if (strcmp(tok, "end") == 0) {
            return;

        } else if (strcmp(tok, "eval") == 0) {
            tok = strtok(NULL, " ");
            assert(tok != NULL);
            int depth = atoi(tok);
            do_search(&Board, depth);

        } else if (strcmp(tok, "play") == 0) {
            tok = strtok(NULL, " ");
            assert(tok != NULL);
            double duration = atof(tok);
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

        } else if (strcmp(tok, "flip") == 0) {
            flipped = !flipped;
            print_board(&Board, flipped);

        } else if (strcmp(tok, "fen") == 0) {
            tok = strtok(NULL, "\0");
            assert(tok != NULL);
            init_tt();
            Board = board_init(tok);
            print_board(&Board, flipped);

        } else {
            int ppt = get_ppt(tok);
            int src = str2src(tok++);
            int dst = str2src(++tok);

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
            // Move_s List2[MAX_MOVES];
            // cur = List2;
            // end = gen_all(&Board, List2, Board.side, CAPTURES);
            // while(cur != end) {
            //     if(SRC(cur->move) == src && DST(cur->move) == dst) {
            //         // change this to commented out //MoveToMake = cur->move;
            //         // MoveSToMake->moveVal = move_eval(&Board, MoveSToMake->move);
            //         break;
            //     }
            //     cur++;
            // }
            if(MoveToMake != NULL) {
                do_move(&Board, MoveToMake);
                inc_age();
                print_board(&Board, flipped);
            } else
                printf("Move not found\n");
        }
        printf("\n");
    }
}


