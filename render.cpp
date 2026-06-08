#include "render.h"
#include "config.h"
#include "types.h"
#include "game_state.h"

void drawCell(ILI9341& display, uint16_t bx, uint16_t by, uint8_t col, uint8_t row, uint16_t color) {
    uint16_t x = bx + col * CELL_SIZE;
    uint16_t y = by + row * CELL_SIZE;
    display.fillRect(x, y, CELL_SIZE, CELL_SIZE, color);
}

void clearCell(ILI9341& display, uint16_t bx, uint16_t by, uint8_t col, uint8_t row) {
    drawCell(display, bx, by, col, row, BACKGROUND_COLOR);
}

void drawGridCell(ILI9341& display, uint16_t bx, uint16_t by, uint8_t col, uint8_t row) {
    uint16_t x = bx + col * CELL_SIZE;
    uint16_t y = by + row * CELL_SIZE;
    display.fillRect(x, y, 1, CELL_SIZE, COLOR_WHITE);
    display.fillRect(x, y, CELL_SIZE, 1, COLOR_WHITE);
}

void drawGrid(ILI9341& display, uint16_t bx, uint16_t by) {
    for (uint8_t c = 0; c < COLS; ++c) {
        uint16_t x = bx + c * CELL_SIZE;
        for (uint8_t r = 0; r < ROWS; ++r) {
            display.fillRect(x, by + r * CELL_SIZE, 1, CELL_SIZE, COLOR_WHITE);
        }
    }
    for (uint8_t r = 0; r < ROWS; ++r) {
        uint16_t y = by + r * CELL_SIZE;
        for (uint8_t c = 0; c < COLS; ++c) {
            display.fillRect(bx + c * CELL_SIZE, y, CELL_SIZE, 1, COLOR_WHITE);
        }
    }

}

void drawBoard(ILI9341& display, uint16_t bx, uint16_t by) {
    display.fillRect(bx, by, COLS * CELL_SIZE, ROWS * CELL_SIZE, BACKGROUND_COLOR);
    for (uint8_t r = 0; r < ROWS; ++r) {
        for (uint8_t c = 0; c < COLS; ++c) {
            if (grid[r][c] == SNAKE) {
                drawCell(display, bx, by, c, r, SNAKE_COLOR);
            } else if (grid[r][c] == APPLE) {
                drawCell(display, bx, by, c, r, APPLE_COLOR);
            }
        }
    }
    drawGrid(display, bx, by);
}

void drawDigit(ILI9341& display, uint16_t x, uint16_t y,
               uint8_t digit, uint16_t color, uint8_t scale = 2) {
    if (digit > 9) return;
    extern const uint8_t font5x7[36][5];
    for (uint8_t col = 0; col < 5; col++) {
        uint8_t colData = font5x7[digit][col];
        for (uint8_t row = 0; row < 7; row++) {
            if (colData & (1 << row))
                display.fillRect(x + col * scale, y + row * scale, scale, scale, color);
        }
    }
}

void drawNumber(ILI9341& display, uint16_t x, uint16_t y,
                uint8_t value, uint16_t color) {
    if (value >= 100) {
        drawDigit(display, x,      y, value / 100,       color);
        drawDigit(display, x + 10, y, (value / 10) % 10, color);
        drawDigit(display, x + 20, y, value % 10,        color);
        return;
    }
    if (value >= 10) {
        drawDigit(display, x,      y, value / 10, color);
        drawDigit(display, x + 10, y, value % 10, color);
        return;
    }
    drawDigit(display, x, y, value, color);
}

void drawLoadGameMode(ILI9341& display, uint16_t bx, uint16_t by,
                      bool aiMode, bool selectedVisible) {
    const uint16_t x = bx + 20;
    const uint16_t y = by + (aiMode ? 20 : 80);
    const bool selected = (aiMode == aiModeEnabled);
    const uint16_t color = selected ? (selectedVisible ? COLOR_GREEN : COLOR_BLACK) : COLOR_WHITE;
    const char* label = aiMode ? "AI mode" : "Player mode";

    display.fillRect(x, y, 150, 18, COLOR_BLACK);
    display.drawString(x, y, label, color, 2U);
}

void updateLoadGameBlink(ILI9341& display, uint16_t bx, uint16_t by,
                         bool selectedVisible) {
    drawLoadGameMode(display, bx, by, true, selectedVisible);
    drawLoadGameMode(display, bx, by, false, selectedVisible);
}

void drawScoreValues(ILI9341& display, uint16_t bx, uint16_t scoreY, uint8_t score) {
    display.fillRect(bx + 42, scoreY, 36, 16, COLOR_BLACK);
    drawNumber(display, bx + 42, scoreY, score, COLOR_WHITE);

    display.fillRect(bx + 168, scoreY, 36, 16, COLOR_BLACK);
    drawNumber(display, bx + 168, scoreY, highscore, COLOR_WHITE);
}

void drawHUD(ILI9341& display, uint16_t bx, uint16_t by, uint16_t boardHeight, uint8_t score) {
    uint16_t titleY = (by >= 16) ? (by -16) : 0;
    uint16_t scoreY = by + boardHeight + 8;

    display.fillRect(bx, titleY, 120, 12, COLOR_BLACK);
    display.fillRect(0, scoreY, LCD_WIDTH, 16, COLOR_BLACK);

    display.drawString(bx + 2, titleY, "SNAKE", COLOR_WHITE);
    display.drawString(bx + 2, scoreY, "SCORE", COLOR_WHITE);
    display.drawString(bx + 104, scoreY, "HIGHSCORE", COLOR_WHITE);

    drawScoreValues(display, bx, scoreY, score);
}

void win() {
}

void lose(ILI9341& display, uint16_t bx, uint16_t by, uint16_t boardHeight) {
    display.fillRect(bx, by, COLS * CELL_SIZE, ROWS * CELL_SIZE, COLOR_BLACK);
    display.drawString(bx + 20, by + boardHeight / 2 - 28, "GAME OVER", COLOR_RED, 4U);
    if (aiModeEnabled) {
        display.drawString(bx + 20, by + boardHeight / 2 + 8, "AUTO RESTART", COLOR_WHITE, 2U);
    } else {
        display.drawString(bx + 20, by + boardHeight / 2 + 8, "PRESS ANY BUTTON", COLOR_WHITE, 2U);
    }
}
