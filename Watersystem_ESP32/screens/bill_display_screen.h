#ifndef BILL_DISPLAY_SCREEN_H
#define BILL_DISPLAY_SCREEN_H

#include "../configuration/config.h"
#include "../database/bill_database.h"

// ===== EXTERNAL OBJECTS =====
extern TFT_eSPI tft;

void displayBillOnTFT() {
  tft.fillScreen(COLOR_BG);
  
  // Calculate used and total
  unsigned long used = currentBill.currReading - currentBill.prevReading;
  float total = used * currentBill.rate + currentBill.penalty;
  
  // Header
  tft.setTextColor(COLOR_HEADER);
  tft.setTextSize(1);
  tft.setCursor(35, 5);
  tft.println(F("WATER BILL"));
  
  tft.drawLine(0, 16, 160, 16, COLOR_LINE);
  
  // Customer info
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 22);
  tft.print(F("Name: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(currentBill.customerName);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 34);
  tft.print(F("Acct: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(currentBill.accountNo);
  
  tft.drawLine(0, 46, 160, 46, COLOR_LINE);
  
  // Meter readings
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 52);
  tft.print(F("Prev: "));
  tft.setTextColor(COLOR_TEXT);
  tft.print(currentBill.prevReading);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(85, 52);
  tft.print(F("Cur: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(currentBill.currReading);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 64);
  tft.print(F("Used: "));
  tft.setTextColor(COLOR_AMOUNT);
  tft.print(used);
  tft.println(F(" m3"));
  
  tft.drawLine(0, 76, 160, 76, COLOR_LINE);
  
  // Due date
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 82);
  tft.print(F("Due: "));
  tft.setTextColor(ST77XX_RED);
  tft.println(currentBill.dueDate);
  
  tft.drawLine(0, 94, 160, 94, COLOR_LINE);
  
  // Total amount - big and prominent
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(25, 100);
  tft.println(F("TOTAL DUE:"));
  
  tft.setTextColor(COLOR_AMOUNT);
  tft.setTextSize(2);
  tft.setCursor(20, 112);
  tft.print(F("P"));
  tft.println(total, 2);
  tft.setTextSize(1);
}

#endif // BILL_DISPLAY_SCREEN_H