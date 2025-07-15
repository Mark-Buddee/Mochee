#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "uci.h"
#include "defs.h"
#include "tt.h"
#include "board.h"
#include "console.h"
#include "search.h"
#include "move.h"
#include "gen.h"
#include "magic.h"
#include "init.h"
#include "debug.h"

static void handle_debug(char* line) {
    (void)line;
    return;
}

static void handle_isready() {
    printf("readyok\n");
}

static void handle_setoption(char* line) {
    (void)line;
    return;
}

static void handle_register(char* line) {
    (void)line;
    return;
}

// position fen fenstr
// position startpos
// ... moves e2e4 e7e5 b7b8q
static void handle_position(char* line) {

	line += 9;
    char *ptrChar = line;

    // char start_fen_s[] = START_FEN;

    if(strncmp(line, "startpos", 8) == 0){
        Board = board_init(START_FEN);

    } else {
        ptrChar = strstr(line, "fen");
        if(ptrChar == NULL) {
            Board = board_init(START_FEN);
        } else {
            ptrChar+=4;
            Board = board_init(ptrChar);
        }
    }

	ptrChar = strstr(line, "moves");

	if(ptrChar != NULL) {
        ptrChar += 6;
        while(*ptrChar) {
            int ppt = get_ppt(ptrChar);
            int src = str2src(ptrChar++);
            int dst = str2src(++ptrChar);

            Move_s List[MAX_MOVES];
            Move_s* cur = List;
            Move_s* end = gen_legal(&Board, List);
            score_moves(&Board, List, end, NULL_MOVE);

            while(cur != end) {
                // printf("PPT: %d\n", PPT(cur->move));
                if(SRC(cur->move) == src && DST(cur->move) == dst && PPT(cur->move) == ppt) {
                    // move = cur->move;
                    // MoveSToMake->moveVal = move_eval(&Board, MoveSToMake->move);
                    break;
                }
                cur++;
            }
            if(cur->move == NULL_MOVE) {
                printf("move not found.\n");
                break;
            }

            do_move(&Board, cur);

            while(*ptrChar && *ptrChar!= ' ' && *ptrChar!= '\n') ptrChar++;
            ptrChar++;
        }
    }
	// PrintBoard(pos);
}

static void handle_ucinewgame(void) {
    handle_position("position startpos\n");
    return;
}

// go movetime 10000
// go depth 6 wtime 180000 btime 100000 binc 1000 winc 1000 movetime 1000 movestogo 40
static void handle_go(char* line) {

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
		if(Board.side == WHITE) ourInc = atoi(ptr + 5);
        else theirInc = atoi(ptr + 5);
	}

	if ((ptr = strstr(line,"binc"))) {
		if(Board.side == WHITE) theirInc = atoi(ptr + 5);
        else ourInc = atoi(ptr + 5);
	}

	if ((ptr = strstr(line,"wtime"))) {
		if(Board.side == WHITE) ourTime = atoi(ptr + 6);
        else theirTime = atoi(ptr + 6);
	}

	if ((ptr = strstr(line,"btime"))) {
		if(Board.side == WHITE) theirTime = atoi(ptr + 6);
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
        duration = INSTANT; // TODO: Do not allow this to be zero unless I explicitly test it

    // printf("duration: %g\n", duration);

	Move bestMove = iterative_deepening(&Board, duration);
    printf("bestmove ");
    print_move(bestMove);
    printf("\n");
    inc_age();
}

static void handle_stop(void) {
    return;
}

static void handle_ponderhit(void) {
    return;
}

static void handle_quit(void) {
    return;
}

void uci(void) {
    
    printf("id name %s %s\n", NAME, VERSION);
    printf("id author %s\n", AUTHOR);
    printf("uciok\n");
    
    init_all();

	char line[STREAM_BUFF_SIZE];

	while (1) {

        if (!fgets(line, STREAM_BUFF_SIZE, stdin))  continue;

        printf("%s", line);

        if      (!strncmp(line, "debug",       5)) handle_debug(line);
        else if (!strncmp(line, "isready",     7)) handle_isready();
        else if (!strncmp(line, "setoption",   9)) handle_setoption(line);
        else if (!strncmp(line, "register",    8)) handle_register(line);
        else if (!strncmp(line, "ucinewgame", 10)) handle_ucinewgame();
        else if (!strncmp(line, "position",    8)) handle_position(line);
        else if (!strncmp(line, "go",          2)) handle_go(line);
        else if (!strncmp(line, "stop",        4)) handle_stop();
        else if (!strncmp(line, "ponderhit",   9)) handle_ponderhit();
        else if (!strncmp(line, "quit",        4)) {
            handle_quit();
            break;
        }
    }
}
