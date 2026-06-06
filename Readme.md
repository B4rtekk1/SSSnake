# SSSnake

SSSnake is a classic Snake game for Raspberry Pi Pico 2, shown on an ILI9341 display and controlled with four physical buttons. It combines C++ game logic with a few ARM assembly helpers for fast board and snake operations.

## Features

- 15 x 15 game board rendered on an ILI9341 display,
- four-button directional controls: `UP`, `LEFT`, `DOWN`, `RIGHT`,
- collision detection for walls and the snake body,
- random apple placement on free cells,
- PWM buzzer melody after eating an apple,
- persistent high score stored in Pico flash memory,
- start screen, lose screen, and button-based restart.

## Hardware

You will need:

- Raspberry Pi Pico 2,
- SPI display with an ILI9341 controller,
- 4 momentary push buttons,
- buzzer,
- breadboard and jumper wires,
- 3.3 V power from the Pico.

The buttons are active-low, so each button connects its GPIO pin to ground. The firmware enables the Pico's internal pull-up resistors.

## Wiring

| Part | Pico 2 GPIO | Description |
| --- | --- | --- |
| UP button | GPIO2 | move up |
| LEFT button | GPIO3 | move left |
| DOWN button | GPIO4 | move down |
| RIGHT button | GPIO5 | move right |
| LCD DC | GPIO8 | data/command line |
| LCD CS | GPIO9 | chip select |
| LCD SCK | GPIO10 | SPI clock |
| LCD MOSI / DIN | GPIO11 | SPI data |
| LCD BL | GPIO13 | backlight |
| LCD RST | GPIO15 | display reset |
| Buzzer | GPIO18 | PWM output |

```mermaid

graph LR
    subgraph Raspberry_Pi_Pico_2["Raspberry Pi Pico 2"]
        P33["3.3V"]
        PGND["GND"]
        GPIO2["GPIO2\nUP button"]
        GPIO3["GPIO3\nLEFT button"]
        GPIO4["GPIO4\nDOWN button"]
        GPIO5["GPIO5\nRIGHT button"]
        GPIO8["GPIO8\nLCD DC"]
        GPIO9["GPIO9\nLCD CS"]
        GPIO10["GPIO10\nLCD SCK"]
        GPIO11["GPIO11\nLCD MOSI"]
        GPIO13["GPIO13\nLCD BL"]
        GPIO15["GPIO15\nLCD RST"]
        GPIO18["GPIO18\nBuzzer PWM"]
    end

    subgraph Breadboard["Breadboard"]
        VRAIL["+3.3V rail"]
        GRAIL["GND rail"]
        BTN_UP["Button UP\n(active low)"]
        BTN_LEFT["Button LEFT\n(active low)"]
        BTN_DOWN["Button DOWN\n(active low)"]
        BTN_RIGHT["Button RIGHT\n(active low)"]
        BUZZER["Buzzer"]
        LCD_CONN["ILI9341 module"]
    end

    subgraph ILI9341["ILI9341"]
        VCC
        GND
        DIN
        CLK
        CS
        DC
        RST
        BL
    end

    P33 --> VRAIL
    PGND --> GRAIL

    GPIO2 --> BTN_UP
    GPIO3 --> BTN_LEFT
    GPIO4 --> BTN_DOWN
    GPIO5 --> BTN_RIGHT
    BTN_UP --> GRAIL
    BTN_LEFT --> GRAIL
    BTN_DOWN --> GRAIL
    BTN_RIGHT --> GRAIL

    GPIO18 --> BUZZER

    GPIO11 --> LCD_CONN
    GPIO10 --> LCD_CONN
    GPIO9 --> LCD_CONN
    GPIO8 --> LCD_CONN
    GPIO15 --> LCD_CONN
    GPIO13 --> LCD_CONN

    VRAIL --> VCC
    GRAIL --> GND
    LCD_CONN --> DIN
    LCD_CONN --> CLK
    LCD_CONN --> CS
    LCD_CONN --> DC
    LCD_CONN --> RST
    LCD_CONN --> BL
```

## Building

The project uses Raspberry Pi Pico SDK and CMake. For a clean build:

```powershell
cmake -S . -B build -DPICO_BOARD=pico2
cmake --build build
```

After a successful build, the UF2 file should be available at:

```text
build/SSSnake.uf2
```

## Flashing

1. Hold the `BOOTSEL` button on the Pico 2.
2. Connect the board to your computer over USB.
3. Copy `build/SSSnake.uf2` to the `RPI-RP2` drive.
4. The Pico will reboot automatically and start the game.

## Controls

- `UP`, `LEFT`, `DOWN`, `RIGHT` change the snake direction.
- The game prevents immediate 180-degree turns.
- After losing, press any button to start a new round.

## Project Structure

| File | Role |
| --- | --- |
| `SSSnake.cpp` | main game logic, controls, score, sound, and game loop |
| `ili9341.cpp` / `ili9341.h` | simple ILI9341 display driver |
| `asm_utils.s` | assembly helper functions for board and snake operations |
| `CMakeLists.txt` | Pico SDK build configuration |
| `pico_sdk_import.cmake` | standard Raspberry Pi Pico SDK import file |

## Notes

The high score is stored in the last sector of flash memory. If you change the program memory layout or add your own flash storage, make sure it does not collide with the address used by `HIGHSCORE_FLASH_OFFSET`.
