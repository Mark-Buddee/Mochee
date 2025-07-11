#ifndef MAGIC_H
#define MAGIC_H

U64 rand64(void);

void init_magic(void);

U64 gen_bishop_magic_attacks(int src, U64 occ);
U64 gen_rook_magic_attacks(int src, U64 occ);
U64 gen_queen_magic_attacks(int src, U64 occ);

#endif