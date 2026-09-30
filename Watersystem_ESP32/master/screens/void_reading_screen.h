#ifndef VOID_READING_SCREEN_H
#define VOID_READING_SCREEN_H

#include "../configuration/config.h"
#include "../database/customers_database.h"
#include "../database/bill_database.h"

extern TFT_eSPI tft;
extern Customer* currentCustomer;

void displayVoidReadingScreen() {
  tft.fillScreen(COLOR_BG);

  if (currentCustomer == nullptr) return;

  unsigned long previousReading = 0;
  unsigned long currentReadingValue = 0;
  unsigned long usage = 0;
  String createdAt;
  String billStatus;
  if (!getLatestReadingForCustomer(currentCustomer->customer_id,
                                   previousReading,
                                   currentReadingValue,
                                   usage,
                                   createdAt,
                                   billStatus)) {
    tft.setTextColor(TFT_RED);
    tft.setCursor(25, 70);
    tft.println(F("No reading found"));
    tft.setTextColor(COLOR_LABEL);
    tft.setCursor(35, 120);
    tft.println(F("Press C to cancel"));
    return;
  }

  tft.fillRect(0, 0, 320, 5, TFT_BLUE);
  tft.fillRect(0, 235, 320, 5, TFT_BLUE);
  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(75, 25);
  tft.println(F("VOID READING"));
  tft.drawLine(0, 35, 320, 35, COLOR_LINE);

  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(10, 60);
  tft.print(F("Account: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(currentCustomer->account_no);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(10, 83);
  tft.print(F("Name: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(currentCustomer->customer_name);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(10, 112);
  tft.print(F("Previous: "));
  tft.setTextColor(COLOR_AMOUNT);
  tft.println(previousReading);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(10, 135);
  tft.print(F("Latest: "));
  tft.setTextColor(COLOR_AMOUNT);
  tft.println(currentReadingValue);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(10, 158);
  tft.print(F("Usage: "));
  tft.setTextColor(COLOR_AMOUNT);
  tft.print(usage);
  tft.println(F(" m3"));

  tft.setTextColor(billStatus == "Paid" ? TFT_RED : COLOR_LABEL);
  tft.setCursor(10, 181);
  tft.print(F("Bill: "));
  tft.println(billStatus.length() > 0 ? billStatus : F("None"));

  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(45, 220);
  tft.println(F("D-Void Reading  C-Cancel"));
}

#endif // VOID_READING_SCREEN_H
