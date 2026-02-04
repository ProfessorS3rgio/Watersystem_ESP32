#ifndef PAYMENT_SUMMARY_SCREEN_H
#define PAYMENT_SUMMARY_SCREEN_H

#include "../configuration/config.h"
#include "../database/customers_database.h"
#include "../database/bill_database.h"
#include <SD.h>

// ===== EXTERNAL OBJECTS =====
extern TFT_eSPI tft;
extern Customer* currentCustomer;
extern unsigned long currentReading;
extern float paymentAmount;  // Assume this is set externally

// ===== FUNCTIONS FROM OTHER MODULES =====
bool generateBillForCustomer(String accountNo, unsigned long currentReading);
bool isSDCardReady();
void deselectTftSelectSd();

void displayPaymentSummary() {
  tft.fillScreen(COLOR_BG);
  
  Customer* cust = currentCustomer;
  if (cust == nullptr) return;
  
  // Generate bill using customer type logic
  bool billGenerated = generateBillForCustomer(cust->account_no, currentReading);
  Serial.print(F("Bill generated: "));
  Serial.println(billGenerated ? F("Success") : F("Failed"));
  if (!billGenerated) {
    // Handle error - customer not found or invalid
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(1);
    tft.setCursor(20, 50);
    tft.println(F("Bill generation failed!"));
    return;
  }
  
  // Calculate change
  float change = paymentAmount - currentBill.total;
  
  Serial.print(F("Customer: "));
  Serial.println(currentBill.customerName);
  Serial.print(F("Usage: "));
  Serial.println(currentBill.usage);
  Serial.print(F("Total: P"));
  Serial.println(currentBill.total, 2);
  Serial.print(F("Payment: P"));
  Serial.println(paymentAmount, 2);
  Serial.print(F("Change: P"));
  Serial.println(change, 2);
  
  tft.setTextColor(COLOR_HEADER);
  tft.setTextSize(1);
  tft.setCursor(35, 5);
  tft.println(F("PAYMENT SUMMARY"));
  
  tft.drawLine(0, 16, 160, 16, COLOR_LINE);
  
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(2, 22);
  tft.println(currentBill.customerName);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 34);
  tft.print(F("Used: "));
  tft.setTextColor(COLOR_AMOUNT);
  tft.print(currentBill.usage);
  tft.println(F(" m3"));
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 46);
  tft.print(F("Type: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(currentBill.customerType);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 58);
  if (currentBill.usage <= currentBill.minM3 && currentBill.minM3 > 0) {
    tft.print(F("Min Charge: P"));
    tft.println(currentBill.minCharge, 2);
  } else {
    tft.print(F("Rate: P"));
    tft.print(currentBill.rate, 2);
    tft.println(F("/m3"));
  }
  
  // Show deduction if any
  if (currentBill.deductions > 0) {
    tft.setTextColor(COLOR_LABEL);
    tft.setCursor(2, 70);
    tft.print(F("Discount: P"));
    tft.println(currentBill.deductions, 2);
    tft.drawLine(0, 82, 160, 82, COLOR_LINE);
    
    tft.setTextColor(COLOR_HEADER);
    tft.setCursor(15, 89);
    tft.println(F("TOTAL DUE:"));
    
    tft.setTextColor(COLOR_AMOUNT);
    tft.setTextSize(2);
    tft.setCursor(20, 102);
    tft.print(F("P"));
    tft.println(currentBill.total, 2);
    tft.setTextSize(1);
    
    tft.setTextColor(COLOR_LABEL);
    tft.setCursor(2, 120);
    tft.print(F("Payment: P"));
    tft.println(paymentAmount, 2);
    
    tft.setCursor(2, 132);
    tft.print(F("Change: P"));
    tft.println(change, 2);
    
    tft.setCursor(20, 150);
    tft.println(F("Press D to confirm"));
  } else {
    tft.drawLine(0, 70, 160, 70, COLOR_LINE);
    
    tft.setTextColor(COLOR_HEADER);
    tft.setCursor(15, 77);
    tft.println(F("TOTAL DUE:"));
    
    tft.setTextColor(COLOR_AMOUNT);
    tft.setTextSize(2);
    tft.setCursor(20, 90);
    tft.print(F("P"));
    tft.println(currentBill.total, 2);
    tft.setTextSize(1);
    
    tft.setTextColor(COLOR_LABEL);
    tft.setCursor(2, 108);
    tft.print(F("Payment: P"));
    tft.println(paymentAmount, 2);
    
    tft.setCursor(2, 120);
    tft.print(F("Change: P"));
    tft.println(change, 2);
    
    tft.setCursor(20, 140);
    tft.println(F("Press D to confirm"));
  }
}

#endif // PAYMENT_SUMMARY_SCREEN_H