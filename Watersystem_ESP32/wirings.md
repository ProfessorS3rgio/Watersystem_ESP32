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

## RTC Module (DS3231 - I2C)

| RTC Pin | ESP32    | Notes                      |
|---------|----------|----------------------------|
| VCC     | 3.3V     | Power                      |
| GND     | GND      | Ground                     |
| SDA     | GPIO 26  | I2C Data                   |
| SCL     | GPIO 27  | I2C Clock                  |
| NC      | -        | Not Connected             |

## 4x4 Matrix Keypad (Direct Wiring)

> **Note:** 10-pin keypad — Pin 1 and Pin 10 are NC (Not Connected).
> Use pins 2-9 only (viewing keypad from front, left to right).

| Keypad Pin | ESP32    | Notes                      |
|------------|----------|----------------------------|
| Pin 1      | -        | NC (Not Connected)         |
| Pin 2 (C1) | -        | NC (Not Connected)         |
| Pin 3 (C2) | -        | NC (Not Connected)         |
| Pin 4 (C3) | -        | NC (Not Connected)         |
| Pin 5 (C4) | -        | NC (Not Connected)         |
| Pin 6 (R1) | -        | NC (Not Connected)         |
| Pin 7 (R2) | -        | NC (Not Connected)         |
| Pin 8 (R3) | -        | NC (Not Connected)         |
| Pin 9 (R4) | -        | NC (Not Connected)         |
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
| 2    | TFT DC                | ILI9341 Display  |
| 4    | TFT RST               | ILI9341 Display  |
| 5    | -                     | -                 |
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
| 26   | RTC SDA              | RTC Module        |
| 27   | RTC SCL              | RTC Module        |
| 32   | -                     | -                 |
| 33   | -                     | -                 |
| 34   | -                     | -                 |
| 35   | -                     | -                 |