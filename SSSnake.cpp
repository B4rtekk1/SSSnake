#include <cstdlib>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "hardware/vreg.h"

#include "config.h"
#include "dqn_inference.h"
#include "game_logic.h"
#include "game_state.h"
#include "ili9341.h"
#include "render.h"
#include "storage.h"

namespace {
    constexpr bool DQN_AGENT_ENABLED = true;

    void loadGame(ILI9341& display, uint16_t bx, uint16_t by) {
        display.fillScreen(COLOR_BLACK);

        aiModeEnabled = DQN_AGENT_ENABLED;
        bool selectedVisible = true;
        uint32_t lastBlinkMs = to_ms_since_boot(get_absolute_time());

        display.drawString(bx + 20, by + 140,
                           "UP: AI mode\nDOWN: Player mode\nLEFT/RIGHT: start",
                           COLOR_WHITE, 2U);
        updateLoadGameBlink(display, bx, by, selectedVisible);

        while (true) {
            if (isButtonPressed(BTN_UP_PIN)) {
                aiModeEnabled = true;
                selectedVisible = true;
                updateLoadGameBlink(display, bx, by, selectedVisible);
                while (anyButtonPressed()) sleep_ms(30);
            } else if (isButtonPressed(BTN_DOWN_PIN)) {
                aiModeEnabled = false;
                selectedVisible = true;
                updateLoadGameBlink(display, bx, by, selectedVisible);
                while (anyButtonPressed()) sleep_ms(30);
            } else if (isButtonPressed(BTN_LEFT_PIN) || isButtonPressed(BTN_RIGHT_PIN)) {
                while (anyButtonPressed()) sleep_ms(30);
                break;
            }

            const uint32_t nowMs = to_ms_since_boot(get_absolute_time());
            if (nowMs - lastBlinkMs >= 300) {
                selectedVisible = !selectedVisible;
                updateLoadGameBlink(display, bx, by, selectedVisible);
                lastBlinkMs = nowMs;
            }
            sleep_ms(30);
        }

        display.fillRect(bx + 20, by + 140, 220, 60, COLOR_BLACK);
        if (aiModeEnabled) {
            if (dqnWeightsReady()) {
                display.drawString(bx + 20, by + 140, "DQN agent\nstarting", COLOR_WHITE, 2U);
            } else {
                display.drawString(bx + 20, by + 140, "DQN fallback\nstarting", COLOR_WHITE, 2U);
            }
            sleep_ms(700);
        } else {
            display.drawString(bx + 20, by + 140, "Player mode\nstarting", COLOR_WHITE, 2U);
            sleep_ms(400);
        }
    }
}

int main() {
    stdio_init_all();
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_wrap(slice_num, 1000);
    pwm_set_enabled(slice_num, true);
    sleep_ms(1000); // Wait for USB serial to initialize
    set_sys_clock_khz(250000, true); // Overclock to 250MHz for better performance
    srand(time_us_32()); // Seed random with current time
    highscore = readHighscore();

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
    initializeGame();
    gameLoop(display, bx, by);
}
