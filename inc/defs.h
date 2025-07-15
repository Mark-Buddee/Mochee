#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>

#define NAME                            "Mochee"
#define VERSION                         "v1.1"
#define NAME_DESC                       "Mother of Chess Engines\nCopyright (C) Chonker Bonker Corporation. All rights reserved."
#define AUTHOR                          "Buddee890"

#define STREAM_BUFF_SIZE                1024
#define BUFF_SIZE                       255

#define START_FEN                       "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define MAX_GAME_PLYS                   2047 // 5898 if 100 ply == draw, 8848.5 if 150 ply == draw
#define MAX_MOVES                       218
#define MAX_DEPTH                       255
#define MAX_QUIESCE_DEPTH               -1   // 0 := no quiescient search

enum {
    WHITE, BLACK, BOTH,
    NUM_SIDES = 2
};

enum {
    CAPTURES, QUIETS, QUIET_CHECKS, EVASIONS, NON_EVASIONS,
    NUM_MOVE_TYPES = 5
};
enum {
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H,
    NUM_FILES = 8
};

enum {
    RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8,
    NUM_RANKS = 8
};

enum{
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    OFFBOARD = -1, NUM_SQUARES = 64

};

enum {
    EMPTY, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
    ALL = 0, NUM_PIECES = 7
};

enum {
    NORMAL     = 0, 
    PROMOTION  = 1 << 14,
    EN_PASSANT = 2 << 14,
    CASTLING   = 3 << 14,
    NULL_MOVE = 0
};

enum {
    NO_CASTLING,
    WHITE_OO,
    WHITE_OOO = WHITE_OO << 1,
    BLACK_OO  = WHITE_OO << 2,
    BLACK_OOO = WHITE_OO << 3,
    NUM_CASTLING = 16,

    KING_SIDE      = WHITE_OO  | BLACK_OO,
    QUEEN_SIDE     = WHITE_OOO | BLACK_OOO,
    WHITE_CASTLING = WHITE_OO  | WHITE_OOO,
    BLACK_CASTLING = BLACK_OO  | BLACK_OOO,
    ANY_CASTLING   = WHITE_CASTLING | BLACK_CASTLING
};

enum {
    NORTH = 8,
    EAST  = 1,
    SOUTH = -NORTH,
    WEST  = -EAST,

    NORTH_EAST = NORTH + EAST,
    SOUTH_EAST = SOUTH + EAST,
    SOUTH_WEST = SOUTH + WEST,
    NORTH_WEST = NORTH + WEST
};

// Fifty move rule
// Chesscom uses 50 moves automatic draw, lichess uses 75 moves automatic draw with the ability for either side to claim a draw after 50 moves
// If I'm playing perfect chess, I can assume the losing side to take a draw after 50 moves
// Meaning I employ the 50 move draw even though we are playing on lichess
#define HUNDRED_PLIES                   100 // fifty moves

#define RANK_1BB                        0x00000000000000FFULL
#define RANK_2BB                        (RANK_1BB << NUM_FILES)
#define RANK_3BB                        (RANK_2BB << NUM_FILES)
#define RANK_4BB                        (RANK_3BB << NUM_FILES)
#define RANK_5BB                        (RANK_4BB << NUM_FILES)
#define RANK_6BB                        (RANK_5BB << NUM_FILES)
#define RANK_7BB                        (RANK_6BB << NUM_FILES)
#define RANK_8BB                        (RANK_7BB << NUM_FILES)

#define FILE_ABB                        0x0101010101010101ULL
#define FILE_BBB                        (FILE_ABB << 1)
#define FILE_CBB                        (FILE_BBB << 1)
#define FILE_DBB                        (FILE_CBB << 1)
#define FILE_EBB                        (FILE_DBB << 1)
#define FILE_FBB                        (FILE_EBB << 1)
#define FILE_GBB                        (FILE_FBB << 1)
#define FILE_HBB                        (FILE_GBB << 1)

#define FR(file, rank)                  ((rank) * 8 + (file))
#define FILE(sq)                        ((sq) % 8)
#define RANK(sq)                        ((sq) / 8)
#define BIT(sq)                         (1ULL << (sq))    
#define PIECE_CHAR(side, piece_type)    (PIECE_CHARS[(side)*8 + (piece_type)])

/* move masks
0000 0000 0011 1111 -> Destination      dst         0x3F << 0
0000 1111 1100 0000 -> Source           src         0x3F << 6
0011 0000 0000 0000 -> Promotion piece  ppt         0x03 << 12
1100 0000 0000 0000 -> Special move     spc         0x03 << 14
*/

#define BITS(x)                         (__builtin_popcountll(x))
#define MOVE(src, dst, ppt, spc)        (Move)((dst) + ((src) << 6) + (((ppt) - KNIGHT) << 12) + (spc))

#define DST(move)                       (((move) & (0x3F << 0 )) >> 0 )
#define SRC(move)                       (((move) & (0x3F << 6 )) >> 6 )
#define PPT(move)                       (((move) & (0x03 << 12)) >> 12)
#define SPC(move)                       ((move) & (0x03 << 14))

typedef uint64_t U64;
typedef uint16_t Move;

typedef struct {
    Move move;
    int materialScore;
    int positionScore;
    int orderingBias;
} Move_s;

typedef struct {
    Move move;
    int captured;
    int castlingRights;
	int hundredPly;
	U64 enPas;
    U64 checkers;
    // U64 checkSquares[NUM_SIDES][NUM_PIECES];
    U64 kingBlockers[NUM_SIDES];
    int staticEval;
    U64 key;
} Undo_s;

typedef struct {
    int side;
    int castlingRights;
    int hundredPly;
    int hisPly;
    U64 enPas;

    int pieces[NUM_SQUARES];
    U64 byType[NUM_PIECES];
    U64 byColour[NUM_SIDES];

    U64 checkers;
    // U64 checkSquares[NUM_SIDES][NUM_PIECES]; // checkSquares[SIDE][EMPTY or KING] are undefined
    // But apparently it's part of the C standard to initialise undefined struct fields with 0 when using a designated initialiser!!! Awesome news
    U64 kingBlockers[NUM_SIDES];

    U64 key;

    int staticEval;

    Undo_s Undos[MAX_GAME_PLYS];
} Board_s;

static const char PIECE_CHARS[] = ".PNBRQK-.pnbrqk-"; // TODO: get this shit out of here
extern Board_s Board;

#endif
