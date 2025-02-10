#ifndef MOVE_H
#define MOVE_H

void inc_posFreq(Board_s* const Board);

void dec_posFreq(Board_s* const Board);

void do_move(Board_s* const Board, const Move_s* cur);

void undo_move(Board_s* const Board);

#endif
