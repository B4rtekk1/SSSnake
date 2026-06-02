#include <cstdio>
#include <cstdlib>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "ili9341.h"

#ifndef FLASH_SECTOR_SIZE
#define FLASH_SECTOR_SIZE 4096
#endif

#define HIGHSCORE_FLASH_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

static const uint8_t COLS = 15;
static const uint8_t ROWS = 15;
static const uint16_t CELL_SIZE = 16;
static const uint8_t APPLES_ON_BOARD = 1;

#define BTN_UP_PIN 2
#define BTN_LEFT_PIN 3
#define BTN_DOWN_PIN 4
#define BTN_RIGHT_PIN 5

static const bool BUTTONS_ACTIVE_LOW = true;

static const uint16_t SNAKE_COLOR = COLOR_BLUE;
static const uint16_t APPLE_COLOR = COLOR_RED;
static const uint16_t BACKGROUND_COLOR = COLOR_GREEN;

static uint8_t highscore = 0; //uint8 can handle up to 255, 222 is the max score for a 15x15 grid with starting length of 3

uint8_t read_uint8_from_flash()
{
    const uint8_t* flash_ptr = (const uint8_t*)(XIP_BASE + HIGHSCORE_FLASH_OFFSET);
    uint8_t value = *flash_ptr;
    return (value == 0xFF) ? 0 : value; // If flash is erased, return 0
}

void save_uint8_to_flash(uint8_t value) {
    uint32_t interrupts = save_and_disable_interrupts();
    flash_range_erase(HIGHSCORE_FLASH_OFFSET, FLASH_SECTOR_SIZE);
    uint8_t page[FLASH_PAGE_SIZE];
    for (uint16_t i = 0; i < FLASH_PAGE_SIZE; i++) {
        page[i] = (i == 0) ? value : 0xFF; // Write value to first byte, rest are 0xFF
    }
    flash_range_program(HIGHSCORE_FLASH_OFFSET, page, FLASH_PAGE_SIZE);
    restore_interrupts(interrupts);
}

enum CellState: uint8_t {
    EMPTY = 0,
    SNAKE = 1,
    APPLE = 2
};

enum Direction : uint8_t {
    UP = 0,
    LEFT = 1,
    DOWN = 2,
    RIGHT = 3
};

struct SnakeSegment {
    uint8_t x;
    uint8_t y;
};

static CellState grid[COLS][ROWS];
static SnakeSegment snake[COLS * ROWS];
static uint16_t snake_length = 3;
static Direction current_direction = RIGHT;
static Direction next_direction = RIGHT;
static uint32_t last_move_time = 0;
static uint8_t score = 0;

static bool isOpposite(Direction dir1, Direction dir2) {
    return (dir1 == UP && dir2 == DOWN) || (dir1 == DOWN && dir2 == UP) ||
           (dir1 == LEFT && dir2 == RIGHT) || (dir1 == RIGHT && dir2 == LEFT);
}


int main()
{
    stdio_init_all();

    // Initialize display
    ILI9341 lcd;
    lcd.init();
    lcd.setRotation(1); // landscape

    // Clear screen to background
    lcd.fillScreen(BACKGROUND_COLOR);

    // Board dimensions in pixels
    const uint16_t board_w = COLS * CELL_SIZE;
    const uint16_t board_h = ROWS * CELL_SIZE;

    // Center board on the display
    const uint16_t start_x = (LCD_WIDTH > board_w) ? (LCD_WIDTH - board_w) / 2 : 0;
    const uint16_t start_y = (LCD_HEIGHT > board_h) ? (LCD_HEIGHT - board_h) / 2 : 0;

    // Draw board background (white) and grid lines (black)
    const uint16_t board_bg = COLOR_WHITE;
    const uint16_t grid_col = COLOR_BLACK;

    lcd.fillRect(start_x, start_y, board_w, board_h, board_bg);

    // Vertical grid lines
    for (uint8_t c = 0; c <= COLS; ++c) {
        uint16_t x = start_x + c * CELL_SIZE;
        lcd.fillRect(x, start_y, 1, board_h, grid_col);
    }

    // Horizontal grid lines
    for (uint8_t r = 0; r <= ROWS; ++r) {
        uint16_t y = start_y + r * CELL_SIZE;
        lcd.fillRect(start_x, y, board_w, 1, grid_col);
    }

    // Keep the board visible
    while (true) {
        tight_loop_contents();
    }
}
