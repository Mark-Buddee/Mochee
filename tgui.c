#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
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

void print_board(const Board_s *Board, int flipped) {
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

void src2str(char srcStr[], int src) {
    srcStr[0] = FILE(src) + 'a';
    srcStr[1] = RANK(src) + '1';
    srcStr[2] = '\0';
}

int str2src(char string[]) {
    int file = string[0] - 'a';
    int rank = atoi(++string) - 1;
    return FR(file, rank);
}

void debug(void) {
    printf("DEBUG MODE\n");
    printf("Type help for a list of commands.\n\n");

    char start_fen_s[] = START_FEN;
    Board_s Board = board_init(start_fen_s);
    int flipped = Board.side;
    print_board(&Board, flipped);
    printf("\n");
    char line[BUFF_SIZE];

    //Commands
    char help_s[] = "help", reset_s[] = "reset", print_s[] = "print", board_s[] = "board", moves_s[] = "moves", undo_s[] = "undo", perft_s[] = "perft", eval_s[] = "eval", play_s[] = "play", flip_s[] = "flip", fen_s[] = "fen", end_s[] = "end";
    // 255 Empty array. Use memcpy here instead duh
    char empty_s[] = "                                                                                                                                                                                                                                                                ";

    while(1) {
        // Detect checkmate
        Move_s List[MAX_MOVES];
        if(gen_legal(&Board, List) == List) {
            printf("Checkmate!\n");
            // exit(1);
        }

        fgets(line, BUFF_SIZE, stdin);
        printf("\033[F%s\033[F", empty_s); // removes previous line, resets cursor
        line[strcspn(line, "\n")] = ' '; // removes newline character from end
        char* tok = strtok(line, " ");
        assert(tok != NULL);

        if(strcmp(tok, help_s) == 0) {
            printf(
                "Commands:\n"
                " help\n"
                " reset\n"
                " print\n"
                " board\n"
                " moves\n"
                " MOVE\n"
                " undo\n"
                " perft DEPTH\n"
                " eval DEPTH\n"
                " play DEPTH\n"
                " flip\n"
                " fen FEN\n"
                " end\n");

        } else if (strcmp(tok, reset_s) == 0) {
            strcpy(start_fen_s, START_FEN);
            Board = board_init(start_fen_s);
            print_board(&Board, flipped);

        } else if (strcmp(tok, print_s) == 0) {
            print_board(&Board, flipped);

        } else if (strcmp(tok, board_s) == 0) {
            print_board(&Board, flipped);
            int side = Board.side;
            printf("\n");
            printf("side..............%s\n", Board.side == WHITE ? "WHITE" : "BLACK");
            printf("hisPly............%d\n", Board.hisPly);
            printf("hundredPly........%d\n", Board.hundredPly);
            printf("castlingRights....%.8x\n", Board.castlingRights);
            printf("staticEval........%d\n", Board.staticEval);
            printf("\n");
            printf("enPas.............%.16llx\n", Board.enPas);
            printf("white.............%.16llx\n", Board.byColour[WHITE]);
            printf("black.............%.16llx\n", Board.byColour[BLACK]);
            printf("all...............%.16llx\n", Board.byType[ALL]);
            printf("pawns.............%.16llx\n", Board.byType[PAWN]);
            printf("knights...........%.16llx\n", Board.byType[KNIGHT]);
            printf("bishops...........%.16llx\n", Board.byType[BISHOP]);
            printf("rooks.............%.16llx\n", Board.byType[ROOK]);
            printf("queens............%.16llx\n", Board.byType[QUEEN]);
            printf("kings.............%.16llx\n", Board.byType[KING]);
            printf("\n");
            printf("checkers..........%.16llx\n", Board.checkers);
            printf("pawnChecks........%.16llx\n", Board.checkSquares[side][PAWN]);
            printf("knightChecks......%.16llx\n", Board.checkSquares[side][KNIGHT]);
            printf("bishopChecks......%.16llx\n", Board.checkSquares[side][BISHOP]);
            printf("rookChecks........%.16llx\n", Board.checkSquares[side][ROOK]);
            printf("queenChecks.......%.16llx\n", Board.checkSquares[side][QUEEN]);
            printf("whiteBlockers.....%.16llx\n", Board.kingBlockers[WHITE]);
            printf("blackBlockers.....%.16llx\n", Board.kingBlockers[BLACK]);

        } else if (strcmp(tok, moves_s) == 0) {
            tok = strtok(NULL, " ");
            int depth = tok == NULL ? 1 : atoi(tok);
            Move_s List[MAX_MOVES];
            Move_s* cur = List;
            Move_s* end = gen_legal(&Board, List);
            while(cur != end) {
                Move move = cur->move;
                char srcStr[3];
                src2str(srcStr, SRC(move));
                char dstStr[3];
                src2str(dstStr, DST(move));
                do_move(&Board, move);
                printf("%.2s%.2s %llu\n", srcStr, dstStr, num_nodes(&Board, depth));
                undo_move(&Board);
                cur++;
            }

        } else if (strcmp(tok, undo_s) == 0) {
            undo_move(&Board);
            print_board(&Board, flipped);
        } else if (strcmp(tok, perft_s) == 0) {
            tok = strtok(NULL, " ");
            assert(tok != NULL);
            int depth = atoi(tok);
            perft(&Board, depth);

        } else if (strcmp(tok, end_s) == 0) {
            exit(1);
        } else if (strcmp(tok, eval_s) == 0) {
            tok = strtok(NULL, " ");
            assert(tok != NULL);
            int depth = atoi(tok);
            do_search(&Board, depth);
        } else if (strcmp(tok, play_s) == 0) {
            tok = strtok(NULL, " ");
            assert(tok != NULL);
            int depth = atoi(tok);
            int bestMove;
            clock_t start, end;
            start = clock();   
            // int eval = alpha_beta(&Board, -INF, INF, depth);
            int eval = alpha_beta(&Board, -INF, INF, depth, depth, &bestMove);
            end = clock();
            double dt = (double)(end-start)/CLOCKS_PER_SEC;
            int trueEval = Board.side == WHITE ? eval : -eval;
            do_move(&Board, bestMove);

            char bestSrc[3];
            char bestDst[3];
            src2str(bestSrc, SRC(bestMove));
            src2str(bestDst, DST(bestMove));
            printf("Eval: %5d bestMove: %s%s time:%7g\n", trueEval, bestSrc, bestDst, dt);
            print_board(&Board, flipped);
        } else if (strcmp(tok, flip_s) == 0) {
            flipped = !flipped;
            print_board(&Board, flipped);
        } else if (strcmp(tok, fen_s) == 0) {
            tok = strtok(NULL, "\0");
            assert(tok != NULL);
            Board = board_init(tok);
            print_board(&Board, flipped);
        } else {
            int src = str2src(tok++);
            int dst = str2src(++tok);

            Move moveToMake = NULL_MOVE;
            Move_s List[MAX_MOVES];
            Move_s* cur = List;
            Move_s* end = gen_legal(&Board, List);
            while(cur != end) {
                if(SRC(cur->move) == src && DST(cur->move) == dst) {
                    moveToMake = cur->move;
                    break;
                }
                cur++;
            }
            if(moveToMake != NULL_MOVE) {
                do_move(&Board, moveToMake);
                print_board(&Board, flipped);
            } else
                printf("Move not found\n");
        }
        printf("\n");
    }
}
