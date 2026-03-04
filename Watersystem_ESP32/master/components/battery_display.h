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

  // Charging indicator (modern lightning bolt) on the right side
  if (isCharging) {
    int boltX = x + w + tipW + 2;
    int boltY = y + 1;

    uint16_t boltColor = ST77XX_YELLOW;  // ⚡ Yellow bolt

    // Top angled part
    tft.fillTriangle(
      boltX + 6, boltY,
      boltX + 12, boltY,
      boltX + 7, boltY + 6,
      boltColor
    );

    // Bottom angled part
    tft.fillTriangle(
      boltX + 5, boltY + 6,
      boltX + 10, boltY + 6,
      boltX + 3, boltY + 13,
      boltColor
    );
  }
}

#endif // BATTERY_DISPLAY_H