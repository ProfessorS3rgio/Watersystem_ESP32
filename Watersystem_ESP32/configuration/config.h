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

// ===== TFT DISPLAY PINS (ILI9341) =====
#define TFT_CS    15
#define TFT_RST   4
#define TFT_DC    2
#define TFT_BLK   21
#define TFT_BACKLIGHT_ACTIVE_HIGH 1  // 1: BLK HIGH = ON, 0: BLK LOW = ON
#define TFT_MOSI  13
#define TFT_SCLK  14
#define TFT_MISO  12   // SD card only (safe to define)

// ===== SD CARD PINS (SPI - Separate Bus) =====
#define SD_CS     22     // SD Card Chip Select
#define SD_MOSI   23
#define SD_MISO   19
#define SD_CLK    18

// ===== THERMAL PRINTER PINS (UART2) =====
#define PRINTER_RX  17   // ESP32 RX <- Printer TX
#define PRINTER_TX  16   // ESP32 TX -> Printer RX
#define PRINTER_BAUD 9600

// ===== RTC MODULE PINS (I2C) =====
#define RTC_SDA 26
#define RTC_SCL 27

// ===== MCP23017 I/O EXPANDER (I2C) =====
#define MCP23017_ADDR 0x20  // A0-A2 grounded
#define MCP23017_INT 32     // Interrupt pin connected to ESP32 GPIO 32 

// ===== POWER SAVING =====
#define POWER_SAVE_TIMEOUT 35000  // 35 seconds in milliseconds

// ===== BATTERY MONITOR =====
#define BATTERY_PIN 25  // ADC pin for battery voltage measurement
#define CHARGING_PIN_MCP 9  // MCP23017 GPB1 for charging state

// ===== 4x4 KEYPAD PINS (via MCP23017) =====
#define KEYPAD_ROWS 4
#define KEYPAD_COLS 4
// Row Pins: MCP23017 GPA0-GPA3 (R1, R2, R3, R4)
// Col Pins: MCP23017 GPA4-GPA7 (C1, C2, C3, C4)
const byte KEYPAD_ROW_PINS[KEYPAD_ROWS] = {0, 1, 2, 3};  // GPA0-GPA3
const byte KEYPAD_COL_PINS[KEYPAD_COLS] = {4, 5, 6, 7};  // GPA4-GPA7

// ===== TFT COLORS =====
#define TFT_BLACK   0x0000
#define TFT_WHITE   0xFFFF
#define TFT_RED     0xF800
#define TFT_GREEN   0x07E0
#define TFT_BLUE    0x001F
#define TFT_CYAN    0x07FF
#define TFT_MAGENTA 0xF81F
#define TFT_YELLOW  0xFFE0
#define TFT_ORANGE  0xFD20

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
#include <Adafruit_MCP23X17.h>
sqlite3 *db = nullptr;

extern Adafruit_MCP23X17 mcp;
extern RTC_DS3231 rtc;
extern float paymentAmount;
extern BatteryMonitor batteryMonitor;

String getCurrentDateTimeString() {
  DateTime now = rtc.now();
  char buf[20];
  sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  return String(buf);
}

#endif  // CONFIG_H
