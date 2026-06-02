#pragma once
#include <hardware/spi.h>
#include <hardware/gpio.h>
#include "pico/stdlib.h"
#include <stdint.h>

#define LCD_SPI_PORT spi1
#define LCD_SCK_PIN 10
#define LCD_MOSI_PIN 11
#define LCD_DC_PIN 8
#define LCD_CS_PIN 9
#define LCD_RST_PIN 15
#define LCD_BL_PIN 13

#define LCD_HEIGHT 240
#define LCD_WIDTH 320

extern const uint8_t font5x7[36][5];

#define COLOR_RED 0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_BLUE 0x001F
#define COLOR_WHITE 0xFFFF
#define COLOR_BLACK 0x0000

static inline uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue) {
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
}

class ILI9341 {
    public:
    void init();
    void setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
    void fillScreen(uint16_t color);
    void fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
    void drawPixel(uint16_t x, uint16_t y, uint16_t color);
    void drawChar(uint16_t x, uint16_t y, char c, uint16_t color);
    void drawString(uint16_t x, uint16_t y, const char* str, uint16_t color);
    void setRotation(uint8_t rotation);

    private:
    void writeCommand(uint8_t cmd);
    void writeData(uint8_t data);
    void writeData16(uint16_t data);
    void csLow() {gpio_put(LCD_CS_PIN, 0);} // Set CS low
    void csHigh() {gpio_put(LCD_CS_PIN, 1);} // Set CS
    void dcCommand() {gpio_put(LCD_DC_PIN, 0);} // Set DC low for command
    void dcData() {gpio_put(LCD_DC_PIN, 1);} // Set DC high for data
    void reset();
};