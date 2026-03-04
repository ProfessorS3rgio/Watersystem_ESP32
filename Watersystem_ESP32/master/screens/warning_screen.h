#ifndef WARNING_SCREEN_H
#define WARNING_SCREEN_H

#include "../configuration/config.h"
#include <TFT_eSPI.h>

// ===== EXTERNAL OBJECTS =====
extern TFT_eSPI tft;

static inline void wsDrawWarningTriangle(int cx, int cy, int size, uint16_t colorFill, uint16_t colorLine) {
  // Simple triangle icon with an exclamation mark
  int x0 = cx;
  int y0 = cy - size;
  int x1 = cx - size;
  int y1 = cy + size;
  int x2 = cx + size;
  int y2 = cy + size;

  tft.fillTriangle(x0, y0, x1, y1, x2, y2, colorFill);
  tft.drawTriangle(x0, y0, x1, y1, x2, y2, colorLine);

  // Exclamation mark
  tft.fillRoundRect(cx - 3, cy - 6, 6, 18, 3, TFT_BLACK);
  tft.fillCircle(cx, cy + 16, 3, TFT_BLACK);
}

static inline void displayWarningScreen(const __FlashStringHelper *title,
                                       const String &line1,
                                       const String &line2,
                                       const __FlashStringHelper *footerHint) {
  const int w = tft.width();
  const int h = tft.height();

  tft.fillScreen(COLOR_BG);

  // Accent bars in red/orange for warning
  tft.fillRect(0, 0, w, 5, TFT_RED);
  tft.fillRect(0, h - 5, w, 5, TFT_RED);

  // Header
  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(TFT_RED);
  tft.setCursor(15, 24);
  tft.println(title);
  tft.drawLine(0, 35, w, 35, COLOR_LINE);

  // Icon
  wsDrawWarningTriangle(w / 2, 78, 18, TFT_ORANGE, TFT_RED);

  // Message
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_TEXT);

  tft.setCursor(18, 120);
  tft.println(line1);

  if (line2.length() > 0) {
    tft.setCursor(18, 142);
    tft.println(line2);
  }

  // Footer hint
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(18, 190);
  tft.println(footerHint);
}

#endif  // WARNING_SCREEN_H
