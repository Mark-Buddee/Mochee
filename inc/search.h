#ifndef SEARCH_H
#define SEARCH_H

#define LOWER 0
#define UPPER 1
#define ASPIRATION_MARGIN 0
#define ASPIRATION_TT_MARGIN 60
#define DEPTH_2_MARGIN 75

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
int alpha_beta(Board_s* const Board, int alpha, int beta, int depth, Move* rootBestMove);
void do_search(Board_s* const Board, int depth);
void score_moves(Board_s* Board, Move_s* cur, Move_s* end, Move bestMove);
Move iterative_deepening(Board_s* const Board, double maxDuration);
void get_aspiration_window(int prevScores[], int depth, int* lower, int* upper);

int is_three_fold(Board_s* Board, int rootPly);

void partial_insertion_sort(Move_s* begin, Move_s* end, int limit);

int quiesce(Board_s* const Board, int alpha, int beta, int rootPly);

#endif