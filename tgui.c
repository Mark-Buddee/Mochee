#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#define NDEBUG
#include <assert.h>
#include <time.h>

#include "inc/defs.h"
#include "inc/tgui.h"
#include "inc/board.h"
#include "inc/gen.h"
#include "inc/move.h"
#include "inc/perft.h"
#include "inc/search.h"
#include "inc/eval.h"
#include "inc/tt.h"
#include "inc/magic.h"
#include "inc/init.h"

void print_version(void) {
    printf("\n%s %s %s\n\n", NAME, VERSION, NAME_DESC);
}

void print_bitBoard(U64 bitBoard) {
    for(int rank = RANK_8; rank >= RANK_1; rank--) {
        for(int file = FILE_A; file <= FILE_H; file++) {
            int sq = FR(file, rank);
            bitBoard & BIT(sq) ? printf("x") : printf(".");
            printf(" ");
        }
        printf("\n");
    }
    printf("\n");
}

void print_board(const Board_s* const Board, int flipped) {
    if(flipped) {
        for(int rank = RANK_1; rank <= RANK_8; rank++) {
            for(int file = FILE_H; file >= FILE_A; file--) {
                int square = FR(file, rank);
                int piece = Board->pieces[square];
                int side = Board->byColour[WHITE] & BIT(square) ? WHITE : BLACK;
                char piece_char = PIECE_CHAR(side, piece);
                printf("%c ", piece_char);
            }
        printf("\n");
        }
    } else {
        for(int rank = RANK_8; rank >= RANK_1; rank--) {
            for(int file = FILE_A; file <= FILE_H; file++) {
                int square = FR(file, rank);
                int piece = Board->pieces[square];
                int side = Board->byColour[WHITE] & BIT(square) ? WHITE : BLACK;
                char piece_char = PIECE_CHAR(side, piece);
                printf("%c ", piece_char);
            }
        printf("\n");
        }
    }
    // printf("\n");
}

void print_detailed(const Board_s* const Board, int flipped) {

    // int posFreq = 0;
    // if(TT[Board->key % TT_ENTRIES].key == Board->key >> 32) {
    //     posFreq = TT[Board->key % TT_ENTRIES].posFreq;
    // }

    print_board(Board, flipped);
    int side = Board->side;
    printf("\n");
    printf("side..............%s\n", Board->side == WHITE ? "WHITE" : "BLACK");
    printf("hisPly............%d\n", Board->hisPly);
    printf("hundredPly........%d\n", Board->hundredPly);
    // printf("posFreq...........%d\n", posFreq);
    printf("castlingRights....%.1x\n", Board->castlingRights);
    printf("staticEval........%d\n", Board->staticEval);
    printf("key...............%.16llx\n", Board->key);
    printf("\n");
    printf("enPas.............%.16llx\n", Board->enPas);
    printf("white.............%.16llx\n", Board->byColour[WHITE]);
    printf("black.............%.16llx\n", Board->byColour[BLACK]);
    printf("all...............%.16llx\n", Board->byType[ALL]);
    printf("pawns.............%.16llx\n", Board->byType[PAWN]);
    printf("knights...........%.16llx\n", Board->byType[KNIGHT]);
    printf("bishops...........%.16llx\n", Board->byType[BISHOP]);
    printf("rooks.............%.16llx\n", Board->byType[ROOK]);
    printf("queens............%.16llx\n", Board->byType[QUEEN]);
    printf("kings.............%.16llx\n", Board->byType[KING]);
    printf("\n");
    printf("checkers..........%.16llx\n", Board->checkers);
    printf("pawnChecks........%.16llx\n", Board->checkSquares[side][PAWN]);
    printf("knightChecks......%.16llx\n", Board->checkSquares[side][KNIGHT]);
    printf("bishopChecks......%.16llx\n", Board->checkSquares[side][BISHOP]);
    printf("rookChecks........%.16llx\n", Board->checkSquares[side][ROOK]);
    printf("queenChecks.......%.16llx\n", Board->checkSquares[side][QUEEN]);
    printf("whiteBlockers.....%.16llx\n", Board->kingBlockers[WHITE]);
    printf("blackBlockers.....%.16llx\n", Board->kingBlockers[BLACK]);
}

// 1. Pe2e4 Pd7d5 2.Pe4d5 qd8d5 3. Ng1f3 Nb8c6
void print_pgn(const Board_s* const Board) {
    char start_fen_s[] = START_FEN; // Must be changed every time you change the start position and want to debug
    // char start_fen_s[] = "8/8/7r/K6k/1Q6/8/8/8 w - - 0 1";
    Board_s SimBoard = board_init(start_fen_s);
    
    int len = Board->hisPly;
    for(int i = 1; i <= len; i++) {
        if(i % 2) printf("%d. ", (i+1)/2);
        Undo_s Undo = Board->Undos[i-1];
        Move Move = Undo.move;

        int src = SRC(Move);
        int mvd = SimBoard.pieces[src];

        Move_s cur = {.move = Move, .positionScore = move_position_eval(&SimBoard, Move), .materialScore = move_material_eval(&SimBoard, Move), .orderingBias = 0};
        do_move(&SimBoard, &cur);
        // print_board(&SimBoard, WHITE);

        if(!(SPC(Move) == PROMOTION)) printf("%c", PIECE_CHARS[mvd]); // Don't print P for pawn when promoting for lichess standards
        print_move(Move);

        // Capitalise the promoting piece q -> Q for lichess standards
        if(SPC(Move) == PROMOTION) {
            printf("\033[D"); // Move cursor back one space
            int ppt = KNIGHT + PPT(Move);
            switch(ppt) {
                case KNIGHT:
                    printf("N");
                    break;
                case BISHOP:
                    printf("B");
                    break;
                case ROOK:
                    printf("R");
                    break;
                case QUEEN:
                    printf("Q");
                    break;
                default:
                    printf("print_pgn unexpected input. Exiting...");
                    exit(1);
            }
        }

        if(SimBoard.checkers) printf("+");
        printf(" ");
    }
    printf("\n");
}

void print_variation(Board_s* const Board, int maxDepth) {

    TTEntry_s Entry = TT[Board->key % TT_ENTRIES];
    if(Entry.key != Board->key >> 32) return;
    if(!IS_PV_NODE(Entry.scoreBound)) return;
    Move bestMove = Entry.move;
    if(bestMove == NULL_MOVE) return;

    print_move(Entry.move);
    printf(" ");
    
    if(maxDepth == 1) return;

    Move_s cur = {.move = bestMove, .positionScore = move_position_eval(Board, bestMove), .materialScore = move_material_eval(Board, bestMove), .orderingBias = 0};
    do_move(Board, &cur);
    print_variation(Board, maxDepth - 1);
    undo_move(Board);

    // printf("\n");
}

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

void print_move(Move move) {
    if(move == NULL_MOVE) {
        printf("0000");
        return;
    }

    char src[3];
    char dst[3];
    src2str(src, SRC(move));
    src2str(dst, DST(move));
    printf("%s%s", src, dst);

    if(SPC(move) == PROMOTION) {
        int ppt = KNIGHT + PPT(move);
        switch(ppt) {
            case KNIGHT:
                printf("n");
                break;
            case BISHOP:
                printf("b");
                break;
            case ROOK:
                printf("r");
                break;
            case QUEEN:
                printf("q");
                break;
            default:
                printf("print_move unexpected input. Exiting...");
                exit(1);
        }
    }
}

void console(void) {
    printf("\033[F");
    // printf("\033[2J");
    printf("Entering console mode. ");
    printf("Type 'help' for a list of commands\n\n");
    init_all();

    char start_fen_s[] = START_FEN;
    Board_s Board = board_init(start_fen_s);
    // Board_s Board = board_init(START_FEN);
    int flipped = Board.side;
    print_board(&Board, flipped);
    printf("\n");
    char line[BUFF_SIZE];

    //Commands
    char test_s[] = "test", help_s[] = "help", reset_s[] = "reset", print_s[] = "print", board_s[] = "board", moves_s[] = "moves", undo_s[] = "undo", perft_s[] = "perft", eval_s[] = "eval", play_s[] = "play", flip_s[] = "flip", fen_s[] = "fen", end_s[] = "end";
    // 255 Empty array. Use memcpy here instead duh
    char empty_s[] = "                                                                                                                                                                                                                                                                ";

    while(1) {
        // Detect checkmate
        Move_s List[MAX_MOVES];
        if(gen_legal(&Board, List) == List) {
            if(Board.checkers) printf("Checkmate!\n");
            else printf("Stalemate!");
            // exit(1);
        }

        fgets(line, BUFF_SIZE, stdin);
        printf("\033[F%s\033[F", empty_s); // removes previous line, resets cursor
        line[strcspn(line, "\n")] = ' '; // removes newline character from end
        char* tok = strtok(line, " ");
        assert(tok != NULL);

        if(strcmp(tok, help_s) == 0) {
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

        } else if (strcmp(tok, reset_s) == 0) {
            strcpy(start_fen_s, START_FEN);
            init_tt();
            Board = board_init(start_fen_s);
            print_board(&Board, flipped);

        } else if (strcmp(tok, test_s) == 0) {
            perft_unit_test();

        } else if (strcmp(tok, print_s) == 0) {
            print_board(&Board, flipped);

        } else if (strcmp(tok, board_s) == 0) {
            print_detailed(&Board, flipped);

        } else if (strcmp(tok, moves_s) == 0) {
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

        } else if (strcmp(tok, undo_s) == 0) {
            // TODO: Don't allow undos beyond starting fen
            undo_move(&Board);
            inc_age();
            // dec_age();
            print_board(&Board, flipped);

        } else if (strcmp(tok, perft_s) == 0) {
            tok = strtok(NULL, " ");
            assert(tok != NULL);
            int depth = atoi(tok);
            perft(&Board, depth);

        } else if (strcmp(tok, end_s) == 0) {
            return;

        } else if (strcmp(tok, eval_s) == 0) {
            tok = strtok(NULL, " ");
            assert(tok != NULL);
            int depth = atoi(tok);
            do_search(&Board, depth);

        } else if (strcmp(tok, play_s) == 0) {
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

        } else if (strcmp(tok, flip_s) == 0) {
            flipped = !flipped;
            print_board(&Board, flipped);

        } else if (strcmp(tok, fen_s) == 0) {
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


