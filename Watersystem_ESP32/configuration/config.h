#ifndef CONFIG_H
#define CONFIG_H

// ===== SERIAL BAUD =====
#define SERIAL_BAUD 115200

// ===== SERIAL LOGGING =====
// 0 = minimal serial prints (recommended for Web Serial sync UI)
// 1 = verbose serial prints (debugging)
#ifndef WS_SERIAL_VERBOSE
#define WS_SERIAL_VERBOSE 0
#endif

// ===== TFT DISPLAY PINS (ST7735) =====
#define TFT_CS    5      // Chip Select
#define TFT_RST   4      // Reset
#define TFT_DC    2      // Data/Command
#define TFT_BLK   15     // Backlight (PWM for sleep mode)
// SDA (MOSI) = GPIO 23 (hardware SPI)
// SCL (CLK)  = GPIO 18 (hardware SPI)

// ===== SD CARD PINS (SPI - Shared Bus) =====
#define SD_CS     22     // SD Card Chip Select
// MOSI = GPIO 23 (shared with TFT)
// MISO = GPIO 19
// CLK  = GPIO 18 (shared with TFT)

// ===== THERMAL PRINTER PINS (UART2) =====
#define PRINTER_RX  16   // ESP32 RX <- Printer TX
#define PRINTER_TX  17   // ESP32 TX -> Printer RX
#define PRINTER_BAUD 9600

// ===== 4x4 KEYPAD PINS (Direct Wiring) =====
// Keypad pinout (left to right, keys facing you):
// [NC] [C1] [C2] [C3] [C4] [R1] [R2] [R3] [R4] [NC]
//  1    2    3    4    5    6    7    8    9    10
#define KEYPAD_ROWS 4
#define KEYPAD_COLS 4
// Row Pins: GPIO 13, 14, 27, 26 (R1, R2, R3, R4)
// Col Pins: GPIO 25, 32, 33, 21 (C1, C2, C3, C4)

// ===== TFT COLORS =====
#define COLOR_BG      ST77XX_BLACK
#define COLOR_TEXT    ST77XX_WHITE
#define COLOR_HEADER  ST77XX_CYAN
#define COLOR_AMOUNT  ST77XX_GREEN
#define COLOR_LABEL   ST77XX_YELLOW
#define COLOR_LINE    ST77XX_BLUE

// ===== LOGO BITMAP =====
#define LOGO_WIDTH 384
#define LOGO_HEIGHT 192

// ===== SD CARD DATABASE PATHS =====
#define DB_ROOT         "/WATER_DB"
#define DB_ASSETS       "/WATER_DB/ASSETS"
#define YIELD_WDT() vTaskDelay(1)

#include <sqlite3.h>
sqlite3 *db = nullptr;

#endif  // CONFIG_H
