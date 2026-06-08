#include "game_state.h"

CellState grid[ROWS][COLS];
SnakeSegment snake[COLS * ROWS];
uint16_t snake_length = 3;
uint8_t score = 0;
uint8_t highscore = 0;
bool aiModeEnabled = true;
