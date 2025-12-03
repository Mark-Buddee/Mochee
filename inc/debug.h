#ifndef DEBUG_H
#define DEBUG_H

#ifndef NDEBUG
    struct TTStats_s {
        unsigned long long hits;
        unsigned long long updates;
        unsigned long long overwrites;
    };

    extern struct TTStats_s TTStats;
    extern unsigned long long nodesSearched;
#endif

void print_bitBoard(U64 bitBoard);
void print_detailed(const Board_s* const Board, int flipped);
void print_pgn(const Board_s* const Board);
void print_board(const Board_s* const Board, int flipped);
void print_variation(Board_s* const Board, int maxDepth);

void print_move(Move move);

#endif