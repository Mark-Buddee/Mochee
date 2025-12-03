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
#include "debug.h"

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

	/*
	    8 MB    num hits:  917434, num updates: 386877, num overwrites: 1985488
	    16 MB   num hits:  998233, num updates: 461563, num overwrites: 2042031
	    32 MB   num hits: 1046749, num updates: 511011, num overwrites: 2050407
		64 MB   num hits: 1070219, num updates: 539126, num overwrites: 2046281
		128 MB  num hits: 1083295, num updates: 555138, num overwrites: 2044203
		256 MB  num hits: 1090408, num updates: 563180, num overwrites: 2044322 
		512 MB  num hits: 1093610, num updates: 567112, num overwrites: 2044082
		1024 MB num hits: 1095901, num updates: 569098, num overwrites: 2044029
		2048 MB num hits: 1096541, num updates: 569927, num overwrites: 2043199
		4096 MB num hits: 1096976, num updates: 570400, num overwrites: 2043243
	*/

	unsigned long long TT_SIZE_MB = 512ULL;
	unsigned long long bytes = TT_SIZE_MB * 1024ULL * 1024ULL;
	// unsigned long long bytes = 64;

	TTEntries = bytes / sizeof(TTEntry_s);
	TT = malloc(TTEntries * sizeof(TTEntry_s));
    if (!TT) {
        fprintf(stderr, "Failed to allocate transposition table!\n");
        return 1;
    }

	// printf("TTEntries: %llu\n", TT_ENTRIES);
	// printf("TT intended entries: %llu\n", TTEntries);
	// printf("TT actual bytes:     %zu\n", TTEntries * sizeof(TTEntry_s));
	// printf("TT actual size (MB): %llu\n",
    //    (unsigned long long)(TTEntries * sizeof(TTEntry_s)) / (1024ULL * 1024ULL));
	// printf("Size of each entry:  %zu\n", sizeof(TTEntry_s));

    // int num1 = 200;
	// uint8_t num2 = 0;
    // printf("Number 1: %d, Number 2: %d\n", num1, num2);
	// printf("Saved as uint8_t: %u\n", (uint8_t)num1);
	// int8_t ageDifference = (int8_t)((uint8_t)num1 - num2);
    // printf("Age difference: %d\n", ageDifference);

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

	#ifndef NDEBUG
		printf("num hits: %lld, num updates: %lld, num overwrites: %lld\n", TTStats.hits, TTStats.updates, TTStats.overwrites);
		printf("nodes searched: %llu\n", nodesSearched);
	#endif
}
