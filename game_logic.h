#pragma once

#include <cstdint>
#include "config.h"
#include "ili9341.h"

void placeApple();
void initializeButtons();
bool isButtonPressed(uint8_t pin);
bool anyButtonPressed();
void initializeGame();
void gameLoop(ILI9341& display, uint16_t bx, uint16_t by);
