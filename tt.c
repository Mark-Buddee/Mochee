#include <stdint.h>
#include <stdio.h>
#define NDEBUG
#include <assert.h>
#include "inc/defs.h"
#include "inc/magic.h"
#include "inc/tt.h"
#include "inc/board.h"
#include "inc/bitboard.h"
#include "inc/tgui.h"

TTEntry_s TT[TT_ENTRIES];

U64 zobrist_square[NUM_SIDES][NUM_PIECES][NUM_SQUARES];
U64 zobrist_blackToPlay;
U64 zobrist_castle[NUM_CASTLING];
U64 zobrist_enpSq[NUM_SQUARES];

extern unsigned long long tableHits;
extern unsigned long long tableUpdates;
extern unsigned long long tableOverwrites;

// int is_hit(U64 key) {
//     TTEntry_s CurrentEntry = TT[key % TT_ENTRIES];
//     if(CurrentEntry.key == (key >> 32)) return 1;
//     return 0;
// }

// int add_entry(U64 key, TTEntry_s NewEntry) {
//     TTEntry_s CurrentEntry = TT[key % TT_ENTRIES];

//     if(IS_PV_NODE(CurrentEntry.scoreBound) && CurrentEntry.age == 0) return;
//     if((CurrentEntry.key == key >> 32) && (CurrentEntry.depth >= NewEntry.depth)) return;

//     TT[key % TT_ENTRIES] = NewEntry;
// }

void add_entry(U64 key, Move bestMove, uint16_t scoreBound, uint8_t depth) {

    TTEntry_s* CurrentEntry = &TT[key % TT_ENTRIES];

    if(CurrentEntry->key != key >> 32) {
        if(CurrentEntry->age == 0 && CurrentEntry->depth > depth) return; // Prioritise greater depth
    }

    if(CurrentEntry->depth > depth) { // Prioritise greater depth
        CurrentEntry->age = 0;
        return;
    }

    // if(CurrentEntry->depth == depth) {
    //     assert(!IS_PV_NODE(CurrentEntry->scoreBound));
    // }

    TTEntry_s NewEntry = {key >> 32, bestMove, scoreBound, depth, 0};
    
    // if(NewEntry.key == CurrentEntry.key) tableUpdates++;
    // else if(CurrentEntry.key) tableOverwrites++;
    *CurrentEntry = NewEntry;
}

// int is_table_hit(U64 key, int depth) {
//     TTEntry_s CurrentEntry = TT[key % TT_ENTRIES];
//     if(CurrentEntry.key == 0) return 0;
//     if(CurrentEntry.key != (key >> 32)) {
//         tableOverwrites++;
//         return 0;
//     }
//     if(CurrentEntry.depth < depth) {
//         tableUpdates++;
//         return 0;
//     }
//     tableHits++;
//     return 1;
// }

void init_tt(void) {
    // TTEntry_s BlankEntry = {0, 0, NULL_MOVE, BLANK_NODE, 0, 0};
    TTEntry_s BlankEntry = {0, NULL_MOVE, BLANK_NODE, 0, 0};
    for(long long unsigned i = 0; i < TT_ENTRIES; i++)
        TT[i] = BlankEntry;
}

void inc_age(void) {
    printf("Incrementing age\n");
    for(long long unsigned i = 0; i < TT_ENTRIES; i++)
        if(TT[i].key) TT[i].age++;
}

void dec_age(void) {
    for(long long unsigned i = 0; i < TT_ENTRIES; i++)
        if(TT[i].key && TT[i].age > 0) TT[i].age--;
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
