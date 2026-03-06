# ESP32 Watersystem Wiring Guide

## TFT Display (ILI9341)

| TFT Pin | ESP32       | Notes                          |
|---------|-------------|--------------------------------|
| VDD     | 3.3V        | Power (Solid Orange)           |
| GND     | GND         | Ground (Solid Black)           |
| CS      | GPIO 15     | Chip Select (Solid green)      |
| RST     | -1          | Reset not connected            |
| DC      | GPIO 2      | Data/Command (stripe blue)     |
| MOSI    | GPIO 13     | MOSI (SPI) (stripe Orange)     |
| SCK     | GPIO 14     | CLK (SPI) (Solid blue)         |
| BLK     | GPIO 21     | Backlight (stripe brown)       |

## Thermal Printer (UART2)

| Printer Pin | ESP32    | Notes                    |
|-------------|----------|--------------------------|
| TX          | GPIO 16  | ESP32 RX ← Printer TX (solid blue) |
| RX          | GPIO 17  | ESP32 TX → Printer RX (solid green) |
| GND         | GND      | Common ground (solid brown) |
| VCC         | 5V-9V    | Printer power supply (solid orange) |

## SD Card Module (SPI - Separate Bus)

| SD Card Pin | ESP32    | Notes                      |
|-------------|----------|----------------------------|
| CS          | GPIO 5   | Chip Select                |
| MOSI        | GPIO 23  | SD SPI (solid green)       |
| CLK         | GPIO 18  | SD SPI (solid blue)        |
| MISO        | GPIO 19  | SD SPI (stripe green)      |

## RTC Module (DS3231 - I2C)

| RTC Pin | ESP32    | Notes                      |
|---------|----------|----------------------------|
| VCC     | 3.3V     | Power                      |
| GND     | GND      | Ground                     |
| SDA     | GPIO 27  | I2C Data                   |
| SCL     | GPIO 22  | I2C Clock                  |
| NC      | -        | Not Connected             |

## MCP23017 I/O Expander (I2C)

| MCP23017 Pin | ESP32    | Notes                      |
|--------------|----------|----------------------------|
| VCC          | 3.3V     | Power                      |
| GND          | GND      | Ground                     |
| SDA          | GPIO 27  | I2C Data (shared with RTC) |
| SCL          | GPIO 22  | I2C Clock (shared with RTC)|
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
| 5    | SD Card CS            | SD Card Module   |
| 13   | TFT MOSI              | ILI9341 Display  |
| 14   | TFT SCLK              | ILI9341 Display  |
| 15   | TFT CS                | ILI9341 Display  |
| 16   | UART2 RX              | Thermal Printer  |
| 17   | UART2 TX              | Thermal Printer  |
| 18   | SD SPI CLK            | SD Card Module   |
| 19   | SD SPI MISO           | SD Card Module   |
| 21   | TFT Backlight         | ILI9341 Display  |
| 22   | -                     | -                 |
| 23   | SD SPI MOSI           | SD Card Module   |
| 25   | -                     | -                 |
| 26   | -                     | -                 |
| 27   | I2C SDA              | RTC Module, MCP23017 |
| 32   | MCP23017 INTA        | Interrupt from MCP23017 |
| 33   | -                     | -                 |
| 34   | -                     | -                 |
| 35   | -                     | -                 |

## ESP32 CYD Reference

### MicroSD card SPI

| Signal | GPIO |
|--------|------|
| MISO   | GPIO 19 |
| MOSI   | GPIO 23 |
| SCK    | GPIO 18 |
| CS     | GPIO 5  |

### TFT display SPI

| Signal | GPIO |
|--------|------|
| MISO (`TFT_MISO`) | GPIO 12 |
| MOSI (`TFT_MOSI`) | GPIO 13 |
| SCLK (`TFT_SCLK`) | GPIO 14 |
| CS (`TFT_CS`)     | GPIO 15 |
| DC (`TFT_DC`)     | GPIO 2  |
| RST (`TFT_RST`)   | -1      |
| Backlight         | GPIO 21 |

### Shared I2C bus

| Signal | GPIO |
|--------|------|
| SDA    | GPIO 27 |
| SCL    | GPIO 22 |

> GPIO 21 is reserved for the TFT backlight on the ESP32 CYD, so it should not be used for I2C.
> GPIO 35 is input-only and should not be used for I2C SDA/SCL.