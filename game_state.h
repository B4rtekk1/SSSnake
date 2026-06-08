#pragma once

#include "types.h"
#include "config.h"

extern CellState grid[ROWS][COLS];
extern SnakeSegment snake[COLS * ROWS];
extern uint16_t snake_length;
extern uint8_t score;
extern uint8_t highscore;
extern bool aiModeEnabled;
