# ESP32 Test Wiring for TPM408-2.8 TFT Touchscreen

This guide is for testing the TPM408-2.8 TFT touchscreen module on a standalone ESP32 board. It uses minimal wiring for display and touch functionality.

## TFT Display (TPM408-2.8)

| TFT Pin | ESP32       | Notes                          |
|---------|-------------|--------------------------------|
| VDD     | 3.3V        | Power (or 5V if needed)        |
| GND     | GND         | Ground                         |
| D/C     | GPIO 2      | Data/Command                   |
| SDI(MOSI)| GPIO 13    | SDI (MOSI Hardware SPI)        |
| SCK     | GPIO 14     | SCK (Hardware SPI)             |
| LED     | GPIO 21     | LED (Backlight PWM for sleep mode) |

## Touchscreen (XPT2046)

| Touch Pin | ESP32    | Notes                      |
|-----------|----------|----------------------------|
| T_CS      | GPIO 33  | Touch Chip Select          |
| T_IRQ     | GPIO 4   | Touch Interrupt            |
| T_OUT     | GPIO 5   | Touch Data Out (MISO)      |
| T_DIN     | GPIO 32  | Touch Data In (MOSI)       |
| T_CLK     | GPIO 25  | Touch Clock (SCK)          |

## Pin Summary

| GPIO | Function              | Peripheral       |
|------|-----------------------|------------------|
| 2    | TFT D/C               | TPM408-2.8 Display |
| 4    | Touch IRQ             | Touchscreen      |
| 5    | Touch OUT             | Touchscreen      |
| 12   | SPI MISO              | Touchscreen      |
| 13   | SPI MOSI              | TFT + Touch      |
| 14   | SPI SCK               | TFT + Touch      |
| 21   | TFT LED               | TPM408-2.8 Display |
| 25   | Touch CLK             | Touchscreen      |
| 32   | Touch DIN             | Touchscreen      |
| 33   | Touch CS              | Touchscreen      |

## Notes
- Use hardware SPI (VSPI) for best performance.
- Power the TFT with 3.3V if possible; switch to 5V if display is dim.
- For software: Use TFT_eSPI library for display and XPT2046_Touchscreen for touch. Calibrate touch after setup.
- Test display first, then add touch.</content>
<parameter name="filePath">c:\Users\ProfessorS3rgio\Documents\Arduino\Watersystem\Watersystem_ESP32\test_tft_touchscreen.md