#ifndef SYSTEM_TOOLS_SCREEN_H
#define SYSTEM_TOOLS_SCREEN_H

#include "../configuration/config.h"

extern TFT_eSPI tft;

void displaySystemToolsScreen() {
  tft.fillScreen(COLOR_BG);

  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(95, 20);
  tft.println(F("SYSTEM TOOLS"));
  tft.drawLine(0, 30, 320, 30, COLOR_LINE);

  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(20, 60);
  tft.println(F("1. Set Date/Time"));
  tft.setCursor(20, 95);
  tft.println(F("2. Edit Previous"));
  tft.setCursor(20, 130);
  tft.println(F("3. Void Payments"));

  tft.drawLine(0, 165, 320, 165, COLOR_LINE);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(55, 195);
  tft.println(F("Press # or C to go back"));
}

#endif // SYSTEM_TOOLS_SCREEN_H