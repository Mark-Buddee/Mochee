#ifndef TGUI_H
#define TGUI_H

void print_version(void);

void print_bitBoard(U64 bitBoard);

void print_board(const Board_s* Board, int flipped);

void src2str(char srcStr[], int src);

int tgui_main(void);

void debug(void);

#endif
