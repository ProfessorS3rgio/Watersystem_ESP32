#ifndef PRINTER_STATUS_SCREEN_H
#define PRINTER_STATUS_SCREEN_H

#include "../configuration/config.h"
#include "../bluetooth/ble_slave_manager.h"

// ===== EXTERNAL OBJECTS =====
extern TFT_eSPI tft;

void displayPrinterStatusScreen() {
  tft.fillScreen(COLOR_BG);

  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(78, 20);
  tft.println(F("PRINTER STATUS"));

  tft.drawLine(0, 30, 320, 30, COLOR_LINE);

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

  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(15, 56);
  tft.println(F("BLE Link:"));
  tft.setCursor(15, 84);
  tft.println(F("Battery:"));
  tft.setCursor(15, 112);
  tft.println(F("Level:"));
  tft.setCursor(15, 140);
  tft.println(F("Charging:"));
  tft.setCursor(15, 168);
  tft.println(F("Paper:"));

  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(124, 56);
  tft.println(link);

  tft.setCursor(124, 84);
  if (batteryPct >= 0) {
    char pctBuf[12];
    snprintf(pctBuf, sizeof(pctBuf), "%d%%", batteryPct);
    tft.println(pctBuf);
  } else {
    tft.println(F("unknown"));
  }

  tft.setCursor(124, 112);
  tft.println(batteryLevel);

  tft.setCursor(124, 140);
  tft.println(charging);

  tft.setCursor(124, 168);
  tft.println(paper);

  tft.drawLine(0, 200, 320, 200, COLOR_LINE);

  tft.setTextColor(hasLink ? COLOR_TEXT : TFT_ORANGE);
  tft.setCursor(15, 220);
  tft.println(hasLink ? F("D=Refresh  C/#=Back") : F("Waiting slave... D=Retry"));
}

#endif  // PRINTER_STATUS_SCREEN_H
