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
  tft.setCursor(20, 52);
  tft.println(F("1. Set Date/Time"));
  tft.setCursor(20, 80);
  tft.println(F("2. Edit Previous"));
  tft.setCursor(20, 108);
  tft.println(F("3. Void Payments"));
  tft.setCursor(20, 136);
  tft.println(F("4. Extra"));
  tft.setCursor(20, 164);
  tft.println(F("5. Void Reading"));
  tft.setCursor(20, 192);
  tft.println(F("6. Wireless Sync"));

  tft.drawLine(0, 225, 320, 225, COLOR_LINE);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(55, 223);
  tft.println(F("Press # or C to go back"));
}

#endif // SYSTEM_TOOLS_SCREEN_H
