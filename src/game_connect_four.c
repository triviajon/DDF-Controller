#include "game_connect_four.h"



void game_connect_four_init(void *game_connect_four, uint8_t color_frame[LED_ROWS][LED_COLS][LED_CHANNELS]) {
    struct game_connect_four *game = (struct game_connect_four *) game_connect_four;

    game->turn = 0;
    game->board = 0;
    game->board_in_use = 0;
}

void game_connect_four_loop(void *game_connect_four, uint8_t color_frame[LED_ROWS][LED_COLS][LED_CHANNELS]) {
    // update 
}


