#ifndef CONSOLE_H
#define CONSOLE_H

#define BACK_SPACE   "\033[D"
#define LINE_START   "\033[F"
#define SCREEN_CLEAR "\033[2J"

void src2str(char srcStr[], int src);
int str2src(char string[]);

int get_ppt(char string[]);

void console(void);

#endif
