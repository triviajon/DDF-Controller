#ifndef GAME_CONNECT_FOUR_H
#define GAME_CONNECT_FOUR_H

#include <stdint.h>
#include <stdio.h>

#include "global_defines.h"


// A structure representing a standard game of Connect Four played on a 7-column-wide, 6-row-high grid.
// Intended to played by two players, player 1 plays when `turn` = 0, player 2 plays otherwise
// `board` holds the state of the board while `board_in_use` stores which grid spaces are
// currently occupied. To be intrepeted as follows:
// The least significant 8 bits of each of `board` and `board_in_use` correspond the bottom row,
// which is 6 wide, followed by 2 blanks (always to be 0). The next row corresponds to the next 8 bits.
struct game_connect_four {
    /* Use the game's struct to store game state info */
    uint8_t turn;
    uint64_t board;
    uint64_t board_in_use;
};

void game_connect_four_init(void *game_connect_four, uint8_t color_frame[LED_ROWS][LED_COLS][LED_CHANNELS]);
void game_connect_four_loop(void *game_connect_four, uint8_t color_frame[LED_ROWS][LED_COLS][LED_CHANNELS]);

#endif

