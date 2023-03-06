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
// which is 7 wide, followed by 1 blank (always to be 0). The next row corresponds to the next 8 bits.
struct game_connect_four {
    /* Use the game's struct to store game state info */
    uint8_t turn;
    uint64_t board;
    uint64_t board_in_use;
};

typedef enum {GAME_BOARD, IN_USE_BOARD} BoardType_t; 

/// @brief Gets the value at specified board and returns it
/// @param game_connect_four ref to connect_four instance
/// @param bt BoardType_t representing the board to look at
/// @param row specified row, requires that 0 <= row < NUM_ROWS
/// @param col specified col, requires that 0 <= col < NUM_COLUMNS
/// @return value of the specified board at position row and col, as specified above
uint8_t get_board(void *game_connect_four, BoardType_t bt, uint8_t row, uint8_t col);

/// @brief Sets the value at specified board and returns it
/// @param game_connect_four ref to connect_four instance
/// @param bt BoardType_t representing the board to look at
/// @param row specified row, requires that 0 <= row < NUM_ROWS
/// @param col specified col, requires that 0 <= col < NUM_COLUMNS
/// @param value the value to mutate the selected board at position row and col to, as specified above
void set_board(void *game_connect_four, BoardType_t bt, uint8_t row, uint8_t col, uint8_t value);

/// @brief Adds a piece to the bottommost unfilled spot in column, or returns 0 if not possible.
/// @param game_connect_four ref to connect_four instance
/// @param column column # to add it, leftmost = 0, rightmost = 6. 0 <= column <= 6
/// @returns 1 if successfully added, 0 otherwise
uint8_t add_to_col(void *game_connect_four, uint8_t column);

/// @brief Checks if the game is won
/// @param game_connect_four ref to connect_four instance
/// @returns 1 if game has been won, 0 otherwise
uint8_t won(void *game_connect_four);

void game_connect_four_init(void *game_connect_four, uint8_t color_frame[LED_ROWS][LED_COLS][LED_CHANNELS]);
void game_connect_four_loop(void *game_connect_four, uint8_t color_frame[LED_ROWS][LED_COLS][LED_CHANNELS]);

#endif

