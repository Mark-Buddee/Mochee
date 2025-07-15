#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stddef.h>
#include "defs.h"
#include "console.h"
#include "init.h"
#include "board.h"
#include "gen.h"
#include "move.h"
#include "perft.h"
#include "bitboard.h"
#include "search.h"
#include "magic.h"
#include "tt.h"
#include "uci.h"
#include "tinycthread.h"

unsigned long long tableHits;
unsigned long long tableUpdates;
unsigned long long tableOverwrites;

Board_s Board;

// Struct {
// 	isReady,
//  debug,
// 
// }

int main(void) {
	
	// printf("%lld\n", 5486823173478921566 % 181117672);
	// printf("%lld\n", 12554450795584811879 % 181117672);

	setbuf(stdout, NULL);

    print_version();
    printf("Welcome to Mochee! Type 'console' for console mode\n");

    char line[STREAM_BUFF_SIZE];

	while (1) {

		if (!fgets(line, STREAM_BUFF_SIZE, stdin)) continue;
		
		if (!strncmp(line, "console", 7)) {
			console();
			break;
        }
		if (!strncmp(line, "uci", 3)) {
			uci();
			break;
        }

		printf("Unknown command: %s", line);
	}

    // printf("num hits: %lld, num updates: %lld, num overwrites: %lld\n", tableHits, tableUpdates, tableOverwrites);
}
