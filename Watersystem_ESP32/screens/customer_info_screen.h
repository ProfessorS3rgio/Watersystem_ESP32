#ifndef CUSTOMER_INFO_SCREEN_H
#define CUSTOMER_INFO_SCREEN_H

#include "../configuration/config.h"
#include "../database/customers_database.h"
#include "../database/readings_database.h"

// ===== EXTERNAL OBJECTS =====
extern TFT_eSPI tft;
extern Customer* currentCustomer;
extern unsigned long correctPreviousReading;

void displayCustomerInfo() {
  tft.fillScreen(COLOR_BG);
  
  Customer* cust = currentCustomer;
  if (cust == nullptr) return;
  
  tft.setTextFont(4);  // Sans-serif font for header
  tft.setTextSize(1);
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(80, 20);
  tft.println(F("CUSTOMER INFO"));
  
  tft.drawLine(0, 30, 320, 30, COLOR_LINE);
  
  tft.setTextFont(2);  // Sans-serif font for labels
  tft.setTextSize(1);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(20, 50);
  tft.print(F("Account: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(cust->account_no);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(20, 70);
  tft.print(F("Name: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(cust->customer_name);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(20, 90);
  tft.print(F("Address: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(cust->address);
  
  tft.drawLine(0, 110, 320, 110, COLOR_LINE);
  
  // Check if customer has existing reading and show correct previous reading
  unsigned long displayPrevReading = cust->previous_reading;
  bool hasReading = hasReadingThisMonth(cust->customer_id);
  if (hasReading) {
    unsigned long existingPrev = 0, existingCurr = 0, existingUsage = 0;
    if (getExistingReadingDataThisMonth(cust->customer_id, existingPrev, existingCurr, existingUsage)) {
      displayPrevReading = existingPrev;
    }
  }
  
  correctPreviousReading = displayPrevReading;
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(20, 130);
  tft.print(F("Previous Reading: "));
  tft.setTextColor(COLOR_AMOUNT);
  tft.println(displayPrevReading);
  
  if (hasReading) {
    tft.setTextColor(TFT_RED);
    tft.setCursor(20, 150);
    tft.println(F("Reading already done this month"));
    tft.setTextColor(COLOR_LABEL);
    tft.setCursor(35, 180);
    tft.println(F("D-Continue  C-Cancel"));
  } else {
    tft.setTextColor(COLOR_LABEL);
    tft.setCursor(35, 180);
    tft.println(F("D-Reading  C-Cancel"));
  }
}

#endif // CUSTOMER_INFO_SCREEN_H