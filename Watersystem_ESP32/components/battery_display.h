#ifndef BATTERY_DISPLAY_H
#define BATTERY_DISPLAY_H

#include <Adafruit_GFX.h>

// Function to draw battery indicator on TFT
void drawBattery(int x, int y, int level) {
  // Clamp level
  level = constrain(level, 0, 100);

  // Colors by level
  uint16_t color =
    (level > 60) ? TFT_GREEN :
    (level > 30) ? TFT_YELLOW :
                   TFT_RED;

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

  // Percentage (optional, small & subtle)
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.setCursor(x + w + tipW + 6, y + 3);
  tft.print(level);
  tft.print('%');
}

#endif // BATTERY_DISPLAY_H