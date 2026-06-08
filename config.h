#pragma once
#include <cstdint>
#include "ili9341.h"

static constexpr uint8_t COLS = 15;
static constexpr uint8_t ROWS = 15;
static constexpr uint16_t CELL_SIZE = 16;
static constexpr uint8_t APPLES_ON_BOARD = 1;

static constexpr uint8_t BTN_UP_PIN = 2;
static constexpr uint8_t BTN_LEFT_PIN = 3;
static constexpr uint8_t BTN_DOWN_PIN = 4;
static constexpr uint8_t BTN_RIGHT_PIN = 5;
static constexpr uint8_t BUZZER_PIN = 18;

static constexpr uint16_t SNAKE_COLOR = COLOR_BLUE;
static constexpr uint16_t APPLE_COLOR = COLOR_RED;
static constexpr uint16_t BACKGROUND_COLOR = COLOR_GREEN;

static constexpr uint32_t MOVE_INTERNAL = 100000; // microseconds (0.1 seconds)