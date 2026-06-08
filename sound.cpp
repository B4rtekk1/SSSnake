#include <cstdint>

namespace
{
    struct NoteEvent {
        uint16_t freq;
        uint16_t duration_ms;
    };

    constexpr uint16_t C4 = 261;
    constexpr uint16_t D4 = 294;
    constexpr uint16_t E4 = 329;
    constexpr uint16_t F4 = 349;
    constexpr uint16_t G4 = 392;
    constexpr uint16_t A4 = 440;
    constexpr uint16_t B4 = 493;
    constexpr uint16_t C5 = 523;
    constexpr uint16_t D5 = 587;
    constexpr uint16_t E5 = 659;
    constexpr uint16_t F5 = 698;
    constexpr uint16_t G5 = 784;
    constexpr uint16_t A5 = 880;
    constexpr uint16_t B5 = 987;

    constexpr uint16_t pause = 0;

    constexpr NoteEvent eatAppleTune[] = {
        {C5, 100},
        {pause, 30},
        {E5, 100},
        {pause, 30},
        {G5, 150}
    };
} // namespace

#include "sound.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "config.h"

namespace {
    static bool melodyPlaying = false;
    static uint8_t melodyIndex = 0;
    static uint32_t melodyStartTime = 0;

    void startMelody(uint16_t freq) {
        uint slice = pwm_gpio_to_slice_num(BUZZER_PIN);

        if (freq == pause) {
            pwm_set_gpio_level(BUZZER_PIN, 0);
            return;
        }

        uint32_t clock = clock_get_hz(clk_sys);
        uint32_t wrap = 1000;

        pwm_set_wrap(slice, wrap);
        pwm_set_clkdiv(slice, (float)clock / (freq * wrap));

        pwm_set_gpio_level(BUZZER_PIN, wrap / 2);
        pwm_set_enabled(slice, true);
    }

    void stopMelody() {
        pwm_set_gpio_level(BUZZER_PIN, 0);
    }
}

void playMelody() {
    melodyPlaying = true;
    melodyIndex = 0;
    melodyStartTime = to_ms_since_boot(get_absolute_time());
}

void updateSound() {
    if (!melodyPlaying) return;

    uint32_t now = to_ms_since_boot(get_absolute_time());

    static bool noteStarted = false;
    if (!noteStarted) {
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

void resetSound() {
    melodyPlaying = false;
    melodyIndex = 0;
    melodyStartTime = 0;
    pwm_set_gpio_level(BUZZER_PIN, 0);
}
