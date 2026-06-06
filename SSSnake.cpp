#include <cstdio>
#include <cstdlib>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/pwm.h"
#include "ili9341.h"

struct SnakeSegment;
enum CellState : uint8_t;

extern "C" bool asm_is_opposite(uint8_t dir1, uint8_t dir2);
extern "C" bool asm_snake_collision(const SnakeSegment* snake, uint16_t limit, uint8_t x, uint8_t y);
extern "C" void asm_shift_snake(SnakeSegment* snake, uint16_t length);
extern "C" void asm_clear_grid(CellState* grid, uint16_t size);
extern "C" bool asm_place_apple(CellState* grid, uint16_t cells, uint16_t target);

#ifndef FLASH_SECTOR_SIZE
#define FLASH_SECTOR_SIZE 4096
#endif

#define HIGHSCORE_FLASH_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

static bool gameOn = true;

static const uint8_t COLS = 15;
static const uint8_t ROWS = 15;
static const uint16_t CELL_SIZE = 16;
static const uint8_t APPLES_ON_BOARD = 1;

#define BTN_UP_PIN 2
#define BTN_LEFT_PIN 3
#define BTN_DOWN_PIN 4
#define BTN_RIGHT_PIN 5

#define BUZZER_PIN 18

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

class Notes {
    public:
    static const uint16_t C4 = 261;
    static const uint16_t D4 = 294;
    static const uint16_t E4 = 329;
    static const uint16_t F4 = 349;
    static const uint16_t G4 = 392;
    static const uint16_t A4 = 440;
    static const uint16_t B4 = 493;

    static const uint16_t C5 = 523;
    static const uint16_t D5 = 587;
    static const uint16_t E5 = 659;
    static const uint16_t F5 = 698;
    static const uint16_t G5 = 784;
    static const uint16_t A5 = 880;
    static const uint16_t B5 = 987;

    static const uint16_t pause = 0;
};

struct NoteEvent {
    uint16_t freq;
    uint16_t duration_ms;
};

static const NoteEvent eatAppleTune[] = {
    {Notes::C5, 100},
    {Notes::pause, 30},
    {Notes::E5, 100},
    {Notes::pause, 30},
    {Notes::G5, 150}
};

static bool melodyPlaying = false;
static uint8_t melodyIndex = 0;
static uint32_t melodyStartTime = 0;

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

static CellState grid[ROWS][COLS];
static SnakeSegment snake[COLS * ROWS];
static uint16_t snake_length = 3;
static Direction current_direction = RIGHT;
static Direction next_direction = RIGHT;
static uint32_t last_move_time = 0;
static uint8_t score = 0;
static const uint32_t MOVE_INTERVAL = 100000; // microseconds (0.1 seconds)

static bool isOpposite(Direction dir1, Direction dir2) {
    return asm_is_opposite(static_cast<uint8_t>(dir1), static_cast<uint8_t>(dir2));
}
static void win();

static bool isButtonPressed(uint8_t pin) {
    return (gpio_get(pin) == (BUTTONS_ACTIVE_LOW ? 0 : 1));
}

static void initializeButtons() {
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

static void requestDirection(Direction dir) {
    if (dir == next_direction) return;
    if (isOpposite(current_direction, dir)) return;
    if (isOpposite(next_direction, dir)) return;
    next_direction = dir;
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

static void placeApple(){
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
    bool leftSnake = (col > 0) && (grid[row][col-1] == SNAKE);
    bool rightSnake = (col < COLS-1) && (grid[row][col+1] == SNAKE);
    if (!(leftSnake && rightSnake)) {
        display.fillRect(x, y, 1, CELL_SIZE, COLOR_WHITE);
    }

    bool topSnake = (row > 0) && (grid[row-1][col] == SNAKE);
    bool bottomSnake = (row < ROWS-1) && (grid[row+1][col] == SNAKE);
    if (!(topSnake && bottomSnake)) {
        display.fillRect(x, y, CELL_SIZE, 1, COLOR_WHITE);
    }
}

void drawGrid(ILI9341& display, uint16_t bx, uint16_t by) {
    for (uint8_t c = 0; c < COLS; ++c) {
        uint16_t x = bx + c * CELL_SIZE;
        for (uint8_t r = 0; r < ROWS; ++r) {
            bool leftSnake = (c > 0) && (grid[r][c-1] == SNAKE);
            bool rightSnake = (c < COLS-1) && (grid[r][c+1] == SNAKE);
            if (leftSnake && rightSnake) continue;
            display.fillRect(x, by + r * CELL_SIZE, 1, CELL_SIZE, COLOR_WHITE);
        }
    }
    for (uint8_t r = 0; r < ROWS; ++r) {
        uint16_t y = by + r * CELL_SIZE;
        for (uint8_t c = 0; c < COLS; ++c) {
            bool topSnake = (r > 0) && (grid[r-1][c] == SNAKE);
            bool bottomSnake = (r < ROWS-1) && (grid[r+1][c] == SNAKE);
            if (topSnake && bottomSnake) continue;
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

static uint8_t drawNumber(ILI9341& display, uint16_t x, uint16_t y,
                           uint8_t value, uint16_t color) {
    if (value >= 100) {
        drawDigit(display, x,      y, value / 100,       color);
        drawDigit(display, x + 10, y, (value / 10) % 10, color);
        drawDigit(display, x + 20, y, value % 10,        color);
        return 3;
    }
    if (value >= 10) {
        drawDigit(display, x,      y, value / 10, color);
        drawDigit(display, x + 10, y, value % 10, color);
        return 2;
    }
    drawDigit(display, x, y, value, color);
    return 1;
}

void drawHUD(ILI9341& display, uint16_t bx, uint16_t by, uint16_t boardHeight, uint8_t score) {
    uint16_t titleY = (by >= 16) ? (by -16) : 0;
    uint16_t scoreY = by + boardHeight + 8;

    display.fillRect(bx, titleY, 120, 12, COLOR_BLACK);
    display.fillRect(0, scoreY, LCD_WIDTH, 16, COLOR_BLACK);
    display.drawString(bx + 2, titleY, "SNAKE", COLOR_WHITE);
    display.drawString(bx + 2, scoreY, "SCORE", COLOR_WHITE);
    drawNumber(display, bx + 42, scoreY, score, COLOR_WHITE);
    display.drawString(bx + 104, scoreY, "HIGHSCORE", COLOR_WHITE);
    drawNumber(display, bx + 168, scoreY, highscore, COLOR_WHITE);
}

void win() {
}

void lose(ILI9341& display, uint16_t bx, uint16_t by, uint16_t boardHeight) {
    display.fillRect(bx, by, COLS * CELL_SIZE, ROWS * CELL_SIZE, COLOR_BLACK);
    display.drawString(bx + 20, by + boardHeight / 2 - 28, "GAME OVER", COLOR_RED, 4U);
    display.drawString(bx + 20, by + boardHeight / 2 + 8, "PRESS ANY BUTTON", COLOR_WHITE, 2U);
}

void loadGame(ILI9341& display, uint16_t bx, uint16_t by) {
    display.fillScreen(COLOR_BLACK);
    display.drawString(bx + 20, by + 20, "Press any button\nto start", COLOR_WHITE, 2U);
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

void playMelody() {
    melodyPlaying = true;
    melodyIndex = 0;
    melodyStartTime = to_ms_since_boot(get_absolute_time());
}

void startMelody(uint16_t freq) {
    uint slice = pwm_gpio_to_slice_num(BUZZER_PIN);

    if (freq == Notes::pause) {
        pwm_set_gpio_level(BUZZER_PIN, 0);
        return;
    }

    uint32_t clock = clock_get_hz(clk_sys);
    uint32_t wrap = 1000;

    pwm_set_wrap(slice, wrap);
    pwm_set_clkdiv(slice, (float)clock / (freq * wrap));

    pwm_set_gpio_level(BUZZER_PIN, wrap / 2);
}

void stopMelody() {
    pwm_set_gpio_level(BUZZER_PIN, 0);
}

void updateSound() {
    if (!melodyPlaying) return;

    uint32_t now = to_ms_since_boot(get_absolute_time());

    static bool noteStarted = false;
    if(!noteStarted) {
        startMelody(eatAppleTune[melodyIndex].freq);
        noteStarted = true;
    }

    if (now - melodyStartTime >= eatAppleTune[melodyIndex].duration_ms) {
        melodyIndex++;
        noteStarted = false;
        melodyStartTime = now;

        if (melodyIndex >= sizeof(eatAppleTune) / sizeof(NoteEvent)) {
            melodyPlaying = false;
            stopMelody();
        }
    }
}

void generateSound(uint32_t freq = Notes::C5, uint16_t duration_ms = 100) {
    uint slice = pwm_gpio_to_slice_num(BUZZER_PIN);

    if (freq == Notes::pause) {
        pwm_set_gpio_level(BUZZER_PIN, 0);
    } else {
        uint16_t level = 500000 / freq; // Calculate duty cycle for desired frequency
        pwm_set_gpio_level(BUZZER_PIN, level);
    }
    sleep_ms(duration_ms);
    uint32_t clock = clock_get_hz(clk_sys);
    uint32_t wrap = 1000;

    pwm_set_wrap(slice, wrap);
    pwm_set_clkdiv(slice, (float)clock / (freq * wrap));

    pwm_set_gpio_level(BUZZER_PIN, wrap / 5); // 50% duty cycle for square wave
    pwm_set_enabled(slice, true);
    sleep_ms(duration_ms);
    pwm_set_gpio_level(BUZZER_PIN, 0); // Turn off sound
}

void resetSound() {
    melodyPlaying = false;
    melodyIndex = 0;
    melodyStartTime = 0;
    pwm_set_gpio_level(BUZZER_PIN, 0);
}

void gameLoop(ILI9341& display, uint16_t bx, uint16_t by) {
    const uint16_t boardHeight = ROWS*CELL_SIZE;
    bool firstFrame = true;
    uint8_t lastScore = 255;    
    while (gameOn) {
        updateSound();
        checkButtonInput();
        uint32_t now = time_us_32();
        if (now - last_move_time < MOVE_INTERVAL) continue;
        last_move_time = now;

        if(firstFrame) {
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
            case DOWN:  if (newHead.y == ROWS-1) { hit_wall = true; } else { newHead.y++; } break;
            case RIGHT: if (newHead.x == COLS-1) { hit_wall = true; } else { newHead.x++; } break;
        }

        bool grow = false;
        if (!hit_wall) grow = (grid[newHead.y][newHead.x] == APPLE);

        uint16_t checkLimit = grow ? snake_length : (snake_length - 1);
        bool collision = asm_snake_collision(snake, checkLimit, newHead.x, newHead.y);

        if (collision || hit_wall) {
            resetSound();
            if (score > highscore) {
                highscore = score;
                save_uint8_to_flash(highscore);
            }
            // Always show lose screen and wait for player's input to restart
            lose(display, bx, by, boardHeight);
            while (true) {
                // Poll raw button state without updating game direction
                if (isButtonPressed(BTN_UP_PIN) || isButtonPressed(BTN_LEFT_PIN) ||
                    isButtonPressed(BTN_DOWN_PIN) || isButtonPressed(BTN_RIGHT_PIN)) {
                    // wait for all buttons to be released to avoid immediate direction on restart
                    while (isButtonPressed(BTN_UP_PIN) || isButtonPressed(BTN_LEFT_PIN) ||
                           isButtonPressed(BTN_DOWN_PIN) || isButtonPressed(BTN_RIGHT_PIN)) {
                        sleep_ms(50);
                    }
                    sleep_ms(50); // additional debounce
                    break;
                }
                sleep_ms(50);
            }
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

        if(grow) {
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
            drawHUD(display, bx, by, boardHeight, score);
            lastScore = score;
        }
    }
}

int main()
{
    stdio_init_all();
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_wrap(slice_num, 1000);
    pwm_set_enabled(slice_num, true);
    sleep_ms(1000); // Wait for USB serial to initialize
    set_sys_clock_khz(250000, true); // Overclock to 250MHz for better performance
    srand(time_us_32()); // Seed random with current time
    highscore = read_uint8_from_flash();

    ILI9341 display;
    display.init();
    initializeButtons();

    const uint16_t board_w = COLS * CELL_SIZE;
    const uint16_t board_h = ROWS * CELL_SIZE;

    const uint16_t screen_w = LCD_WIDTH;
    const uint16_t screen_h = LCD_HEIGHT;
    const uint16_t bx = (screen_w > board_w) ? (screen_w - board_w) / 2 : 0;
    const uint16_t by = (screen_h > board_h) ? (screen_h - board_h) / 2 : 0;

    display.fillScreen(COLOR_BLACK);
    loadGame(display, bx, by);
    while (true) {
                // Poll raw button state without updating game direction
                if (isButtonPressed(BTN_UP_PIN) || isButtonPressed(BTN_LEFT_PIN) ||
                    isButtonPressed(BTN_DOWN_PIN) || isButtonPressed(BTN_RIGHT_PIN)) {
                    // wait for all buttons to be released to avoid immediate direction on restart
                    while (isButtonPressed(BTN_UP_PIN) || isButtonPressed(BTN_LEFT_PIN) ||
                           isButtonPressed(BTN_DOWN_PIN) || isButtonPressed(BTN_RIGHT_PIN)) {
                        sleep_ms(50);
                    }
                    sleep_ms(50); // additional debounce
                    break;
                }
                sleep_ms(50);
            }
    initializeGame();
    gameLoop(display, bx, by);

}
