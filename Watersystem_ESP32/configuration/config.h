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

// ===== TFT DISPLAY PINS (ILI9341 2.8") =====
#define TFT_CS    15     // Chip Select
#define TFT_RST   4      // Reset
#define TFT_DC    2      // Data/Command
#define TFT_BLK   21     // Backlight (PWM for sleep mode)
// MOSI = GPIO 13
// MISO = GPIO 12
// CLK  = GPIO 14

// ===== SD CARD PINS (SPI - Dedicated Bus) =====
#define SD_CS     22     // SD Card Chip Select
// MOSI = GPIO 23 (default SPI)
// MISO = GPIO 19
// CLK  = GPIO 18 (default SPI)

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
// Row Pins: GPIO 5, 18, 27, 26 (R1, R2, R3, R4)
// Col Pins: GPIO 25, 32, 33, 19 (C1, C2, C3, C4)

// ===== TFT COLORS =====
#define COLOR_BG      TFT_BLACK
#define COLOR_TEXT    TFT_WHITE
#define COLOR_HEADER  TFT_CYAN
#define COLOR_AMOUNT  TFT_GREEN
#define COLOR_LABEL   TFT_YELLOW
#define COLOR_LINE    TFT_BLUE

// ===== LOGO BITMAP =====
#define LOGO_WIDTH 384
#define LOGO_HEIGHT 192

// ===== SD CARD DATABASE PATHS =====
#define DB_ROOT         "/WATER_DB"
#define DB_ASSETS       "/WATER_DB/ASSETS"
#define DB_PATH         "/sd/watersystem.db"
#define YIELD_WDT() vTaskDelay(1)

#include <sqlite3.h>
sqlite3 *db = nullptr;

#endif  // CONFIG_H
