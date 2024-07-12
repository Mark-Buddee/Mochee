#ifndef SEARCH_H
#define SEARCH_H

#define INSERTION_SORT_MIN 30

#define TABLE_BEST_BIAS 1000
#define PROMOTION_BIAS 300
#define WINNING_CAPTURE_BIAS 250
#define LOSING_CAPTURE_BIAS 150

#define DELTA_VAL 150

int clamp(int lower, int upper, int score);
int alpha_beta(Board_s* const Board, int alpha, int beta, int depth, clock_t endTime);
void do_search(Board_s* const Board, int depth);
Move iterative_deepening(Board_s* const Board, double duration);

#endif