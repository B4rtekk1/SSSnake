#include "storage.h"
#include <cstdint>

#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico.h"

#ifndef FLASH_SECTOR_SIZE
#define FLASH_SECTOR_SIZE 4096
#endif

#define HIGHSCORE_FLASH_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

uint8_t readHighscore() {
    const uint8_t* flash_ptr = (const uint8_t*)(XIP_BASE + HIGHSCORE_FLASH_OFFSET);
    uint8_t value = *flash_ptr;
    return (value == 0xFF) ? 0 : value; // If flash is erased, return 0
}

void saveHighscore(uint8_t score) {
    uint32_t interrupts = save_and_disable_interrupts();
    flash_range_erase(HIGHSCORE_FLASH_OFFSET, FLASH_SECTOR_SIZE);
    uint8_t page[FLASH_PAGE_SIZE];
    for (uint16_t i = 0; i < FLASH_PAGE_SIZE; i++) {
        page[i] = (i == 0) ? score : 0xFF; // Write score to first byte, rest are 0xFF
    }
    flash_range_program(HIGHSCORE_FLASH_OFFSET, page, FLASH_PAGE_SIZE);
    restore_interrupts(interrupts);
}