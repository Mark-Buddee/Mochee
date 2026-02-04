#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include "defs.h"
#include "magic.h"
#include "tt.h"
#include "board.h"
#include "bitboard.h"
#include "console.h"
#include "debug.h"

TTEntry_s* TT;
unsigned long long TTEntries;

U64 zobrist_square[NUM_SIDES][NUM_PIECES][NUM_SQUARES];
U64 zobrist_blackToPlay;
U64 zobrist_castle[NUM_CASTLING];
U64 zobrist_enpSq[NUM_SQUARES];

extern unsigned long long tableHits;
extern unsigned long long tableUpdates;
extern unsigned long long tableOverwrites;

// int is_hit(U64 key) {
//     TTEntry_s CurrentEntry = TT[key % TT_ENTRIES];
//     if(CurrentEntry.key == KEY_TOP(key)) return 1;
//     return 0;
// }

// int add_entry(U64 key, TTEntry_s NewEntry) {
//     TTEntry_s CurrentEntry = TT[key % TT_ENTRIES];

//     if(IS_PV_NODE(CurrentEntry.scoreBound) && CurrentEntry.rootPly == 0) return; // TODO rootply should not be 0
//     if((CurrentEntry.key == KEY_TOP(key)) && (CurrentEntry.depth >= NewEntry.depth)) return;

//     TT[key % TT_ENTRIES] = NewEntry;
// }

void add_entry(U64 key, Move bestMove, uint16_t scoreBound, uint8_t depth, int rootPly) {

    TTEntry_s* CurrentEntry = &TT[key % TTEntries];

    int keyMatch = CurrentEntry->key == KEY_TOP(key);
    int deeper = depth > CurrentEntry->depth;
    int8_t ageDifference = (int8_t)((uint8_t)rootPly - CurrentEntry->rootPly);
    
    if(!deeper) {

        // Oldest entries (age difference > 3) can always be overwritten, regardless of depth
        // or if its in the future age difference <0

        if(!ageDifference) return; // same age, not deeper -> do not overwrite

        if(keyMatch && (depth != CurrentEntry->depth || IS_PV_NODE(CurrentEntry->scoreBound) || !IS_PV_NODE(scoreBound))) {
            assert(depth != CurrentEntry->depth || IS_PV_NODE(CurrentEntry->scoreBound) || !IS_PV_NODE(scoreBound)); // Don't miss the chance to upgrade to a PV node
            CurrentEntry->rootPly = (uint8_t)rootPly; // untouched keymatches can update their age
            return;
        }

    }

    #ifndef NDEBUG

        // Don't downgrade a PV entry to non-PV
        if(keyMatch && CurrentEntry->depth == depth) assert(!IS_PV_NODE(CurrentEntry->scoreBound) || IS_PV_NODE(scoreBound));
        
        if (keyMatch) TTStats.updates++;
        else if (CurrentEntry->key) TTStats.overwrites++;

    #endif

    // printf("Replacing depth %u (rootPly %u) entry with depth %u (rootPly %u) entry. Key match: %d, Age difference: %d, deeper: %d",
    //     (unsigned)CurrentEntry->depth, (unsigned)CurrentEntry->rootPly,
    //     (unsigned)depth, (unsigned)rootPly,
    //     keyMatch, (int)ageDifference, (int)deeper);
    // if(IS_PV_NODE(CurrentEntry->scoreBound)) printf(" [OLD PV NODE]");
    // else if(IS_CUT_NODE(CurrentEntry->scoreBound)) printf(" [OLD CUT NODE]");
    // else if(IS_ALL_NODE(CurrentEntry->scoreBound)) printf(" [OLD ALL NODE]");
    // if(IS_PV_NODE(scoreBound)) printf(" [PV NODE]");
    // else if(IS_CUT_NODE(scoreBound)) printf(" [CUT NODE]");
    // else if(IS_ALL_NODE(scoreBound)) printf(" [ALL NODE]");
    // printf(" key: 0x%04x", (unsigned)(KEY_TOP(key));
    // printf("\n");

    // Write to / overwrite the transposition table
    TTEntry_s NewEntry = {KEY_TOP(key), bestMove, scoreBound, depth, (uint8_t)rootPly};
    // TTEntry_s NewEntry = {KEY_TOP(key), bestMove, scoreBound, depth, 0};
    *CurrentEntry = NewEntry;
    
}

// int is_table_hit(U64 key, int depth) {
//     TTEntry_s CurrentEntry = TT[key % TT_ENTRIES];
//     if(CurrentEntry.key == 0) return 0;
//     if(CurrentEntry.key != (KEY_TOP(key))) {
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
    TTEntry_s BlankEntry = {0, NULL_MOVE, BLANK_NODE, 0, 0};
    for(long long unsigned i = 0; i < TTEntries; i++)
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

void print_entry(U64 key) {
    TTEntry_s CurrentEntry = TT[key % TTEntries];
    if(CurrentEntry.key != KEY_TOP(key)) {
        printf("No entry found.\n");
        return;
    }
    printf("Entry found:\n");
    printf(" Key:        0x%04x\n", (unsigned)CurrentEntry.key);
    printf(" Move:       ");
    print_move(CurrentEntry.move);
    printf("\n");
    printf(" Node type:  ");
    if(IS_PV_NODE(CurrentEntry.scoreBound))      printf("PV_NODE\n");
    else if(IS_CUT_NODE(CurrentEntry.scoreBound)) printf("CUT_NODE\n");
    else if(IS_ALL_NODE(CurrentEntry.scoreBound)) printf("ALL_NODE\n");
    else if(IS_BLANK_NODE(CurrentEntry.scoreBound)) printf("BLANK_NODE\n");
    else                                          printf("UNKNOWN NODE TYPE\n");
    printf(" Score:      %d\n", SCORE(CurrentEntry.scoreBound));
    printf(" Depth:      %u\n", (unsigned)CurrentEntry.depth);
    printf(" RootPly:    %u\n", (unsigned)CurrentEntry.rootPly);
}
