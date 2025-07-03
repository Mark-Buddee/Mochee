#include <stdlib.h>
#include <stdio.h>
#define NDEBUG
#include <assert.h>
#include <time.h>
#include <string.h>
#include "inc/defs.h"
#include "inc/magic.h"
#include "inc/bitboard.h"
#include "inc/tgui.h"

enum {BISHOP_IDX, ROOK_IDX};
const int magic_shift[2][64] = {
    {
        5, 4, 5, 5, 5, 5, 4, 5, // 4800 entries of U64 in attack table
        4, 4, 5, 5, 5, 5, 4, 4,
        4, 4, 7, 7, 7, 7, 4, 4,
        5, 5, 7, 9, 9, 7, 5, 5,
        5, 5, 7, 9, 9, 7, 5, 5,
        4, 4, 7, 7, 7, 7, 4, 4,
        4, 4, 5, 5, 5, 5, 4, 4,
        5, 4, 5, 5, 5, 5, 4, 5
    }, {
        12, 11, 11, 11, 11, 11, 11, 12, // 88064 entries of U64 in attack table
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        10,  9,  9,  9,  9,  9,  9, 10,
        11, 10, 10, 10, 10, 11, 10, 11
    }
};
const U64 magics[2][64] = {
    {
        0xbce126a1a89c3ffc, 0x2a0b032a5cbffe5b, 0xf5904c0d43c792cf, 0x04180607c0b010de, 0xc3c40c21e0675540, 0x1002021004200900, 0x17610b8c57ff59eb, 0xc8ab126cec177fe2,
        0x0432b68d2561e7f0, 0x67cfa5cfdcf6b3f3, 0x62b3f081898304a0, 0x538ddc04058c3cf4, 0x913f141c209ab59f, 0x7ddf0f0c200c988f, 0x1253f7630c0a7ffd, 0x26c584d8f971fef4,
        0x86c058f66c74afe5, 0xcb20013ab53b37f3, 0x2ab00f2802c145b8, 0xd35405c80253211d, 0x3f9c000e81a05f65, 0xd12e019b0107252b, 0x0fd400df42a63fc1, 0x905a012e5ac4df8a,
        0x87c806bfa6e0685d, 0xca52576b905000e1, 0x3d98c80650008218, 0x004a404004010200, 0x1a01010030c44004, 0x559b020000c05010, 0x4845131142080105, 0x88e2cc4015130802,
        0x0042104000100210, 0x1002021120200300, 0x0880202800140804, 0x422040c801098200, 0x0040024010010100, 0x8950100040002401, 0x000c010410120980, 0x00080082a0010120,
        0xe67fe58cb6174010, 0xcb3ff3257b16602a, 0x0062202028001000, 0x4880c4a011000804, 0x0000200414000040, 0x0291300100404201, 0xf53fb65aaf52d402, 0xf03fbb2d5fa01202,
        0x22bff345bab92f18, 0xe14ffb77366944ed, 0x63ddafa7df485064, 0x9290bb0c8404019a, 0xfca1037d9f888111, 0x798e7e98cdf385bd, 0x027fc9a657fb6769, 0xda7fb778fed55c9c,
        0x0147ff497cf2ae06, 0x3b736bfedcf084bf, 0xe2153bc3f566e521, 0x4c8f6242e0618825, 0xb910fbc0a0c4240d, 0x9d2f707d602bf30a, 0x2f3f7fdfdb24ecb5, 0x2cffbb1e12e9b5f5
    }, {
        0x008008a080c00010, 0x5940004630002000, 0x010010e000c00900, 0x0880051000080080, 0x9200220008900420, 0x05000624003b0008, 0x0c000488010c5006, 0x9a800100004d2280,
        0x2409800080204000, 0x4008c01000c0a000, 0x0001001820010040, 0x0029001000090022, 0xc102001022000844, 0x8282001005260048, 0xa824000944100288, 0x5222001244008621,
        0x0880004010402000, 0x000a020020c10181, 0x0001120020820140, 0x1024420012000920, 0x71220200100c3920, 0x7908808044007200, 0x04a014001e089310, 0x00100a0010440087,
        0x0000410100208008, 0x1040200540100040, 0x0090405500200100, 0x4500210100500128, 0x4828012880040080, 0x642e0002000410c8, 0x23310085000c5600, 0x004405060000ea84,
        0xc775400c20800084, 0x0000886000804004, 0x8025200101001940, 0x00010028e1001000, 0x800201040a001020, 0x0044010040402a00, 0x0422810204001048, 0x2280008102000064,
        0x028000c000808021, 0x011008a002424000, 0x4001200104c30012, 0x0540201a00320040, 0x0d00080004008080, 0x8a06008004008100, 0x2200820001008080, 0x0400014184020001,
        0x48FFFE99FECFAA00, 0x48FFFE99FECFAA00, 0x497FFFADFF9C2E00, 0x613FFFDDFFCE9200, 0xffffffe9ffe7ce00, 0xfffffff5fff3e600, 0x0003FF95E5E6A4C0, 0x510FFFF5F63C96A0,
        0xEBFFFFB9FF9FC526, 0x61FFFEDDFEEDAEAE, 0x53BFFFEDFFDEB1A2, 0x127FFFB9FFDFB5F6, 0x411FFFDDFFDBF4D6, 0x0001000208040001, 0x0003FFEF27EEBE74, 0x7645FFFECBFEA79E
    }
};
U64 inner_6[2][64];
U64 attack_table[92864]; // 725.5 KB
U64* atkPtr[2][NUM_SQUARES];

void init_rand(void) {
    // time_t t;
    // srand((unsigned) time(&t));
    srand(510474708); // TODO: random seed makes the engine unexploitable
}

U64 rand64(void) {
    U64 s = 0;
    for (int i = 0; i < 64; i += 15)
        s |= (U64)rand() << i;
    return s;
}

U64 rand_sparse(void) {
    return rand64() & rand64() & rand64();
}

void init_inner_6() {
    for(int sq = A1; sq <= H8; sq++) {
        U64 bishopAtt = gen_bishop_slides(sq, 0);
        U64 rookAtt = gen_rook_slides(sq, 0);
        if(!(FILE_ABB & BIT(sq))) {
            bishopAtt &= ~FILE_ABB;
            rookAtt   &= ~FILE_ABB;
        }
        if(!(FILE_HBB & BIT(sq))) {
            bishopAtt &= ~FILE_HBB;
            rookAtt   &= ~FILE_HBB;
        }
        if(!(RANK_1BB & BIT(sq))) {
            bishopAtt &= ~RANK_1BB;
            rookAtt   &= ~RANK_1BB;
        }
        if(!(RANK_8BB & BIT(sq))) {
            bishopAtt &= ~RANK_8BB;
            rookAtt   &= ~RANK_8BB;
        }
        inner_6[BISHOP_IDX][sq] = bishopAtt;
        inner_6[ROOK_IDX][sq] = rookAtt;
    }
}

void gen_occ(U64* occ, U64 relOcc) {
    U64 out = relOcc;
    while(out) {
        *occ++ = out;
        U64 dup = relOcc;
        while(1) {
            U64 bit = BIT(pop_lsb(&dup));
            if(out & bit) {
                out ^= bit;
                break;
            } else out |= bit;
        }
    }
    *occ++ = out;
}

void gen_att(U64 att[], U64 occ[], int numOcc, int pieceIdx, int sq) {
    for(int i = 0; i < numOcc; i++)
        att[i] = pieceIdx == BISHOP_IDX ? gen_bishop_slides(sq, occ[i]) : gen_rook_slides(sq, occ[i]);
}

void init_magic(void) {
    init_rand();
    init_inner_6();
    int atkIdx = 0;
    for(int pieceIdx = BISHOP_IDX; pieceIdx <= ROOK_IDX; pieceIdx++) {
        for(int sq = A1; sq <= H8; sq++) {
            U64 occ[4096];
            U64 att[4096];
            U64 map[4096];
            U64 relOcc = inner_6[pieceIdx][sq];
            int numOcc = 1 << BITS(relOcc);
            gen_occ(occ, relOcc);
            gen_att(att, occ, numOcc, pieceIdx, sq);

            U64 magic;
            int badCollision;
            do {
                memset(map, -1, 4096*sizeof(U64));
                magic = magics[pieceIdx][sq] ? magics[pieceIdx][sq] : rand_sparse();

                badCollision = 0;
                for(int i = 0; i < numOcc; i++) {
                    int idx = (occ[i] * magic) >> (64 - magic_shift[pieceIdx][sq]);
                    if(map[idx] == att[i]) continue;
                    if(map[idx] == -1ULL ) {
                        map[idx] = att[i];
                        continue;
                    }
                    badCollision = 1;
                    if(magics[pieceIdx][sq]) {
                        printf("Warning. Bad magic %llx at pieceIdx: %d square: %d\n", magics[pieceIdx][sq], pieceIdx, sq);
                        exit(1);
                    }
                    break;
                }
            } while(badCollision);
            atkPtr[pieceIdx][sq] = &(attack_table[atkIdx]);
            int numMapEntries = 1 << magic_shift[pieceIdx][sq];
            atkIdx += numMapEntries;
            memcpy(atkPtr[pieceIdx][sq], map, numMapEntries*sizeof(U64));
        }
    }
}

U64 gen_bishop_magic_attacks(int src, U64 occ) {
    int idx = (occ & inner_6[BISHOP_IDX][src]) * magics[BISHOP_IDX][src] >> (64 - magic_shift[BISHOP_IDX][src]);
    return atkPtr[BISHOP_IDX][src][idx];
}

U64 gen_rook_magic_attacks(int src, U64 occ) {
    int idx = (occ & inner_6[ROOK_IDX][src]) * magics[ROOK_IDX][src] >> (64 - magic_shift[ROOK_IDX][src]);
    return atkPtr[ROOK_IDX][src][idx];
}

U64 gen_queen_magic_attacks(int src, U64 occ) {
    return gen_bishop_magic_attacks(src, occ) | gen_rook_magic_attacks(src, occ);
}
