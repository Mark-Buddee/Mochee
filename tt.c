#include <stdint.h>
#include "inc/defs.h"
#include "inc/magic.h"
#include "inc/tt.h"
#include "inc/board.h"
#include "inc/bitboard.h"

TTEntry_s TT[TT_ENTRIES];

U64 zobrist_square[NUM_SIDES][NUM_PIECES][NUM_SQUARES];
U64 zobrist_blackToPlay;
U64 zobrist_castle[NUM_CASTLING];
U64 zobrist_enpSq[NUM_SQUARES];

void init_tt(void) {
    TTEntry_s BlankEntry = {0, 0, 0, 0, 0, 0};
    for(long long unsigned i = 0; i < TT_ENTRIES; i++)
        TT[i] = BlankEntry;
}

void init_zobrist(void) {
    for(int i = 0; i < NUM_SIDES; i++)
        for(int j = 0; j < NUM_PIECES; j++)
            for(int k = 0; k < NUM_SQUARES; k++)
                zobrist_square[i][j][k] = rand64();

    zobrist_blackToPlay = rand64();

    for(int i = 0; i < NUM_CASTLING; i++)
        zobrist_castle[i] = rand64();

    for(int file = FILE_A; file <= FILE_H; file++) {
        U64 tRand = rand64();
        for(int rank = RANK_1; rank <= RANK_8; rank++) {
            int sq = FR(file, rank);
            zobrist_enpSq[sq] = tRand;
        }
    }
}

void init_zobrist_key(Board_s* const Board) {
    U64 key = 0ULL;

    for(int side = WHITE; side <= BLACK; side++)
        for(int pieceType = PAWN; pieceType <= KING; pieceType++) {
            U64 srcs = piece(Board, pieceType, side);

            while(srcs) {
                int src = pop_lsb(&srcs);
                key ^= zobrist_square[side][pieceType][src];
            }
        }
    
    key ^= zobrist_castle[Board->castlingRights & ANY_CASTLING];
    if(Board->side == BLACK) key ^= zobrist_blackToPlay;
    if(Board->enPas)         key ^= zobrist_enpSq[lsb(Board->enPas)];

    Board->key = key;
}
