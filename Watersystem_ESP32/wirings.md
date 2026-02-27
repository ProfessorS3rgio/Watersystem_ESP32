# ESP32 Watersystem Wiring Guide

## TFT Display (ILI9341)

| TFT Pin | ESP32       | Notes                          |
|---------|-------------|--------------------------------|
| VDD     | 3.3V        | Power (Solid Orange)           |
| GND     | GND         | Ground (Solid Black)           |
| CS      | GPIO 15     | Chip Select (Solid green)      |
| RST     | GPIO 4      | Reset (stripe green)           |
| DC      | GPIO 2      | Data/Command (stripe blue)     |
| MOSI    | GPIO 13     | MOSI (SPI) (stripe Orange)     |
| SCK     | GPIO 14     | CLK (SPI) (Solid blue)         |
| BLK     | GPIO 21     | Backlight (stripe brown)       |

## Thermal Printer (UART2)

| Printer Pin | ESP32    | Notes                    |
|-------------|----------|--------------------------|
| TX          | GPIO 16  | ESP32 RX ← Printer TX (solid blue) |
| RX          | GPIO 17  | ESP32 TX → Printer RX (solid green) |
| DTR         | GPIO 5   | ESP32 DTR → Printer DTR (stripe blue) |
| GND         | GND      | Common ground (solid brown) |
| VCC         | 5V-9V    | Printer power supply (solid orange) |

## SD Card Module (SPI - Separate Bus)

| SD Card Pin | ESP32    | Notes                      |
|-------------|----------|----------------------------|
| CS          | GPIO 22  | Chip Select (dedicated) (stripe blue) |
| MOSI        | GPIO 23  | SD SPI (solid green)       |
| CLK         | GPIO 18  | SD SPI (solid blue)        |
| MISO        | GPIO 19  | SD SPI (stripe green)      |

## RTC Module (DS3231 - I2C)

| RTC Pin | ESP32    | Notes                      |
|---------|----------|----------------------------|
| VCC     | 3.3V     | Power                      |
| GND     | GND      | Ground                     |
| SDA     | GPIO 26  | I2C Data                   |
| SCL     | GPIO 27  | I2C Clock                  |
| NC      | -        | Not Connected             |

## MCP23017 I/O Expander (I2C)

| MCP23017 Pin | ESP32    | Notes                      |
|--------------|----------|----------------------------|
| VCC          | 3.3V     | Power                      |
| GND          | GND      | Ground                     |
| SDA          | GPIO 26  | I2C Data (shared with RTC) |
| SCL          | GPIO 27  | I2C Clock (shared with RTC)|
| RESET        | 3.3V     | Reset (active low, tie high)|
| A0           | GND      | Address bit 0 (address 0x20)|
| A1           | GND      | Address bit 1              |
| A2           | GND      | Address bit 2              |

## 4x4 Matrix Keypad (via MCP23017)

> **Note:** Keypad connected to MCP23017 GPIO pins. Board labels are used (e.g., A0 = GPA0).

| Keypad Pin | MCP23017 Board Pin | MCP23017 GPIO | Wire Color    | Notes                      |
|------------|---------------------|---------------|----------------|----------------------------|
| R1         | A0                  | GPA0          | stripe blue   | Row 1                      |
| R2         | A1                  | GPA1          | stripe green  | Row 2                      |
| R3         | A2                  | GPA2          | solid blue    | Row 3                      |
| R4         | A3                  | GPA3          | solid green   | Row 4                      |
| C1         | A4                  | GPA4          | stripe yellow | Column 1                   |
| C2         | A5                  | GPA5          | stripe brown  | Column 2                   |
| C3         | A6                  | GPA6          | solid brown   | Column 3                   |
| C4         | A7                  | GPA7          | solid orange  | Column 4                   |

> **Keypad Layout:**
> ```
>       C1  C2  C3  C4
> R1   [1] [2] [3] [A]
> R2   [4] [5] [6] [B]
> R3   [7] [8] [9] [C]
> R4   [*] [0] [#] [D]
> ```

## Pin Summary

| GPIO | Function              | Peripheral       |
|------|-----------------------|------------------|
| 2    | TFT DC                | ILI9341 Display  |
| 4    | TFT RST               | ILI9341 Display  |
| 5    | Printer DTR            | Thermal Printer  |
| 13   | TFT MOSI              | ILI9341 Display  |
| 14   | TFT SCLK              | ILI9341 Display  |
| 15   | TFT CS                | ILI9341 Display  |
| 16   | UART2 RX              | Thermal Printer  |
| 17   | UART2 TX              | Thermal Printer  |
| 18   | SD SPI CLK            | SD Card Module   |
| 19   | SD SPI MISO           | SD Card Module   |
| 21   | TFT Backlight         | ILI9341 Display  |
| 22   | SD Card CS            | SD Card Module   |
| 23   | SD SPI MOSI           | SD Card Module   |
| 25   | -                     | -                 |
| 26   | I2C SDA              | RTC Module, MCP23017 |
| 27   | I2C SCL              | RTC Module, MCP23017 |
| 32   | MCP23017 INTA        | Interrupt from MCP23017 |
| 33   | -                     | -                 |
| 34   | -                     | -                 |
| 35   | -                     | -                 |