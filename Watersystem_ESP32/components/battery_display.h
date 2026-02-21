#ifndef BATTERY_DISPLAY_H
#define BATTERY_DISPLAY_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
// #include "battery_monitor.h"  // Already included in main .ino

// Function to draw battery indicator on TFT
void drawBattery(int x, int y, BatteryMonitor& batteryMonitor) {
  int level = batteryMonitor.getPercentage();
  bool isCharging = batteryMonitor.isCharging();
  // Clamp level
  level = constrain(level, 0, 100);

  // Colors by level
  uint16_t color =
    (level > 60) ? ST77XX_GREEN :
    (level > 30) ? ST77XX_YELLOW :
                   ST77XX_RED;

  // Dimensions
  int w = 34;
  int h = 14;
  int tipW = 3;
  int radius = 3;

  // Outline
  tft.drawRoundRect(x, y, w, h, radius, TFT_WHITE);
  tft.fillRoundRect(x + w, y + 4, tipW, h - 8, 2, TFT_WHITE);

  // Fill
  int fillW = map(level, 0, 100, 0, w - 4);
  tft.fillRoundRect(x + 2, y + 2, fillW, h - 4, radius - 1, color);

  // Percentage inside the battery
  tft.setTextColor(ST77XX_BLACK);
  tft.setTextFont(1);
  tft.setCursor(x + 5, y + 3);
  tft.printf("%d%%", level);

  // Charging indicator (lightning bolt)
  if (isCharging) {
    int boltX = x + w - 12;
    int boltY = y + 2;
    tft.fillTriangle(boltX + 2, boltY, boltX + 8, boltY, boltX + 4, boltY + 5, TFT_WHITE);
    tft.fillTriangle(boltX + 3, boltY + 5, boltX + 7, boltY + 5, boltX + 1, boltY + 10, TFT_WHITE);
  }
}

#endif // BATTERY_DISPLAY_H