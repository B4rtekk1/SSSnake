#pragma once
#include <cstdint>

enum CellState : uint8_t {
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