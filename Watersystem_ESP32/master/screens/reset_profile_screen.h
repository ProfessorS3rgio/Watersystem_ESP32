#ifndef RESET_PROFILE_SCREEN_H
#define RESET_PROFILE_SCREEN_H

#include "../configuration/config.h"

extern TFT_eSPI tft;
extern String inputBuffer;

void displayResetProfilePinScreen() {
  tft.fillScreen(COLOR_BG);

  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(45, 20);
  tft.println(F("RESET DEVICE"));
  tft.drawLine(0, 30, 320, 30, COLOR_LINE);

  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(55, 65);
  tft.println(F("Enter security PIN"));

  tft.drawRect(75, 80, 170, 55, COLOR_LINE);
  tft.setFreeFont(&FreeSans18pt7b);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(115, 119);
  for (unsigned int i = 0; i < inputBuffer.length(); ++i) {
    tft.print('*');
  }

  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(35, 175);
  tft.println(F("D-Confirm  B-Clear  C-Cancel"));
}

#endif // RESET_PROFILE_SCREEN_H
