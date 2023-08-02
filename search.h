#ifndef SEARCH_H
#define SEARCH_H

int alpha_beta(Board_s* const Board, int alpha, int beta, int depth, const int maxDepth, int* bestMove);
// int alpha_beta(Board_s* const Board, int alpha, int beta, int depth);
void do_search(Board_s* const Board, int depth);

#endif