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
  
  tft.setTextColor(COLOR_HEADER);
  tft.setTextSize(1);
  tft.setCursor(15, 10);
  tft.println(F("CURRENT READING"));
  
  tft.drawLine(0, 20, 160, 20, COLOR_LINE);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 28);
  tft.print(F("Name: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(cust->customer_name);
  
  tft.drawLine(0, 40, 160, 40, COLOR_LINE);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 48);
  tft.println(F("Enter current reading:"));

  // Input box
  tft.drawRect(10, 58, 140, 30, COLOR_LINE);

  tft.setTextColor(COLOR_AMOUNT);
  tft.setTextSize(2);
  tft.setCursor(20, 66);
  if (inputBuffer.length() > 0) {
    tft.println(inputBuffer);
  } else {
    // Placeholder
    tft.setTextColor(COLOR_LABEL);
    tft.println(F("___"));
  }
  tft.setTextSize(1);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(20, 95);
  tft.println(F("D - Calculate"));
  tft.setCursor(32, 107);
  tft.println(F("B - Clear"));
  tft.setCursor(28, 119);
  tft.println(F("C - Cancel"));
}

#endif // ENTER_READING_SCREEN_H