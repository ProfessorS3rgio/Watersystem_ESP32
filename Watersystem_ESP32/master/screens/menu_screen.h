#ifndef MENU_SCREEN_H
#define MENU_SCREEN_H

#include "../configuration/config.h"

// ===== EXTERNAL OBJECTS =====
extern TFT_eSPI tft;

void displayMenuScreen() {
  tft.fillScreen(COLOR_BG);
  
  // ===== TOP BORDER =====
  // tft.fillRect(0, 0, 320, 5, TFT_BLUE);
  
  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(130, 20);
  tft.println(F("MENU"));
  
  tft.drawLine(0, 30, 320, 30, COLOR_LINE);
  
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(20, 50);
  tft.println(F("1. About"));
  tft.setCursor(20, 75);
  tft.println(F("2. Update"));
  tft.setCursor(20, 100);
  tft.println(F("3. Void payments"));
  tft.setCursor(20, 125);
  tft.println(F("4. Restart"));
  tft.setCursor(20, 150);
  tft.println(F("5. Printer Test"));
  tft.setCursor(20, 175);
  tft.println(F("6. Gen Test Data"));
  
  tft.drawLine(0, 200, 320, 200, COLOR_LINE);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(67, 223);
  tft.println(F("Press # or C to cancel"));
  
  // ===== BOTTOM BORDER =====
  // tft.fillRect(0, 235, 320, 5, TFT_BLUE);
}

#endif // MENU_SCREEN_H