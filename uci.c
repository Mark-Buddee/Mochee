#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "inc/uci.h"
#include "inc/defs.h"
#include "inc/tt.h"
#include "inc/board.h"
#include "inc/tgui.h"
#include "inc/search.h"
#include "inc/move.h"
#include "inc/gen.h"
#include "inc/magic.h"
#include "inc/init.h"

// go depth 6 wtime 180000 btime 100000 binc 1000 winc 1000 movetime 1000 movestogo 40
void ParseGo(char* line, Board_s *Board) {

	// int depth = -1, movestogo = 30,movetime = -1;
	// int wtime = -1, btime = -1, winc = 0, binc = 0;
    char *ptr = NULL;
    int ourTime = 0;
    int theirTime = 0;
    int ourInc = 0;
    int theirInc = 0;
	// info->timeset = FALSE;

	// if ((ptr = strstr(line,"infinite"))) {
	// 	;
	// }

	if ((ptr = strstr(line,"winc"))) {
		if(Board->side == WHITE) ourInc = atoi(ptr + 5);
        else theirInc = atoi(ptr + 5);
	}

	if ((ptr = strstr(line,"binc"))) {
		if(Board->side == WHITE) theirInc = atoi(ptr + 5);
        else ourInc = atoi(ptr + 5);
	}

	if ((ptr = strstr(line,"wtime"))) {
		if(Board->side == WHITE) ourTime = atoi(ptr + 6);
        else theirTime = atoi(ptr + 6);
	}

	if ((ptr = strstr(line,"btime"))) {
		if(Board->side == WHITE) theirTime = atoi(ptr + 6);
        else ourTime = atoi(ptr + 6);
	}
    // printf("us: %d, them: %d\n", ourTime, theirTime);
    // printf("ourInc: %d, theirInc: %d\n", ourInc, theirInc);

	// if ((ptr = strstr(line,"movestogo"))) {
	// 	movestogo = atoi(ptr + 10);
	// }

	// if ((ptr = strstr(line,"movetime"))) {
	// 	movetime = atoi(ptr + 9);
	// }

	// if ((ptr = strstr(line,"depth"))) {
	// 	depth = atoi(ptr + 6);
	// }

	// if(movetime != -1) {
	// 	time = movetime;
	// 	movestogo = 1;
	// }

	// info->starttime = GetTimeMs();
	// info->depth = depth;

	// if(time != -1) {
	// 	info->timeset = TRUE;
	// 	time /= movestogo;
	// 	time -= 50;
	// 	info->stoptime = info->starttime + time + inc;
	// }

	// if(depth == -1) {
	// 	info->depth = MAXDEPTH;
	// }

	// printf("time:%d start:%d stop:%d depth:%d timeset:%d\n",
	// 	time,info->starttime,info->stoptime,info->depth,info->timeset);
    int diff = ourTime - theirTime;
    double duration;

    if(diff >= LARGE_LEAD) 
        duration = ourTime >= RAPID ? RAPID_MAX
                 : ourTime >= BLITZ ? BLITZ_MAX
                 :                    BULLET_MAX;
    else if(diff >= LEAD)
        duration = ourTime >= RAPID ? RAPID_STANDARD
                 : ourTime >= BLITZ ? BLITZ_STANDARD
                 :                    BULLET_STANDARD;
    else if(ourTime >= 15000 && ((theirTime - 60000)*3/8 + 15000 <= ourTime))
        duration = ourTime >= RAPID ? RAPID_RUSH
                 : ourTime >= BLITZ ? BLITZ_RUSH
                 :                    BULLET_RUSH;
    else if(ourTime >= 7500 && ((theirTime - 60000)*2/8 + 7500 <= ourTime))
        duration = DANGER;
    else 
        duration = INSTANT;

    // printf("duration: %g\n", duration);

	Move bestMove = iterative_deepening(Board, duration);
    printf("bestmove ");
    print_move(bestMove);
    printf("\n");
}


// position fen fenstr
// position startpos
// ... moves e2e4 e7e5 b7b8q
void ParsePosition(char* line, Board_s* Board) {

	line += 9;
    char *ptrChar = line;

    char start_fen_s[] = START_FEN;

    if(strncmp(line, "startpos", 8) == 0){
        *Board = board_init(start_fen_s);

    } else {
        ptrChar = strstr(line, "fen");
        if(ptrChar == NULL) {
            *Board = board_init(start_fen_s);
        } else {
            ptrChar+=4;
            *Board = board_init(ptrChar);
        }
    }

	ptrChar = strstr(line, "moves");
	Move move = NULL_MOVE;

	if(ptrChar != NULL) {
        ptrChar += 6;
        while(*ptrChar) {
            int ppt = get_ppt(ptrChar);
            int src = str2src(ptrChar++);
            int dst = str2src(++ptrChar);

            Move_s List[MAX_MOVES];
            Move_s* cur = List;
            Move_s* end = gen_legal(Board, List);
            while(cur != end) {
                // printf("PPT: %d\n", PPT(cur->move));
                if(SRC(cur->move) == src && DST(cur->move) == dst && PPT(cur->move) == ppt) {
                    move = cur->move;
                    // MoveSToMake->moveVal = move_eval(&Board, MoveSToMake->move);
                    break;
                }
                cur++;
            }
            if(move == NULL_MOVE) {
                printf("move not found.\n");
                break;
            }

            do_move(Board, move);

            while(*ptrChar && *ptrChar!= ' ') ptrChar++;
            ptrChar++;
        }
    }
	// PrintBoard(pos);
}


void uci(void) {
    init_all();
    Board_s Board;

	setbuf(stdin, NULL);
    setbuf(stdout, NULL);

	char line[STREAM_BUFF_SIZE];
    printf("id name %s\n", NAME);
    printf("id author %s\n", AUTHOR);
    printf("uciok\n");

	while (1) {
		memset(&line[0], 0, sizeof(line));
        fflush(stdout);

        if (!fgets(line, STREAM_BUFF_SIZE, stdin))  continue;
        if (line[0] == '\n')                        continue;

        if      (!strncmp(line, "isready",     7))  printf("readyok\n");
        else if (!strncmp(line, "position",    8))  ParsePosition(line, &Board);
        else if (!strncmp(line, "ucinewgame", 10))  ParsePosition("position startpos\n", &Board);
        else if (!strncmp(line, "go",          2))  ParseGo(line, &Board);
        else if (!strncmp(line, "quit",        4))  break;
    }
}
