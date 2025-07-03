#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define NDEBUG
#include <assert.h>
#include "defs.h"
#include "read.h"

int fen_piece(const char fenPiece) {
    switch(tolower(fenPiece)) {
        case 'p':
            return PAWN;
        case 'n':
            return KNIGHT;
        case 'b':
            return BISHOP;
        case 'r':
            return ROOK;
        case 'q':
            return QUEEN;
        case 'k':
            return KING;
        default:
            printf("fen_piece unexpected input. Exiting...\n");
            exit(1);
    }
}

int fen_colour(const char fenPiece) {
    return tolower(fenPiece) == fenPiece ? BLACK : WHITE;
}

int fen_side(const char* fenSide) {
    assert(fenSide != NULL);
    assert(*fenSide == 'w' || *fenSide == 'b');
    return *fenSide == 'w' ? WHITE : BLACK;
}

int fen_castlingRights(const char* fenCastlingRights) {
    assert(fenCastlingRights != NULL);
    int castlingRights = 0;
    castlingRights += strchr(fenCastlingRights, 'K') ? WHITE_OO  : 0;
    castlingRights += strchr(fenCastlingRights, 'Q') ? WHITE_OOO : 0;
    castlingRights += strchr(fenCastlingRights, 'k') ? BLACK_OO  : 0;
    castlingRights += strchr(fenCastlingRights, 'q') ? BLACK_OOO : 0;
    return castlingRights;
}

int fen_hundredPly(const char* fenHalfMove) {
    if(fenHalfMove == NULL) return 0;
    return atoi(fenHalfMove);
}

int fen_hisPly(const char* fenFullMove, const int side) {
    if(fenFullMove == NULL) return 0;
    return (atoi(fenFullMove) - 1) * 2 + side;  // start from zero
}

int fen_file(const char fileChar) {
    return fileChar - 'a';
}

int fen_rank(const char rankChar) {
    return rankChar - '1';
}

int fen_sq(const char* fenSq) {
    int file = fen_file(fenSq[0]);
    int rank = fen_rank(fenSq[1]);
    return FR(file, rank);
}

U64 fen_enPas(const char* fenEnPas) {
    assert(fenEnPas != NULL);
    if(!strcmp(fenEnPas, "-")) return 0;

    assert(strlen(fenEnPas) >= 2);
    return BIT(fen_sq(fenEnPas));
}