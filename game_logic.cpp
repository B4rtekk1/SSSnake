#include "game_logic.h"

#include <cstdlib>

#include "dqn_inference.h"
#include "game_state.h"
#include "render.h"
#include "sound.h"
#include "storage.h"
#include "pico/stdlib.h"

extern "C" bool asm_is_opposite(uint8_t dir1, uint8_t dir2);
extern "C" bool asm_snake_collision(const SnakeSegment* snake, uint16_t limit, uint8_t x, uint8_t y);
extern "C" void asm_shift_snake(SnakeSegment* snake, uint16_t length);
extern "C" void asm_clear_grid(CellState* grid, uint16_t size);
extern "C" bool asm_place_apple(CellState* grid, uint16_t cells, uint16_t target);

namespace {
    constexpr bool BUTTONS_ACTIVE_LOW = true;
    constexpr uint32_t MOVE_INTERVAL = 100000; // microseconds (0.1 seconds)

    Direction current_direction = RIGHT;
    Direction next_direction = RIGHT;
    uint32_t last_move_time = 0;

    bool isOpposite(Direction dir1, Direction dir2) {
        return asm_is_opposite(static_cast<uint8_t>(dir1), static_cast<uint8_t>(dir2));
    }

    void requestDirection(Direction dir) {
        if (dir == next_direction) return;
        if (isOpposite(current_direction, dir)) return;
        if (isOpposite(next_direction, dir)) return;
        next_direction = dir;
    }

    Direction turnRight(Direction dir) {
        switch (dir) {
            case UP: return RIGHT;
            case RIGHT: return DOWN;
            case DOWN: return LEFT;
            case LEFT: return UP;
        }
        return RIGHT;
    }

    Direction turnLeft(Direction dir) {
        switch (dir) {
            case UP: return LEFT;
            case LEFT: return DOWN;
            case DOWN: return RIGHT;
            case RIGHT: return UP;
        }
        return RIGHT;
    }

    Direction actionToDirection(uint8_t action, Direction base) {
        if (action == 1) {
            return turnRight(base);
        }
        if (action == 2) {
            return turnLeft(base);
        }
        return base;
    }

    bool movedHead(Direction dir, SnakeSegment& nextHead) {
        nextHead = snake[0];
        switch (dir) {
            case UP:
                if (nextHead.y == 0) return false;
                nextHead.y--;
                break;
            case LEFT:
                if (nextHead.x == 0) return false;
                nextHead.x--;
                break;
            case DOWN:
                if (nextHead.y == ROWS - 1) return false;
                nextHead.y++;
                break;
            case RIGHT:
                if (nextHead.x == COLS - 1) return false;
                nextHead.x++;
                break;
        }
        return true;
    }

    bool wouldCollide(Direction dir) {
        SnakeSegment nextHead;
        if (!movedHead(dir, nextHead)) {
            return true;
        }

        if (grid[nextHead.y][nextHead.x] != SNAKE) {
            return false;
        }

        const SnakeSegment tail = snake[snake_length - 1];
        return !(nextHead.x == tail.x && nextHead.y == tail.y);
    }

    bool findApple(uint8_t& appleX, uint8_t& appleY) {
        for (uint8_t y = 0; y < ROWS; ++y) {
            for (uint8_t x = 0; x < COLS; ++x) {
                if (grid[y][x] == APPLE) {
                    appleX = x;
                    appleY = y;
                    return true;
                }
            }
        }
        return false;
    }

    uint16_t manhattanDistance(SnakeSegment point, uint8_t x, uint8_t y) {
        uint16_t dx = (point.x > x) ? (point.x - x) : (x - point.x);
        uint16_t dy = (point.y > y) ? (point.y - y) : (y - point.y);
        return dx + dy;
    }

    void buildDqnState(float state[11]) {
        const Direction straight = current_direction;
        const Direction right = turnRight(current_direction);
        const Direction left = turnLeft(current_direction);

        uint8_t appleX = snake[0].x;
        uint8_t appleY = snake[0].y;
        findApple(appleX, appleY);

        state[0] = wouldCollide(straight) ? 1.0f : 0.0f;
        state[1] = wouldCollide(right) ? 1.0f : 0.0f;
        state[2] = wouldCollide(left) ? 1.0f : 0.0f;
        state[3] = (current_direction == LEFT) ? 1.0f : 0.0f;
        state[4] = (current_direction == RIGHT) ? 1.0f : 0.0f;
        state[5] = (current_direction == UP) ? 1.0f : 0.0f;
        state[6] = (current_direction == DOWN) ? 1.0f : 0.0f;
        state[7] = (appleX < snake[0].x) ? 1.0f : 0.0f;
        state[8] = (appleX > snake[0].x) ? 1.0f : 0.0f;
        state[9] = (appleY < snake[0].y) ? 1.0f : 0.0f;
        state[10] = (appleY > snake[0].y) ? 1.0f : 0.0f;
    }

    uint8_t chooseSafeGreedyAction() {
        uint8_t appleX = snake[0].x;
        uint8_t appleY = snake[0].y;
        findApple(appleX, appleY);

        const uint16_t currentDistance = manhattanDistance(snake[0], appleX, appleY);
        int16_t bestScore = -30000;
        uint8_t bestAction = 0;

        for (uint8_t action = 0; action < 3; ++action) {
            const Direction dir = actionToDirection(action, current_direction);
            SnakeSegment nextHead;
            int16_t actionScore = 0;

            if (!movedHead(dir, nextHead) || wouldCollide(dir)) {
                actionScore -= 10000;
            } else {
                const uint16_t nextDistance = manhattanDistance(nextHead, appleX, appleY);
                actionScore += 1000;
                actionScore += static_cast<int16_t>(currentDistance - nextDistance) * 20;
                if (action == 0) {
                    actionScore += 2;
                }
            }

            if (actionScore > bestScore) {
                bestScore = actionScore;
                bestAction = action;
            }
        }

        return bestAction;
    }

    void updateDqnInput() {
        float state[11];
        buildDqnState(state);

        uint8_t action = dqnWeightsReady() ? dqnPredictAction(state) : chooseSafeGreedyAction();
        Direction desiredDirection = actionToDirection(action, current_direction);

        if (wouldCollide(desiredDirection)) {
            action = chooseSafeGreedyAction();
            desiredDirection = actionToDirection(action, current_direction);
        }

        if (!isOpposite(current_direction, desiredDirection)) {
            next_direction = desiredDirection;
        }
    }

    void checkButtonInput() {
        if (isButtonPressed(BTN_UP_PIN)) {
            requestDirection(UP);
        } else if (isButtonPressed(BTN_LEFT_PIN)) {
            requestDirection(LEFT);
        } else if (isButtonPressed(BTN_DOWN_PIN)) {
            requestDirection(DOWN);
        } else if (isButtonPressed(BTN_RIGHT_PIN)) {
            requestDirection(RIGHT);
        }
    }

    void waitForManualButtonPress() {
        while (true) {
            if (anyButtonPressed()) {
                while (anyButtonPressed()) {
                    sleep_ms(50);
                }
                sleep_ms(50);
                break;
            }
            sleep_ms(50);
        }
    }

    void waitForRestartOrAuto() {
        if (aiModeEnabled) {
            sleep_ms(750);
        } else {
            waitForManualButtonPress();
        }
    }
}

bool isButtonPressed(uint8_t pin) {
    return (gpio_get(pin) == (BUTTONS_ACTIVE_LOW ? 0 : 1));
}

bool anyButtonPressed() {
    return isButtonPressed(BTN_UP_PIN) || isButtonPressed(BTN_LEFT_PIN) ||
           isButtonPressed(BTN_DOWN_PIN) || isButtonPressed(BTN_RIGHT_PIN);
}

void initializeButtons() {
    const uint8_t button_pins[] = {BTN_UP_PIN, BTN_LEFT_PIN, BTN_DOWN_PIN, BTN_RIGHT_PIN};
    for (uint8_t pin : button_pins) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        if (BUTTONS_ACTIVE_LOW) {
            gpio_pull_up(pin);
        } else {
            gpio_pull_down(pin);
        }
    }
}

void placeApple() {
    uint16_t emptyCount = 0;
    for (uint8_t y = 0; y < ROWS; ++y) {
        for (uint8_t x = 0; x < COLS; ++x) {
            if (grid[y][x] == EMPTY) {
                emptyCount++;
            }
        }
    }
    if (emptyCount == 0) {
        win();
        return;
    }
    uint16_t target = rand() % emptyCount;
    asm_place_apple(&grid[0][0], ROWS * COLS, target);
}

void initializeGame() {
    requestDirection(RIGHT);
    asm_clear_grid(&grid[0][0], ROWS * COLS);
    snake[0] = {2, 0};
    snake[1] = {1, 0};
    snake[2] = {0, 0};
    snake_length = 3;
    for (uint16_t i = 0; i < snake_length; ++i) {
        grid[snake[i].y][snake[i].x] = SNAKE;
    }
    current_direction = RIGHT;
    next_direction = RIGHT;
    score = 0;
    placeApple();
}

void gameLoop(ILI9341& display, uint16_t bx, uint16_t by) {
    const uint16_t boardHeight = ROWS * CELL_SIZE;
    bool firstFrame = true;
    uint8_t lastScore = 255;

    while (true) {
        updateSound();
        if (aiModeEnabled) {
            updateDqnInput();
        } else {
            checkButtonInput();
        }

        uint32_t now = time_us_32();
        if (now - last_move_time < MOVE_INTERVAL) continue;
        last_move_time = now;

        if (firstFrame) {
            drawBoard(display, bx, by);
            drawHUD(display, bx, by, boardHeight, score);
            lastScore = score;
            firstFrame = false;
            continue;
        }

        current_direction = next_direction;
        SnakeSegment newHead = snake[0];
        bool hit_wall = false;
        switch (current_direction) {
            case UP:    if (newHead.y == 0) { hit_wall = true; } else { newHead.y--; } break;
            case LEFT:  if (newHead.x == 0) { hit_wall = true; } else { newHead.x--; } break;
            case DOWN:  if (newHead.y == ROWS - 1) { hit_wall = true; } else { newHead.y++; } break;
            case RIGHT: if (newHead.x == COLS - 1) { hit_wall = true; } else { newHead.x++; } break;
        }

        bool grow = false;
        if (!hit_wall) grow = (grid[newHead.y][newHead.x] == APPLE);

        uint16_t checkLimit = grow ? snake_length : (snake_length - 1);
        bool collision = asm_snake_collision(snake, checkLimit, newHead.x, newHead.y);

        if (collision || hit_wall) {
            resetSound();
            if (score > highscore) {
                highscore = score;
                saveHighscore(highscore);
            }
            lose(display, bx, by, boardHeight);
            waitForRestartOrAuto();
            initializeGame();
            firstFrame = true;
            continue;
        }

        if (!grow) {
            uint16_t tx = snake[snake_length - 1].x;
            uint16_t ty = snake[snake_length - 1].y;
            grid[ty][tx] = EMPTY;
            clearCell(display, bx, by, tx, ty);
            drawGridCell(display, bx, by, tx, ty);
        } else if (snake_length < (uint16_t)(ROWS * COLS)) {
            snake_length++;
            score++;
        }

        asm_shift_snake(snake, snake_length);
        snake[0] = newHead;

        grid[newHead.y][newHead.x] = SNAKE;
        drawCell(display, bx, by, newHead.x, newHead.y, SNAKE_COLOR);
        drawGridCell(display, bx, by, newHead.x, newHead.y);

        if (grow) {
            playMelody();
            placeApple();
            for (uint8_t y = 0; y < ROWS; ++y) {
                for (uint8_t x = 0; x < COLS; ++x) {
                    if (grid[y][x] == APPLE) {
                        drawCell(display, bx, by, x, y, APPLE_COLOR);
                        drawGridCell(display, bx, by, x, y);
                        goto apple_drawn;
                    }
                }
            }
            apple_drawn:;
        }

        if (score != lastScore) {
            uint16_t scoreY = by + boardHeight + 8;
            drawScoreValues(display, bx, scoreY, score);
            lastScore = score;
        }
    }
}
