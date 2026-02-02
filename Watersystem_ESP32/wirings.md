# ESP32 Watersystem Wiring Guide

## TFT Display (ILI9341)

| TFT Pin | ESP32       | Notes                          |
|---------|-------------|--------------------------------|
| VDD     | 3.3V        | Power                          |
| GND     | GND         | Ground                         |
| CS      | GPIO 15     | Chip Select                    |
| RST     | GPIO 4      | Reset                          |
| DC      | GPIO 2      | Data/Command                   |
| MOSI    | GPIO 13     | MOSI (SPI)                     |
| SCK     | GPIO 14     | CLK (SPI)                      |
| BLK     | GPIO 21     | Backlight                      |

## Thermal Printer (UART2)

| Printer Pin | ESP32    | Notes                    |
|-------------|----------|--------------------------|
| TX          | GPIO 16  | ESP32 RX ← Printer TX    |
| RX          | GPIO 17  | ESP32 TX → Printer RX    |
| GND         | GND      | Common ground            |
| VCC         | 5V-9V    | Printer power supply     |

## SD Card Module (SPI - Separate Bus)

| SD Card Pin | ESP32    | Notes                      |
|-------------|----------|----------------------------|
| 3V3         | 3.3V     | Power                      |
| GND         | GND      | Ground                     |
| CS          | GPIO 22  | Chip Select (dedicated)    |
| MOSI        | GPIO 23  | SD SPI                     |
| CLK         | GPIO 18  | SD SPI                     |
| MISO        | GPIO 19  | SD SPI                     |

## 4x4 Matrix Keypad (Direct Wiring)

> **Note:** 10-pin keypad — Pin 1 and Pin 10 are NC (Not Connected).
> Use pins 2-9 only (viewing keypad from front, left to right).

| Keypad Pin | ESP32    | Notes                      |
|------------|----------|----------------------------|
| Pin 1      | -        | NC (Not Connected)         |
| Pin 2 (C1) | GPIO 25  | Column 1                   |
| Pin 3 (C2) | GPIO 32  | Column 2                   |
| Pin 4 (C3) | GPIO 33  | Column 3                   |
| Pin 5 (C4) | GPIO 21  | Column 4                   |
| Pin 6 (R1) | GPIO 13  | Row 1                      |
| Pin 7 (R2) | GPIO 14  | Row 2                      |
| Pin 8 (R3) | GPIO 27  | Row 3                      |
| Pin 9 (R4) | GPIO 26  | Row 4                      |
| Pin 10     | -        | NC (Not Connected)         |

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
| 2    | TFT DC                | ST7735 Display   |
| 4    | TFT RST               | ST7735 Display   |
| 5    | TFT CS                | ST7735 Display   |
| 13   | Keypad R1             | 4x4 Keypad       |
| 14   | Keypad R2             | 4x4 Keypad       |
| 15   | TFT Backlight         | ST7735 Display   |
| 16   | UART2 RX              | Thermal Printer  |
| 17   | UART2 TX              | Thermal Printer  |
| 18   | SPI CLK               | TFT + SD Card    |
| 19   | SPI MISO              | SD Card Module   |
| 21   | Keypad C4             | 4x4 Keypad       |
| 22   | SD Card CS            | SD Card Module   |
| 23   | SPI MOSI              | TFT + SD Card    |
| 25   | Keypad C1             | 4x4 Keypad       |
| 26   | Keypad R4             | 4x4 Keypad       |
| 27   | Keypad R3             | 4x4 Keypad       |
| 32   | Keypad C2             | 4x4 Keypad       |
| 33   | Keypad C3             | 4x4 Keypad       |