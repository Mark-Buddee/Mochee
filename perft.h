#ifndef PERFT_H
#define PERFT_H

unsigned long long num_nodes(Board_s* const Board, const int depth);

void perft(Board_s* const Board, const int depth);

void perft_unit_test(void);

#endif