#ifndef ENTER_ACCOUNT_SCREEN_H
#define ENTER_ACCOUNT_SCREEN_H

#include "../configuration/config.h"

// ===== EXTERNAL OBJECTS =====
extern TFT_eSPI tft;
extern String inputBuffer;

void displayEnterAccountScreen() {
  tft.fillScreen(COLOR_BG);
  
  
  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(80, 20);
  tft.println(F("ENTER ACCOUNT #"));
  
  tft.drawLine(0, 30, 320, 30, COLOR_LINE);
  
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(20, 50);
  tft.println(F("Account Number:"));
  
  // Input box
  tft.drawRect(20, 60, 280, 60, COLOR_LINE);
  
  tft.setFreeFont(&FreeSans18pt7b);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(55, 100);
  if (inputBuffer.length() > 0) {
    tft.println(inputBuffer);
  } else {
    // Placeholder
    tft.setTextColor(COLOR_LABEL);
    tft.println(F("___________"));
  }
  
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(35, 180);
  tft.println(F("D-Confirm  B-Clear  C-Cancel"));
  

}

#endif // ENTER_ACCOUNT_SCREEN_H