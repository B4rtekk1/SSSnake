#include <ili9341.h>
#include <cstring>

#define MHZ 1000000

const uint8_t font5x7[36][5] = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}  // Z
};

void ILI9341::reset() {
    gpio_put(LCD_RST_PIN, 1);
    sleep_ms(5);
    gpio_put(LCD_RST_PIN, 0);
    sleep_ms(20);
    gpio_put(LCD_RST_PIN, 1);
    sleep_ms(150);
}

void ILI9341::writeData16(uint16_t data) {
    uint8_t buf[2] = {
        static_cast<uint8_t>(data >> 8),
        static_cast<uint8_t>(data & 0xFF)
    };
    dcData();
    csLow();
    spi_write_blocking(LCD_SPI_PORT, buf, 2);
    csHigh();
};

void ILI9341::writeData(uint8_t data) {
    dcData();
    csLow();
    spi_write_blocking(LCD_SPI_PORT, &data, 1);
    csHigh();
};

void ILI9341::writeCommand(uint8_t cmd) {
    dcCommand();
    csLow();
    spi_write_blocking(LCD_SPI_PORT, &cmd, 1);
    csHigh();
};

void ILI9341::init() {
    spi_init(LCD_SPI_PORT, 40 * MHZ);
    spi_set_format(LCD_SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(LCD_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(LCD_MOSI_PIN, GPIO_FUNC_SPI);

    gpio_init(LCD_DC_PIN); gpio_set_dir(LCD_DC_PIN, GPIO_OUT);
    gpio_init(LCD_CS_PIN); gpio_set_dir(LCD_CS_PIN, GPIO_OUT);
    gpio_init(LCD_RST_PIN); gpio_set_dir(LCD_RST_PIN, GPIO_OUT);
    gpio_init(LCD_BL_PIN); gpio_set_dir(LCD_BL_PIN, GPIO_OUT);

    gpio_put(LCD_BL_PIN, 1); // Turn on backlight
    gpio_put(LCD_CS_PIN, 1);
    reset();

    writeCommand(0x01); // Software reset
    sleep_ms(20);
    writeCommand(0x11); // Sleep out
    sleep_ms(120);

    writeCommand(0xCF); // Power control B
    writeData(0x00); writeData(0xC1); writeData(0x30);

    writeCommand(0xED); // Power on sequence control
    writeData(0x64); writeData(0x03); writeData(0x12); writeData(0x81);

    writeCommand(0xE8); // Driver timing control A
    writeData(0x85); writeData(0x00); writeData(0x78);

    writeCommand(0xCB); // Power control A
    writeData(0x39); writeData(0x2C); writeData(0x00); writeData(0x34); writeData(0x02);

    writeCommand(0xF7); writeData(0x20); // Pump ratio control
    writeCommand(0xEA); writeData(0x00); writeData(0x00); // Driver timing control B

    writeCommand(0xC0); writeData(0x23); // Power control 1
    writeCommand(0xC1); writeData(0x10); // Power control 2
    writeCommand(0xC5); writeData(0x3e); writeData(0x28); // VCOM control 1
    writeCommand(0xC7); writeData(0x86); // VCOM control 2

    writeCommand(0x36); writeData(0x48); // Memory access control
    writeCommand(0x3A); writeData(0x55); // Pixel format 16-bit per pixel

    writeCommand(0xB1); writeData(0x00); writeData(0x18); // Frame rate control
    writeCommand(0xB6); writeData(0x08); writeData(0x82); writeData(0x27); // Display function control
    writeCommand(0xF2); writeData(0x00); // 3Gamma function disable

    writeCommand(0x26); writeData(0x01); // Gamma curve selected
    writeCommand(0xE0); // Positive gamma correction
    writeData(0x0F); writeData(0x31); writeData(0x2B); writeData(0x0C); writeData(0x0E);
    writeData(0x08); writeData(0x4E); writeData(0xF1); writeData(0x37); writeData(0x07);
    writeData(0x10); writeData(0x03); writeData(0x0E); writeData(0x09); writeData(0x00);
    writeCommand(0xE1); // Negative gamma correction
    writeData(0x00); writeData(0x0E); writeData(0x14); writeData(0x03); writeData(0x11);
    writeData(0x07); writeData(0x31); writeData(0xC1); writeData(0x48); writeData(0x08);
    writeData(0x0F); writeData(0x0C); writeData(0x31); writeData(0x36); writeData(0x0F);

    writeCommand(0x11); // Sleep out
    sleep_ms(120);
    writeCommand(0x29); // Display on
}

void ILI9341::setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    writeCommand(0x2A); // Column address set
    writeData16(x0);
    writeData16(x1);
    writeCommand(0x2B); // Page address set
    writeData16(y0);
    writeData16(y1);
    writeCommand(0x2C); // Memory write
}

void ILI9341::fillScreen(uint16_t color) {
    fillRect(0, 0, LCD_WIDTH, LCD_HEIGHT, color);
}

void ILI9341::fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    setWindow(x, y, x + w - 1, y + h - 1);
    uint8_t hi = color >> 8, lo = color & 0xFF;
    uint8_t buf[2] = {hi, lo};
    dcData();
    csLow();

    for (uint32_t i = 0; i < (uint32_t)w * h; i++) {
        spi_write_blocking(LCD_SPI_PORT, buf, 2);
    }
    csHigh();
}

void ILI9341::drawPixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return; // Out of bounds
    setWindow(x, y, x, y);
    writeData16(color);
};

void ILI9341::setRotation(uint8_t rotation) {
    writeCommand(0x36); // Memory access control
    switch (rotation % 4) {
        case 0: writeData(0x48); break; // Portrait
        case 1: writeData(0x28); break; // Landscape
        case 2: writeData(0x88); break; // Inverted portrait
        case 3: writeData(0xE8); break; // Inverted landscape
    }
}

void ILI9341::drawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint8_t scale) {
    uint8_t index;

    if (c >= '0' && c <= '9') {
        index = static_cast<uint8_t>(c - '0');
    } else if (c >= 'a' && c <= 'z') {
        index = static_cast<uint8_t>(10 + (c - 'a'));
    } else if (c >= 'A' && c <= 'Z') {
        index = static_cast<uint8_t>(10 + (c - 'A'));
    } else if (c == ' ') {
        return;
    } else {
        return;
    }

    for (uint8_t col = 0; col < 5; col++) {
        uint8_t colData = font5x7[index][col];
        for (uint8_t row = 0; row < 7; row++) {
            if (colData & (1u << row)) {
                if (scale <= 1) {
                    drawPixel(x + col, y + row, color);
                } else {
                    fillRect(x + col * scale, y + row * scale, scale, scale, color);
                }
            }
        }
    }
}

void ILI9341::drawString(uint16_t x, uint16_t y, const char* str, uint16_t color, uint8_t scale) {
    uint16_t cursorX = x;
    while (*str) {
        if (*str == ' ') {
            cursorX += 6 * scale;
        } else {
            drawChar(cursorX, y, *str, color, scale);
            cursorX += 6 * scale; // 5 pixels for char + 1 pixel space, scaled
        }
        ++str;
    }
}
