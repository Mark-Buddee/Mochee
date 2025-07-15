#include <stdio.h>
#include <stdlib.h>
#include "defs.h"
#include "debug.h"
#include "board.h"
#include "console.h"
#include "tt.h"
#include "move.h"
#include "eval.h"

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
    // int side = Board->side;
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
    // printf("pawnChecks........%.16llx\n", Board->checkSquares[side][PAWN]);
    // printf("knightChecks......%.16llx\n", Board->checkSquares[side][KNIGHT]);
    // printf("bishopChecks......%.16llx\n", Board->checkSquares[side][BISHOP]);
    // printf("rookChecks........%.16llx\n", Board->checkSquares[side][ROOK]);
    // printf("queenChecks.......%.16llx\n", Board->checkSquares[side][QUEEN]);
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
            printf(BACK_SPACE);
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