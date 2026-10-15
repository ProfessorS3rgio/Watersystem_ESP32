#ifndef PRINTER_SELECT_SCREEN_H
#define PRINTER_SELECT_SCREEN_H

#include "../configuration/config.h"
#include "../configuration/device_profile.h"

extern TFT_eSPI tft;

void displayPrinterSelectScreen() {
  tft.fillScreen(COLOR_BG);

  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(65, 20);
  tft.println(F("SELECT PRINTER"));
  tft.drawLine(0, 30, 320, 30, COLOR_LINE);

  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(20, 65);
  tft.println(F("1 - Dona Josefa"));
  tft.setCursor(20, 95);
  tft.println(F("2 - Makilas"));
  tft.setCursor(20, 125);
  tft.println(F("3 - Buluan (Default)"));
  tft.setCursor(20, 155);
  tft.println(F("4 - Caparan"));

  tft.drawLine(0, 180, 320, 180, COLOR_LINE);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(35, 210);
  tft.println(F("Press # to cancel"));
}

#endif // PRINTER_SELECT_SCREEN_H
