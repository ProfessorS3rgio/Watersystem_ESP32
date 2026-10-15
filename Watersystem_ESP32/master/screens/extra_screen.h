#ifndef EXTRA_SCREEN_H
#define EXTRA_SCREEN_H

#include "../configuration/config.h"

extern TFT_eSPI tft;

void displayExtraScreen() {
  tft.fillScreen(COLOR_BG);

  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(125, 20);
  tft.println(F("EXTRA"));
  tft.drawLine(0, 30, 320, 30, COLOR_LINE);

  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(20, 65);
  tft.println(F("1. Reset Device Profile"));
  tft.setCursor(20, 105);
  tft.println(F("2. Select Printer/Slave"));

  tft.drawLine(0, 145, 320, 145, COLOR_LINE);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(55, 180);
  tft.println(F("Press # or C to go back"));
}

#endif // EXTRA_SCREEN_H
