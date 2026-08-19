#ifndef WORKER_H
#define WORKER_H

#include "defs.h"

int mock_alpha_beta(Board_s* const Board, int alpha, int beta, int depth, Move* rootBestMove);
int thread_func(void* arg);

#endif