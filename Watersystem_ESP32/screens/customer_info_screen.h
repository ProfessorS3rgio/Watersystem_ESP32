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
  
  tft.setTextColor(COLOR_HEADER);
  tft.setTextSize(1);
  tft.setCursor(20, 5);
  tft.println(F("CUSTOMER INFO"));
  
  tft.drawLine(0, 16, 160, 16, COLOR_LINE);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 22);
  tft.print(F("Acct: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(cust->account_no);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 34);
  tft.print(F("Name: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(cust->customer_name);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 46);
  tft.print(F("Addr: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(cust->address);
  
  tft.drawLine(0, 58, 160, 58, COLOR_LINE);
  
  // Check if customer has existing reading and show correct previous reading
  unsigned long displayPrevReading = cust->previous_reading;
  if (hasReadingThisMonth(cust->customer_id)) {
    unsigned long existingPrev = 0, existingCurr = 0, existingUsage = 0;
    if (getExistingReadingDataThisMonth(cust->customer_id, existingPrev, existingCurr, existingUsage)) {
      displayPrevReading = existingPrev;
    }
  }
  
  correctPreviousReading = displayPrevReading;
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 64);
  tft.print(F("Prev: "));
  tft.setTextColor(COLOR_AMOUNT);
  tft.println(displayPrevReading);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 100);
  tft.println(F("Press D for reading"));
  tft.setCursor(2, 112);
  tft.println(F("Press C to cancel"));
}

#endif // CUSTOMER_INFO_SCREEN_H