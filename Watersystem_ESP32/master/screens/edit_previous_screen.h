#ifndef EDIT_PREVIOUS_SCREEN_H
#define EDIT_PREVIOUS_SCREEN_H

#include "../configuration/config.h"
#include "../database/customers_database.h"

// ===== EXTERNAL OBJECTS =====
extern TFT_eSPI tft;
extern Customer* currentCustomer;
extern String inputBuffer;

void displayEditPreviousScreen() {
  tft.fillScreen(COLOR_BG);

  Customer* cust = currentCustomer;
  if (cust == nullptr) return;

  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(78, 20);
  tft.println(F("EDIT PREVIOUS"));

  tft.drawLine(0, 30, 320, 30, COLOR_LINE);

  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(20, 50);
  tft.print(F("Account: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(cust->account_no);

  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(20, 75);
  tft.print(F("Name: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(cust->customer_name);

  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(20, 100);
  tft.print(F("Current Previous: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(cust->previous_reading);

  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(20, 125);
  tft.println(F("New Previous Reading:"));

  tft.drawRect(20, 135, 280, 50, COLOR_LINE);

  tft.setFreeFont(&FreeSans18pt7b);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(55, 170);
  if (inputBuffer.length() > 0) {
    tft.println(inputBuffer);
  } else {
    tft.setTextColor(COLOR_LABEL);
    tft.println(F("_____"));
  }

  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(24, 220);
  tft.println(F("D-Save  B-Clear  C-Cancel"));
}

#endif  // EDIT_PREVIOUS_SCREEN_H