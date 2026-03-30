#ifndef PRINTER_STATUS_SCREEN_H
#define PRINTER_STATUS_SCREEN_H

#include "../configuration/config.h"
#include "../bluetooth/ble_slave_manager.h"

// ===== EXTERNAL OBJECTS =====
extern TFT_eSPI tft;

static void drawCardFrame(int y) {
  tft.drawRoundRect(8, y, 304, 36, 6, COLOR_LINE);
}

static void drawBleIcon(int x, int y, bool connected) {
  const uint16_t c = connected ? TFT_GREEN : TFT_ORANGE;
  tft.drawLine(x + 6, y + 2, x + 6, y + 22, c);
  tft.drawLine(x + 6, y + 2, x + 12, y + 8, c);
  tft.drawLine(x + 6, y + 2, x, y + 8, c);
  tft.drawLine(x + 6, y + 12, x + 12, y + 6, c);
  tft.drawLine(x + 6, y + 12, x, y + 6, c);
  tft.drawLine(x + 6, y + 12, x + 12, y + 18, c);
  tft.drawLine(x + 6, y + 12, x, y + 18, c);
  tft.drawLine(x + 6, y + 22, x + 12, y + 16, c);
  tft.drawLine(x + 6, y + 22, x, y + 16, c);
}

static void drawBatteryIcon(int x, int y, int pct) {
  tft.drawRect(x, y, 24, 12, COLOR_TEXT);
  tft.drawRect(x + 24, y + 3, 2, 6, COLOR_TEXT);

  int bars = 0;
  if (pct >= 80) bars = 4;
  else if (pct >= 55) bars = 3;
  else if (pct >= 30) bars = 2;
  else if (pct >= 10) bars = 1;

  const uint16_t fillColor = pct < 25 ? TFT_RED : (pct < 55 ? TFT_YELLOW : TFT_GREEN);
  for (int i = 0; i < bars; i++) {
    tft.fillRect(x + 2 + (i * 5), y + 2, 4, 8, fillColor);
  }
}

static void drawChargingIcon(int x, int y, bool charging) {
  const uint16_t c = charging ? TFT_GREEN : TFT_ORANGE;
  tft.drawRoundRect(x, y, 16, 22, 3, c);
  tft.fillTriangle(x + 9, y + 3, x + 5, y + 12, x + 9, y + 12, c);
  tft.fillTriangle(x + 7, y + 10, x + 11, y + 10, x + 7, y + 19, c);
}

static void drawPaperIcon(int x, int y, bool present) {
  const uint16_t c = present ? TFT_GREEN : TFT_ORANGE;
  tft.drawRect(x, y, 16, 20, c);
  tft.drawLine(x + 11, y, x + 15, y + 4, c);
  tft.drawLine(x + 11, y + 1, x + 11, y + 4, c);
  tft.drawLine(x + 11, y + 4, x + 15, y + 4, c);
  tft.drawFastHLine(x + 3, y + 8, 9, c);
  tft.drawFastHLine(x + 3, y + 11, 9, c);
  tft.drawFastHLine(x + 3, y + 14, 9, c);
}

void displayPrinterStatusScreen() {
  tft.fillScreen(COLOR_BG);

  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(76, 21);
  tft.println(F("PRINTER STATUS"));

  tft.drawLine(0, 32, 320, 32, COLOR_LINE);

  // Printing flow intentionally disconnects BLE after each job, so
  // status screen must actively reconnect before requesting telemetry.
  bleSlaveManagerBegin();

  bool ready = blePrepareForPrint(9000);
  if (!ready) {
    // Force one reconnect cycle in case the first attempt raced slave advertising startup.
    bleRequestReconnect();
    delay(250);
    ready = blePrepareForPrint(9000);
  }

  const bool hasLink = ready && bleRequestSlaveStatus(3000);
  const String link = bleConnectionStatusText();

  const int batteryPct = bleSlaveBatteryPercent();
  const String batteryLevel = bleSlaveBatteryLevelText();
  const String charging = bleSlaveChargingStatusText();
  const String paper = bleSlavePaperStatusText();

  const bool chargingNow = charging.equalsIgnoreCase("charging");
  const bool paperPresent = paper.equalsIgnoreCase("present");

  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_LABEL);
  drawCardFrame(42);
  drawCardFrame(82);
  drawCardFrame(122);
  drawCardFrame(162);

  drawBleIcon(18, 49, hasLink);
  tft.setCursor(44, 65);
  tft.println(F("BLE Link"));
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(186, 65);
  tft.println(link);

  drawBatteryIcon(16, 94, batteryPct < 0 ? 0 : batteryPct);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(44, 105);
  tft.println(F("Battery"));
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(146, 105);
  if (batteryPct >= 0) {
    char pctBuf[12];
    snprintf(pctBuf, sizeof(pctBuf), "%d%%", batteryPct);
    tft.print(pctBuf);
    tft.print(F("  "));
    tft.println(batteryLevel);
  } else {
    tft.println(F("unknown"));
  }

  drawChargingIcon(16, 128, chargingNow);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(44, 145);
  tft.println(F("Charging"));
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(146, 145);
  tft.println(charging);

  drawPaperIcon(16, 168, paperPresent);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(44, 185);
  tft.println(F("Paper"));
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(146, 185);
  tft.println(paper);

  tft.drawLine(0, 206, 320, 206, COLOR_LINE);

  tft.setTextColor(hasLink ? COLOR_TEXT : TFT_ORANGE);
  tft.setCursor(14, 228);
  tft.println(hasLink ? F("D=Refresh  C/#=Back") : F("Waiting slave... D=Retry"));
}

#endif  // PRINTER_STATUS_SCREEN_H
