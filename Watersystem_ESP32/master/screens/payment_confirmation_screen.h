#ifndef PAYMENT_CONFIRMATION_SCREEN_H
#define PAYMENT_CONFIRMATION_SCREEN_H

#include "../configuration/config.h"
#include "../database/customers_database.h"
#include "../database/bill_database.h"
#include "../database/bill_transactions_database.h"
#include <SD.h>

// ===== EXTERNAL OBJECTS =====
extern TFT_eSPI tft;
extern Customer* currentCustomer;
extern unsigned long currentReading;
extern float paymentAmount;  // Assume this is set externally

// ===== FUNCTIONS FROM OTHER MODULES =====
bool isSDCardReady();
void deselectTftSelectSd();

void displayPaymentConfirmation() {
  tft.fillScreen(COLOR_BG);
  
  Customer* cust = currentCustomer;
  if (cust == nullptr) return;
  
  // Calculate change
  float change = paymentAmount - currentBill.total;
  if (change < 0.0f) change = 0.0f;
  
  Serial.print(F("Payment: P"));
  Serial.println(paymentAmount, 2);
  Serial.print(F("Change: P"));
  Serial.println(change, 2);
  
  tft.setTextColor(COLOR_HEADER);
  tft.setTextSize(1);
  tft.setCursor(25, 20);
  tft.println(F("PAYMENT CONFIRMED"));
  
  tft.drawLine(0, 35, 320, 35, COLOR_LINE);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(20, 60);
  tft.print(F("Cash Received: P"));
  tft.setTextColor(COLOR_AMOUNT);
  tft.println(paymentAmount, 2);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(20, 90);
  tft.print(F("Total Due: P"));
  tft.setTextColor(COLOR_TEXT);
  tft.println(currentBill.total, 2);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(20, 120);
  tft.print(F("Change: P"));
  tft.setTextColor(COLOR_AMOUNT);
  tft.println(change, 2);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(50, 180);
  tft.println(F("Press D to Print Receipt"));
}

#endif // PAYMENT_CONFIRMATION_SCREEN_H