#include "game_connect_four.h"

const NUM_ROWS = 6;
const NUM_COLUMNS = 7;
const WIN_LENGTH = 4;
typedef enum {GAME_BOARD, IN_USE_BOARD} BoardType_t; 

uint8_t get_board(void *game_connect_four, BoardType_t bt, uint8_t row, uint8_t col) {
    struct game_connect_four *game = (struct game_connect_four *) game_connect_four;
    if (row > NUM_ROWS - 1 || row < 0) { return; }
    if (col > NUM_COLUMNS -1 || col < 0) { return; }

    if (bt == GAME_BOARD) {
        return (game->board >> (8*row + (7-col))) && 1;
    } else {
        return (game->board_in_use >> (8*row + (7-col))) && 1;
    }
}

void set_board(void *game_connect_four, BoardType_t bt, uint8_t row, uint8_t col, uint8_t value) {
    struct game_connect_four *game = (struct game_connect_four *) game_connect_four;
    if (row > NUM_ROWS - 1 || row < 0) { return; }
    if (col > NUM_COLUMNS -1 || col < 0) { return; }
    if (bt == GAME_BOARD) {
        game->board |= (value << (8*row + (7-col)));
    } else {
        game->board_in_use |= (value << (8*row + (7-col)));
    }
}


uint8_t add_to_col(void *game_connect_four, uint8_t column) {
    struct game_connect_four *game = (struct game_connect_four *) game_connect_four;
    if (column > NUM_COLUMNS - 1 || column < 0) {
        return 0;
    }

    for (uint8_t row = 0; row < NUM_ROWS; row++) {
        uint8_t cell_occupied = get_board(game_connect_four, IN_USE_BOARD, row, column);
        if (!cell_occupied) {
            set_board(game_connect_four, GAME_BOARD, row, column, game->turn);
            set_board_in_use(game_connect_four, IN_USE_BOARD, row, column, 1);
            return 1;
        }
    }
    return 0;
}

uint8_t won(void *game_connect_four) {
    struct game_connect_four *game = (struct game_connect_four *) game_connect_four;

    // check horizontal wins
    for (uint8_t row = 0; row < NUM_ROWS; row++) {
        uint8_t cons = 0;
        for (uint8_t col = 0; col < NUM_COLUMNS; col++) {
            if (get_board(game_connect_four, IN_USE_BOARD, row, col) == 0 && get_board(game_connect_four, GAME_BOARD, row, col) == game->turn) {
                cons += 1;
            }
            else {
                cons = 0;
            }
            if (cons == WIN_LENGTH) { return 1; }
        }
    }

    // check vertical wins
    for (uint8_t col = 0; col < NUM_COLUMNS; col++) {
        uint8_t cons = 0;
        for (uint8_t row = 0; row < NUM_ROWS; row++) {
            if (get_board(game_connect_four, IN_USE_BOARD, row, col) == 0 && get_board(game_connect_four, GAME_BOARD, row, col) == game->turn) {
                cons += 1;
            }
            else {
                cons = 0;
            }
            if (cons == WIN_LENGTH) { return 1; }
        }
    }

    // check TL -> BR diagonal wins
    for (uint8_t row = NUM_ROWS - 1; row >= WIN_LENGTH - 1; row--) {
        for (uint8_t col = 0; col < NUM_COLUMNS - WIN_LENGTH + 1; col++) {
            uint8_t count_belonging = 0;
            for (uint8_t i = 0; i < WIN_LENGTH; i++) {
                if (get_board(game_connect_four, IN_USE_BOARD, row-i, col+i) == 1 && get_board(game_connect_four, GAME_BOARD, row-i, col+i) == game->turn) {
                    count_belonging += 1;
                }
            if (count_belonging == WIN_LENGTH) { return 1; }

            }
        }
    }

    // check TR -> BL diagonal wins
    for (uint8_t row = 0; row < NUM_ROWS - WIN_LENGTH + 1; row++) {
        for (uint8_t col = 0; col < NUM_COLUMNS - WIN_LENGTH + 1; col++) {
            uint8_t count_belonging = 0;
            for (uint8_t i = 0; i < WIN_LENGTH; i++) {
                if (get_board(game_connect_four, IN_USE_BOARD, row+i, col+i) == 1 && get_board(game_connect_four, GAME_BOARD, row+i, col+i) == game->turn) {
                    count_belonging += 1;
                }
            if (count_belonging == WIN_LENGTH) { return 1; }
            }
        }
    }

    return 0;
}

void game_connect_four_init(void *game_connect_four, uint8_t color_frame[LED_ROWS][LED_COLS][LED_CHANNELS]) {
    struct game_connect_four *game = (struct game_connect_four *) game_connect_four;

    game->turn = 0;
    game->board = 0;
    game->board_in_use = 0;
}

void game_connect_four_loop(void *game_connect_four, uint8_t color_frame[LED_ROWS][LED_COLS][LED_CHANNELS]) {
    // update 
    struct game_connect_four *game = (struct game_connect_four *) game_connect_four;
}