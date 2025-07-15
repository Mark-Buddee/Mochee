#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <assert.h>
#include "defs.h"
#include "board.h"
#include "gen.h"
#include "read.h"
#include "console.h"
#include "bitboard.h"
#include "eval.h"
#include "magic.h"
#include "tt.h"
#include "move.h"

U64 piece(const Board_s* const Board, const int pieceType, const int side) {
    return Board->byType[pieceType] & Board->byColour[side];
}

U64 attackers_to(const Board_s* const Board, int sq, U64 obstacles) {
    return (pawn_attacks[BLACK][sq]                 &  piece(Board, PAWN, WHITE))
         | (pawn_attacks[WHITE][sq]                 &  piece(Board, PAWN, BLACK))
         | (pseudo_attacks[KNIGHT][sq]              &  Board->byType[KNIGHT])
         | (gen_bishop_magic_attacks(sq, obstacles) & (Board->byType[BISHOP] | Board->byType[QUEEN]))
         | (gen_rook_magic_attacks(sq, obstacles)   & (Board->byType[ROOK]   | Board->byType[QUEEN]))
         | (pseudo_attacks[KING][sq]                &  Board->byType[KING]);
}

void update_checkers(Board_s* const Board) {
    int side = Board->side;
    int ksq = lsb(piece(Board, KING, side));
    U64 obstacles = Board->byType[ALL];
    Board->checkers = attackers_to(Board, ksq, obstacles) & Board->byColour[!side];
}

// // Squares for a piece that will deliver check upon landing on
// void update_checkSquares(Board_s* const Board, const int side) {
//     int enemyKsq = lsb(piece(Board, KING, !side));
//     U64 obstacles = Board->byType[ALL];
//     for(int pieceType = PAWN; pieceType <= ROOK; pieceType++)
//         Board->checkSquares[side][pieceType] = attacks(pieceType, enemyKsq, obstacles);
//     Board->checkSquares[side][QUEEN] = Board->checkSquares[side][BISHOP] | Board->checkSquares[side][ROOK];
// }

// Pinned pieces (for both sides)
void update_kingBlockers(Board_s* const Board, const int side) {
    U64 obstacles = Board->byType[ALL];
    Board->kingBlockers[side] = 0ULL;
    int ksq = lsb(piece(Board, KING, side));
    U64 kingBlockerCandidates = gen_queen_magic_attacks(ksq, obstacles) & obstacles;

    while(kingBlockerCandidates) {
        int kingBlockerCandidate = pop_lsb(&kingBlockerCandidates);
        if((attackers_to(Board, ksq, obstacles ^ BIT(kingBlockerCandidate)) ^ Board->checkers) & Board->byColour[!side]) // sketchy but valid
            Board->kingBlockers[side] |= BIT(kingBlockerCandidate);
    }
}

void init_check_data(Board_s* const Board) {
    update_checkers(Board);
    // update_checkSquares(Board, WHITE);
    // update_checkSquares(Board, BLACK);
    update_kingBlockers(Board, WHITE);
    update_kingBlockers(Board, BLACK);
}

// We may not have a king
// Either side may unknowingly be in check
// Function is to only be called within do_move as Board->side is opposite to convention
// kingBlockers and checkers are NOT valid during Qsearch
void update_check_data(Board_s* const Board, const Move move, const int mvd) {
    int side = Board->side; // we investigate if side is in check. !side has just moved
    int src = SRC(move);
    int dst = DST(move);
    int spc = SPC(move);
    int ppt = PPT(move);

    U64 srcDst64 = BIT(src) | BIT(dst);
    U64 ksqBit      = piece(Board, KING, side);
    U64 enemyKsqBit = piece(Board, KING, !side);
    assert(enemyKsqBit);

    if(!ksqBit) { // they've just captures our king
        Board->checkers = BIT(dst);
        // Board->kingBlockers[side] = 0; // there is no king to block
        // update_kingBlockers(Board, !side);
        return;
    }

    int ksq      = lsb(piece(Board, KING, side));
    int enemyKsq = lsb(piece(Board, KING, !side));

    int promotedPiece = (spc == PROMOTION ? ppt + KNIGHT : mvd);
    int castle = src < dst ? side == WHITE ? BLACK_OO  : WHITE_OO
                           : side == WHITE ? BLACK_OOO : WHITE_OOO;
    if( (BIT(src) & Board->kingBlockers[side])          ||
        (BIT(dst) & pseudo_attacks[promotedPiece][ksq]) ||
        (spc == EN_PASSANT)                             ||
        ((spc == CASTLING) && (BIT(rook_dst[castle]) & gen_rook_magic_attacks(ksq, Board->byType[ALL]))))
            update_checkers(Board);
    else Board->checkers = 0ULL;

    // update_checkSquares(Board, side);
    // update_checkSquares(Board, !side);
    // if(srcDst64 & Board->checkSquares[side][QUEEN])
    //     update_checkSquares(Board, side);
    // else if(mvd == KING)
    //     update_checkSquares(Board, !side);
    // else if(srcDst64 & Board->checkSquares[!side][QUEEN])
    //     update_checkSquares(Board, !side);
    
    if((mvd == KING) || (spc == EN_PASSANT)) {
        update_kingBlockers(Board, WHITE); // TODO: surely I don't need two conditions
        update_kingBlockers(Board, BLACK);
    } else {
        if(srcDst64 & pseudo_attacks[QUEEN][ksq])      update_kingBlockers(Board, side);
        if(srcDst64 & pseudo_attacks[QUEEN][enemyKsq]) update_kingBlockers(Board, !side);
    }
}

void remove_piece(Board_s* const Board, const int pieceType, const int square, const int side) {
    U64 bit = BIT(square);
    Board->pieces[square] = EMPTY;
    Board->byType[pieceType] ^= bit;
    Board->byType[ALL] ^= bit;
    Board->byColour[side] ^= bit;
    Board->key ^= zobrist_square[side][pieceType][square];
}

void add_piece(Board_s* const Board, const int pieceType, const int square, const int side) {
    U64 bit = BIT(square);
    Board->pieces[square] = pieceType;
    Board->byType[pieceType] |= bit;
    Board->byType[ALL] |= bit;
    Board->byColour[side] |= bit;
    Board->key ^= zobrist_square[side][pieceType][square];
}

void move_piece(Board_s* const Board, const int pieceType, const int src, const int dst, const int side) {
    U64 srcDst64 = BIT(src) | BIT(dst);
    Board->pieces[src] = EMPTY;
    Board->pieces[dst] = pieceType;
    Board->byType[pieceType] ^= srcDst64;
    Board->byType[ALL] ^= srcDst64;
    Board->byColour[side] ^= srcDst64;
    Board->key ^= zobrist_square[side][pieceType][src];
    Board->key ^= zobrist_square[side][pieceType][dst];
}

int isLegal(const Board_s* const Board) {
    int sideToMove = Board->side;
    int notSideToMove = sideToMove == WHITE ? BLACK : WHITE;

    U64 whiteKsq64 = piece(Board, KING, WHITE);
    U64 blackKsq64 = piece(Board, KING, BLACK);
    if(BITS(whiteKsq64) != 1) return 0;
    if(BITS(blackKsq64) != 1) return 0;

    int stmEnemyKsq = lsb(piece(Board, KING, notSideToMove));
    if(attackers_to(Board, stmEnemyKsq, Board->byType[ALL]) & Board->byColour[sideToMove]) return 0;

    return 1;
}

Board_s board_init(const char* fen) {
    Board_s Board;

    // Make a writable copy of the FEN string
    char fen_copy[128];
    strncpy(fen_copy, fen, sizeof(fen_copy) - 1);
    fen_copy[sizeof(fen_copy) - 1] = '\0';

    // Parse FEN
    char *fenPieces         = strtok(fen_copy , " ");
    char *fenSide           = strtok(NULL, " ");
    char *fenCastlingRights = strtok(NULL, " ");
    char *fenEnPas          = strtok(NULL, " ");
    char *fenHalfMove       = strtok(NULL, " ");
    char *fenFullMove       = strtok(NULL, " ");

    // Meta data
    Board.side           = fen_side(fenSide);
    Board.castlingRights = fen_castlingRights(fenCastlingRights);
    Board.hundredPly     = fen_hundredPly(fenHalfMove);
    Board.hisPly         = fen_hisPly(fenFullMove, Board.side);
    Board.enPas          = fen_enPas(fenEnPas);

    // Initialise
    for(int sq = A1; sq <= H8; sq++)
        Board.pieces[sq] = 0;
    for(int i = 0; i < NUM_PIECES; i++)
        Board.byType[i] = 0;
    for(int i = 0; i < NUM_SIDES; i++)
        Board.byColour[i] = 0;
    // for(int i = 0; i < NUM_PIECES; i++) {
    //     Board.checkSquares[WHITE][i] = 0;
    //     Board.checkSquares[BLACK][i] = 0;
    // }
    Undo_s tUndo = {NULL_MOVE, EMPTY, NO_CASTLING, 0, 0ULL, .kingBlockers[WHITE] = 0ULL, 0ULL, 0, 0ULL};
    for(int i = 0; i < MAX_GAME_PLYS; i++)
        Board.Undos[i] = tUndo;

    // Set piece placement
    char* tok = strtok(fenPieces, "/");
    for(int rank = RANK_8; rank >= RANK_1; rank--) {
        int i = 0;
        int file = FILE_A;
        while(file <= FILE_H) {
            char fenChar = tok[i];
            if(isdigit(fenChar)) {
                file += atoi(&fenChar);
            } else {
                int pieceType = fen_piece(fenChar);
                int side = fen_colour(fenChar);
                int square = FR(file, rank);
                add_piece(&Board, pieceType, square, side);
                file++;    
            }
            i++;
        }
        tok = strtok(NULL, "/");
    }

    // Legality metadata
    init_check_data(&Board);

    // Zobrist hash key
    init_zobrist_key(&Board);

    // Static evaluation
    Board.staticEval = static_eval(&Board);

    // inc_posFreq(&Board);

    return Board;
}
