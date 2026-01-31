#ifndef MENU_SCREEN_H
#define MENU_SCREEN_H

#include "../configuration/config.h"

// ===== EXTERNAL OBJECTS =====
extern TFT_eSPI tft;

void displayMenuScreen() {
  tft.fillScreen(COLOR_BG);
  
  tft.setTextColor(COLOR_HEADER);
  tft.setTextSize(1);
  tft.setCursor(55, 5);
  tft.println(F("MENU"));
  
  tft.drawLine(0, 16, 160, 16, COLOR_LINE);
  
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(10, 25);
  tft.println(F("1. About"));
  tft.setCursor(10, 40);
  tft.println(F("2. Update"));
  tft.setCursor(10, 55);
  tft.println(F("3. Bill Rate"));
  tft.setCursor(10, 70);
  tft.println(F("4. Restart"));
  tft.setCursor(10, 85);
  tft.println(F("5. Printer Test"));
  tft.setCursor(10, 100);
  tft.println(F("6. Gen Test Data"));
  
  tft.drawLine(0, 115, 160, 115, COLOR_LINE);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setTextSize(1);
  tft.setCursor(20, 123);
  tft.println(F("Press # or press"));
  tft.setCursor(32, 133);
  tft.println(F("C to cancel"));
}

#endif // MENU_SCREEN_H