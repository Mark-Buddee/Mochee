#ifndef TT_H
#define TT_H

#define NODE_TYPE 3
#define PV_NODE  0
#define CUT_NODE 1
#define BLANK_NODE 2
#define ALL_NODE 3

// #define TT_SIZE_MB 1024 // 1900
#define TT_SIZE_MB 1900
#define TT_ENTRIES (unsigned long long)(TT_SIZE_MB*1024*1024/sizeof(TTEntry_s))

#define IS_PV_NODE(scoreBound)      (((scoreBound) & NODE_TYPE) == PV_NODE)
#define IS_ALL_NODE(scoreBound)     (((scoreBound) & NODE_TYPE) == ALL_NODE)
#define IS_BLANK_NODE(scoreBound)   (((scoreBound) & NODE_TYPE) == BLANK_NODE)
#define IS_CUT_NODE(scoreBound)     (((scoreBound) & NODE_TYPE) == CUT_NODE)
#define SCORE(scoreBound)           ((scoreBound) >> 2)
#define SCOREBOUND(score, nodeType) (4*(score) + (nodeType))

typedef struct {
    uint32_t key;
    Move move;
    int16_t scoreBound;
    // uint8_t posFreq : 2; // TODO: Bit packing 
    uint8_t depth;
    uint8_t age;
} TTEntry_s;

extern TTEntry_s TT[TT_ENTRIES];
extern U64 zobrist_square[NUM_SIDES][NUM_PIECES][NUM_SQUARES];
extern U64 zobrist_blackToPlay;
extern U64 zobrist_castle[NUM_CASTLING];
extern U64 zobrist_enpSq[NUM_SQUARES];

// int is_hit(U64 key);
void add_entry(U64 key, Move bestMove, uint16_t scoreBound, uint8_t depth);
void init_tt(void);
void inc_age(void);
void dec_age(void);
void init_zobrist(void);

void init_zobrist_key(Board_s* const Board);
void init_tt(void);

#endif