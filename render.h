#pragma once

#include <cstdint>
#include "ili9341.h"

void drawCell(ILI9341& display, uint16_t bx, uint16_t by, uint8_t col, uint8_t row, uint16_t color);
void clearCell(ILI9341& display, uint16_t bx, uint16_t by, uint8_t col, uint8_t row);
void drawGridCell(ILI9341& display, uint16_t bx, uint16_t by, uint8_t col, uint8_t row);
void drawGrid(ILI9341& display, uint16_t bx, uint16_t by);
void drawBoard(ILI9341& display, uint16_t bx, uint16_t by);
void drawDigit(ILI9341& display, uint16_t x, uint16_t y,
               uint8_t digit, uint16_t color, uint8_t scale);
void drawLoadGameMode(ILI9341& display, uint16_t bx, uint16_t by, bool aiMode, bool selectedVisible);
void updateLoadGameBlink(ILI9341& display, uint16_t bx, uint16_t by, bool selectedVisible);
void drawNumber(ILI9341& display, uint16_t x, uint16_t y, uint8_t number, uint16_t color);
void drawScoreValues(ILI9341& display, uint16_t bx, uint16_t scoreY, uint8_t score);
void drawHUD(ILI9341& display, uint16_t bx, uint16_t by, uint16_t boardHeight, uint8_t score);
void win();
void lose(ILI9341& display, uint16_t bx, uint16_t by, uint16_t boardHeight);
