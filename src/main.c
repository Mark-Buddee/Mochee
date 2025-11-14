#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stddef.h>
#include "console.h"
#include "defs.h"
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

// unsigned long long tableHits;
// unsigned long long tableUpdates;
// unsigned long long tableOverwrites;

Board_s Board;

// Struct {
// 	isReady,
//  debug,
// 
// }

static void print_version(void) {
    printf("\n%s %s %s\n\n", NAME, VERSION, NAME_DESC);
}

int main(void) {

	// unsigned long long TT_SIZE_MB = 512ULL;
	unsigned long long TT_SIZE_MB = 64ULL;
	unsigned long long bytes = TT_SIZE_MB * 1024ULL * 1024ULL;

	TTEntries = bytes / sizeof(TTEntry_s);
	TT = malloc(TTEntries * sizeof(TTEntry_s));

	
    if (!TT) {
        fprintf(stderr, "Failed to allocate transposition table!\n");
        return 1;
    }

	// printf("TTEntries: %llu\n", TT_ENTRIES);
	printf("TT intended entries: %llu\n", TTEntries);
	printf("TT actual bytes:     %zu\n", TTEntries * sizeof(TTEntry_s));
	printf("TT actual size (MB): %llu\n",
       (unsigned long long)(TTEntries * sizeof(TTEntry_s)) / (1024ULL * 1024ULL));
	printf("Size of each entry:  %zu\n", sizeof(TTEntry_s));

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
