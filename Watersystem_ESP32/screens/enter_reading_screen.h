#ifndef ENTER_READING_SCREEN_H
#define ENTER_READING_SCREEN_H

#include "../configuration/config.h"
#include "../database/customers_database.h"

// ===== EXTERNAL OBJECTS =====
extern TFT_eSPI tft;
extern Customer* currentCustomer;
extern String inputBuffer;

void displayEnterReadingScreen() {
  tft.fillScreen(COLOR_BG);
  
  Customer* cust = currentCustomer;
  if (cust == nullptr) return;

  // Match Enter Account screen styling
  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(85, 20);
  tft.println(F("CURRENT READING"));

  tft.drawLine(0, 30, 320, 30, COLOR_LINE);

  // Customer name
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(20, 50);
  tft.print(F("Name: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(cust->customer_name);

  // Prompt
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(20, 75);
  tft.println(F("Current Reading:"));

  // Input box (same style as account screen)
  tft.drawRect(20, 85, 280, 60, COLOR_LINE);

  tft.setFreeFont(&FreeSans18pt7b);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(55, 125);
  if (inputBuffer.length() > 0) {
    tft.println(inputBuffer);
  } else {
    tft.setTextColor(COLOR_LABEL);
    tft.println(F("_____"));
  }

  // Footer controls
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(35, 180);
  tft.println(F("D-Calculate  B-Clear  C-Cancel"));
}

#endif // ENTER_READING_SCREEN_H