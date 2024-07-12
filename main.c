#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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

// Hello Github!


int main(void) {
    print_version();
    init_all();
    init_magic();
    init_zobrist();
    init_tt();

    // printf("%g", (double)(TT_SIZE_MB)*1024*1024/sizeof(TTEntry_s));

    // char start_fen_s[] = START_FEN;
    // Board_s Board = board_init(start_fen_s);
    // printf("Zobrist original: %llx\n", Board.key);
    // do_search(&Board, 9);
    // printf("Zobrist after:    %llx\n", Board.key);
    perft_unit_test();
    // debug();

    return 0;
}
