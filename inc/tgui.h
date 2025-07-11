#ifndef TGUI_H
#define TGUI_H

#define BACK_SPACE   "\033[D"
#define LINE_START   "\033[F"
#define SCREEN_CLEAR "\033[2J"

void print_version(void);
void print_bitBoard(U64 bitBoard);
void print_detailed(const Board_s* const Board, int flipped);
void print_pgn(const Board_s* const Board);
void print_board(const Board_s* const Board, int flipped);
void print_variation(Board_s* const Board, int maxDepth);

void src2str(char srcStr[], int src);
int str2src(char string[]);

int get_ppt(char string[]);
void print_move(Move move);

int tgui_main(void);
void console(void);

#endif
