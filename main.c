#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stddef.h>

#include "inc/defs.h"
#include "inc/tgui.h"
#include "inc/init.h"
#include "inc/board.h"
#include "inc/gen.h"
#include "inc/move.h"
#include "inc/perft.h"
#include "inc/bitboard.h"
#include "inc/search.h"
#include "inc/magic.h"
#include "inc/tt.h"
#include "inc/uci.h"

unsigned long long tableHits;
unsigned long long tableUpdates;
unsigned long long tableOverwrites;

int main(void) {

	// printf("%lld\n", 5486823173478921566 % 181117672);
	// printf("%lld\n", 12554450795584811879 % 181117672);

    print_version();
    printf("Welcome to Mochee! Type 'console' for console mode\n");

    // char start_fen_s[] = START_FEN;

    // Board_s Board = board_init(start_fen_s);
    // Board_s Board = board_init(START_FEN);

	// console();
    char line[STREAM_BUFF_SIZE];
	while (1) {
		memset(&line[0], 0, sizeof(line));

		fflush(stdout);
		if (!fgets(line, STREAM_BUFF_SIZE, stdin)) continue;
		if (line[0] == '\n') continue;
		
		if (!strncmp(line, "console",7)) {
			console();
			break;
        }
		if (!strncmp(line, "uci",3)) {
			uci();
			break;
        }
	}

    printf("num hits: %lld, num updates: %lld, num overwrites: %lld\n", tableHits, tableUpdates, tableOverwrites);
}
