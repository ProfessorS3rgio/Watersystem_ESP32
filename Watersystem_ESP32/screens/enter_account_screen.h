#ifndef ENTER_ACCOUNT_SCREEN_H
#define ENTER_ACCOUNT_SCREEN_H

#include "../configuration/config.h"

// ===== EXTERNAL OBJECTS =====
extern TFT_eSPI tft;
extern String inputBuffer;

void displayEnterAccountScreen() {
  tft.fillScreen(COLOR_BG);
  
  tft.setTextColor(COLOR_HEADER);
  tft.setTextSize(1);
  tft.setCursor(20, 10);
  tft.println(F("ENTER ACCOUNT #"));
  
  tft.drawLine(0, 20, 160, 20, COLOR_LINE);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(5, 35);
  tft.println(F("Account Number:"));
  
  // Input box
  tft.drawRect(10, 50, 140, 30, COLOR_LINE);
  
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setCursor(20, 58);
  if (inputBuffer.length() > 0) {
    tft.println(inputBuffer);
  } else {
    // Placeholder
    tft.setTextColor(COLOR_LABEL);
    tft.println(F("___"));
  }
  
  tft.setTextSize(1);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(10, 100);
  tft.println(F("D-Confirm B-Clear"));
  tft.setCursor(32, 112);
  tft.println(F("C-Cancel"));
}

#endif // ENTER_ACCOUNT_SCREEN_H