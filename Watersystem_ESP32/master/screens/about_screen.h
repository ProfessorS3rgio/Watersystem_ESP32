#ifndef ABOUT_SCREEN_H
#define ABOUT_SCREEN_H

#include "../configuration/config.h"
#include "../configuration/device_profile.h"

extern TFT_eSPI tft;

void displayAboutScreen() {
  tft.fillScreen(COLOR_BG);

  tft.fillRect(0, 0, 320, 5, COLOR_LINE);
  tft.fillRect(0, 230, 320, 5, COLOR_LINE);

  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(126, 25);
  tft.println(F("ABOUT"));
  tft.drawLine(20, 35, 300, 35, COLOR_LINE);

  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(78, 65);
  tft.println(F("WATER SYSTEM"));

  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(105, 88);
  tft.println(F("Water Billing v1.0"));

  tft.drawLine(55, 102, 265, 102, COLOR_LINE);

  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(20, 125);
  tft.print(F("Collector: "));
  tft.println(g_collectorName);
  tft.setCursor(20, 148);
  tft.print(F("Barangay ID: "));
  tft.println(g_deviceId);
  tft.setCursor(20, 171);
  tft.println(F("Zamboanga Sibugay"));

  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(78, 210);
  tft.println(F("C - Back to Menu"));
}

#endif // ABOUT_SCREEN_H
