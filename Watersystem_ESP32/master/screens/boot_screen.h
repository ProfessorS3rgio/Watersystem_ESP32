#ifndef BOOT_SCREEN_H
#define BOOT_SCREEN_H

#include <TFT_eSPI.h>
#include "../managers/sdcard_manager.h"
#include "../configuration/config.h"

// Extern declarations for global objects
extern TFT_eSPI tft;
extern RTC_DS3231 rtc;   // allow RTC checks

// ===== BOOT SCREEN HELPERS =====
static bool checkPrinterCommunication(uint16_t /*timeoutMs*/ = 0) {
  // no actual printer attached in master; always report false
  return false;
}

static void showBootScreen() {
  tft.fillScreen(COLOR_BG);
  tft.setTextSize(1);

  int y = 2;
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(2, y);
  tft.println(F("[BOOT] Watersystem ESP32"));
  y += 12;

  tft.setCursor(2, y);
  tft.println(F("[CHK] SD Card..."));

  initSDCard();
  y += 12;
  tft.setCursor(2, y);
  tft.print(F("[SD ] Status: "));
  if (isSDCardReady()) {
    tft.setTextColor(TFT_GREEN);
    tft.println(F("OK"));
  } else {
    tft.setTextColor(TFT_RED);
    tft.println(F("FAIL"));
  }
  tft.setTextColor(TFT_GREEN);

  // show RTC presence
  y += 12;
  tft.setCursor(2, y);
  tft.print(F("[RTC] "));
  if (rtc.begin()) {
    tft.setTextColor(TFT_GREEN);
    tft.println(F("found"));
  } else {
    tft.setTextColor(TFT_RED);
    tft.println(F("missing"));
  }
  tft.setTextColor(TFT_GREEN);

  y += 12;
  tft.setCursor(2, y);
  tft.println(F("[CHK] Printer..."));


  // check MCP23017 on-screen as well
  y += 12;
  tft.setCursor(2, y);
  tft.print(F("[MCP] "));
  if (mcp.begin_I2C(MCP23017_ADDR)) {
    g_mcpReady = true;
    tft.setTextColor(TFT_GREEN);
    tft.println(F("OK"));
    // restore the pinMode we set earlier since begin_I2C resets pins
    mcp.pinMode(CHARGING_PIN_MCP, INPUT);
  } else {
    g_mcpReady = false;
    tft.setTextColor(TFT_RED);
    tft.println(F("FAIL"));
  }
  tft.setTextColor(TFT_GREEN);



  y += 12;
  tft.setCursor(2, y);
  tft.println(F("[OK ] Starting UI..."));

  int countdownY = y + 12;
  for (int i = 5; i >= 1; i--) {
    tft.fillRect(0, countdownY, 160, 10, COLOR_BG);
    tft.setTextColor(COLOR_LABEL);
    tft.setCursor(2, countdownY);
    tft.print(F("Booting inwithin "));
    tft.print(i);
    tft.print(F("..."));
    delay(1000);
  }
  tft.fillRect(0, countdownY, 160, 10, COLOR_BG);
  delay(300);
}

#endif  // BOOT_SCREEN_H