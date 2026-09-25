#ifndef SET_DATETIME_SCREEN_H
#define SET_DATETIME_SCREEN_H

#include "../configuration/config.h"

extern TFT_eSPI tft;
extern String inputBuffer;

void displaySetDateTimeScreen() {
  tft.fillScreen(COLOR_BG);

  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(65, 20);
  tft.println(F("SET DATE/TIME"));
  tft.drawLine(0, 30, 320, 30, COLOR_LINE);

  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(15, 55);
  tft.println(F("Enter: YYYYMMDDHHMMSS"));

  tft.drawRect(15, 70, 290, 60, COLOR_LINE);
  tft.setFreeFont(&FreeSans12pt7b);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(25, 108);

  if (inputBuffer.length() > 0) {
    String formatted;
    for (unsigned int i = 0; i < inputBuffer.length(); ++i) {
      if (i == 4 || i == 6) formatted += '-';
      if (i == 8) formatted += ' ';
      if (i == 10 || i == 12) formatted += ':';
      formatted += inputBuffer[i];
    }
    tft.println(formatted);
  } else {
    tft.setTextColor(COLOR_LABEL);
    tft.println(F("____-__-__ __:__:__"));
  }

  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(30, 170);
  tft.println(F("D-Save  B-Clear  C-Cancel"));
}

#endif // SET_DATETIME_SCREEN_H