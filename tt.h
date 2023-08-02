#ifndef TT_H
#define TT_H

#define TT_SIZE_MB 1024
#define TT_ENTRIES (TT_SIZE_MB*1024*1024/sizeof(TTEntry_s))

#define PV_NODE  1
#define ALL_NODE 2
#define CUT_NODE 4

#define IS_PV_NODE(nodeType)  ((nodeType) & PV_NODE)
#define IS_ALL_NODE(nodeType) ((nodeType) & ALL_NODE)
#define IS_CUT_NODE(nodeType) ((nodeType) & CUT_NODE)

extern U64 zobrist_square[NUM_SIDES][NUM_PIECES][NUM_SQUARES];
extern U64 zobrist_blackToPlay;
extern U64 zobrist_castle[NUM_CASTLING];
extern U64 zobrist_enpSq[NUM_SQUARES];

typedef struct {
    uint32_t key32;
    Move move;
    int depth;
    int score;
    uint8_t nodeType;
    int age;
} TTEntry_s;


void init_tt(void);
void init_zobrist(void);

void init_zobrist_key(Board_s* const Board);

#endif