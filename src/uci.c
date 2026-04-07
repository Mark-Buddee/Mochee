#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdatomic.h>
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
#include "worker.h"

static void handle_debug(char* line) {

    (void)line;

}

static void handle_isready() {

    printf("readyok\n");

}

static void handle_setoption(char* line) {

    (void)line;

}

static void handle_register(char* line) {
    
    (void)line;

}

static void handle_position(char* line) {
    
    // position fen fenstr
    // position startpos
    // ... moves e2e4 e7e5 b7b8q

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

    init_tt();
    handle_position("position startpos\n");

}

int get_searchtime(int wtime, int btime, int winc, int binc, int movestogo) {

    (void)winc;
    (void)binc;
    (void)movestogo;

    int ourtime   = Board.side == WHITE ? wtime : btime;
    // int theirtime = Board.side == WHITE ? btime : wtime;
    // int ourinc    = Board.side == WHITE ? winc  : binc;
    // int theirinc  = Board.side == WHITE ? binc  : winc;
    // int lead = ourtime - theirtime;

    // int searchtime = 0;
    // if(lead >= LARGE_LEAD) 
    //     searchtime = ourtime >= RAPID ? RAPID_MAX
    //                : ourtime >= BLITZ ? BLITZ_MAX
    //                :                    BULLET_MAX;

    // else if(lead >= LEAD)
    //     searchtime = ourtime >= RAPID ? RAPID_STANDARD
    //                : ourtime >= BLITZ ? BLITZ_STANDARD
    //                :                    BULLET_STANDARD;

    // else if(ourtime >= DANGER_TIME && ((theirtime - 60000)*3/8 + 15000 <= ourtime))
    //     searchtime = ourtime >= RAPID ? RAPID_RUSH
    //                : ourtime >= BLITZ ? BLITZ_RUSH
    //                :                    BULLET_RUSH;

    // else if(ourtime >= INSTANT_TIME && ((theirtime - 60000)*2/8 + 7500 <= ourtime))
    //     searchtime = DANGER;

    // else 
        // searchtime = ourtime / 50;

    // printf("duration: %d\n", duration);

    // return searchtime;

    return ourtime / 50;
}

static void handle_go(char* line) {

    // End previous search if there is one
    if(SearchInfo.active) {
        
        atomic_store(&SearchInfo.stop, true); // change this once pondering is implemented
        thrd_join(thrd, NULL);
        SearchInfo.active = false;
        
    }

    // Parse go command
    char *ptr = NULL;
    
    ptr = strstr(line,"wtime");
    int wtime     = ptr ? atoi(ptr + 6)  : 0;
    ptr = strstr(line,"btime");
    int btime     = ptr ? atoi(ptr + 6)  : 0;
    ptr = strstr(line,"winc");
    int winc      = ptr ? atoi(ptr + 5)  : 0;
    ptr = strstr(line,"binc");
    int binc      = ptr ? atoi(ptr + 5)  : 0;
    ptr = strstr(line,"movestogo");
    int movestogo = ptr ? atoi(ptr + 10) : -1;
    ptr = strstr(line,"depth");
    int depth     = ptr ? atoi(ptr + 6)  : -1;
    ptr = strstr(line,"nodes");
    int nodes     = ptr ? atoi(ptr + 6)  : -1;
    ptr = strstr(line,"mate");
    int mate      = ptr ? atoi(ptr + 5)  : -1;
    ptr = strstr(line,"movetime");
    int movetime  = ptr ? atoi(ptr + 9)  : -1;
    
    bool ponder   = strstr(line,"ponder")   ? true : false;
    bool infinite = strstr(line,"infinite") ? true : false;
    
    Move searchmoves[MAX_MOVES] = {NULL_MOVE}; // Not implemented yet
    int num_searchmoves = 0;

    // Start worker thread
    SearchInfo.ply        = Board.hisPly;
    SearchInfo.endtime    = clock() + (long)(get_searchtime(wtime, btime, winc, binc, movestogo) * CLOCKS_PER_SEC) / 1000;

    SearchInfo.active     = true;
    atomic_store(&SearchInfo.stop, false);
    
    SearchInfo.ponder     = ponder;
    SearchInfo.infinite   = infinite;
    SearchInfo.depth      = depth;
    SearchInfo.nodes      = nodes;
    SearchInfo.mate       = mate;
    SearchInfo.movetime   = movetime;

    memcpy(SearchInfo.searchmoves, searchmoves, num_searchmoves * sizeof(Move));

    // thread_func(NULL);
    thrd_create(&thrd, thread_func, NULL);

}

static void handle_stop(void) {

    atomic_store(&SearchInfo.stop, true);
    
    if(SearchInfo.active) {

        thrd_join(thrd, NULL);
        SearchInfo.active = false;

    }

}

static void handle_ponderhit(void) {

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

        // printf("%s", line);

        if      (!strncmp(line, "debug",       5)) handle_debug(line + 6);
        else if (!strncmp(line, "isready",     7)) handle_isready();
        else if (!strncmp(line, "setoption",   9)) handle_setoption(line + 10);
        else if (!strncmp(line, "register",    8)) handle_register(line);
        else if (!strncmp(line, "ucinewgame", 10)) handle_ucinewgame();
        else if (!strncmp(line, "position",    8)) handle_position(line + 9);
        else if (!strncmp(line, "go",          2)) handle_go(line + 3);
        else if (!strncmp(line, "stop",        4)) handle_stop();
        else if (!strncmp(line, "ponderhit",   9)) handle_ponderhit();
        else if (!strncmp(line, "quit",        4)) break;

    }

}
