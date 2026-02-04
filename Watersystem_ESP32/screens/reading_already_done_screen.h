#ifndef READING_ALREADY_DONE_SCREEN_H
#define READING_ALREADY_DONE_SCREEN_H

#include "../configuration/config.h"
#include "../database/customers_database.h"

// ===== EXTERNAL OBJECTS =====
extern TFT_eSPI tft;
extern Customer* currentCustomer;

void displayReadingAlreadyDoneScreen() {
  tft.fillScreen(COLOR_BG);

  Customer* cust = currentCustomer;

  tft.setTextColor(COLOR_HEADER);
  tft.setTextSize(1);
  tft.setCursor(12, 10);
  tft.println(F("READING ALREADY DONE"));

  tft.drawLine(0, 22, 160, 22, COLOR_LINE);

  if (cust != nullptr) {
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(2, 30);
    tft.print(F("Acct: "));
    tft.println(cust->account_no);

    tft.setCursor(2, 42);
    tft.print(F("Name: "));
    tft.println(cust->customer_name);
  }

  tft.setTextColor(ST77XX_RED);
  tft.setCursor(10, 62);
  tft.println(F("This month already"));
  tft.setCursor(10, 74);
  tft.println(F("has a reading."));

  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 100);
  tft.println(F("D - Continue"));
  tft.setCursor(2, 112);
  tft.println(F("C - Cancel"));
}

#endif // READING_ALREADY_DONE_SCREEN_H