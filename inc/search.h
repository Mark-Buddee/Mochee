#ifndef SEARCH_H
#define SEARCH_H

#define INSERTION_SORT_MIN 30

#define TABLE_BEST_BIAS 1000
#define PROMOTION_BIAS 300

#define CAPTURE_BIAS 100
#define WINNING_CAPTURE_BIAS 100
#define EQUAL_CAPTURE_BIAS 10
#define CAPTURED_PIECE_MULT 0.15
#define MOVED_PIECE_PENALTY_MULT 20
#define CHECK_BIAS 80

#define DELTA_VAL 150 // investigate capture even if after the capture we are down DELTA_VAL points in material
#define QUIET_DELTA_VAL 150 // investigate quiet checks even if we are QUIET_DELTA_VAL down in material
#define MAX_QUIET_CHECK_PLIES 1 // quiet checks are allowed within the first X plies of quiesce

int clamp(int lower, int upper, int score);
int alpha_beta(Board_s* const Board, int alpha, int beta, int depth, int rootPly, clock_t endTime);
void do_search(Board_s* const Board, int depth);
void score_moves(Board_s* Board, Move_s* cur, Move_s* end, Move bestMove);
Move iterative_deepening(Board_s* const Board, double duration);

#endif