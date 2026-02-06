#ifndef PAYMENT_SUMMARY_SCREEN_H
#define PAYMENT_SUMMARY_SCREEN_H

#include "../configuration/config.h"
#include "../database/customers_database.h"
#include "../database/bill_database.h"
#include <SD.h>
#include <RTClib.h>

// ===== EXTERNAL OBJECTS =====
extern TFT_eSPI tft;
extern Customer* currentCustomer;
extern unsigned long currentReading;
extern float paymentAmount;  // Assume this is set externally
extern RTC_DS3231 rtc;

// ===== FUNCTIONS FROM OTHER MODULES =====
bool generateBillForCustomer(String accountNo, unsigned long currentReading);
bool getBillForCustomer(String accountNo);
bool isSDCardReady();
void deselectTftSelectSd();

static const char* shortMonthName(int month) {
  static const char* months[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };
  if (month < 1 || month > 12) return "";
  return months[month - 1];
}

void displayPaymentSummary() {
  tft.fillScreen(COLOR_BG);
  
  Customer* cust = currentCustomer;
  if (cust == nullptr) return;
  
  // Get existing bill for customer
  bool billRetrieved = getBillForCustomer(cust->account_no);
  Serial.print(F("Bill retrieved: "));
  Serial.println(billRetrieved ? F("Success") : F("Failed"));
  if (!billRetrieved) {
    // Handle error - no bill found
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(1);
    tft.setCursor(20, 50);
    tft.println(F("No outstanding bill found!"));
    tft.setCursor(20, 70);
    tft.println(F("Cannot process payment."));
    return;
  }
  
  DateTime now = rtc.now();
  
  Serial.print(F("Customer: "));
  Serial.println(currentBill.customerName);
  Serial.print(F("Usage: "));
  Serial.println(currentBill.usage);
  Serial.print(F("Total: P"));
  Serial.println(currentBill.total, 2);

  // Welcome-screen style fonts + simple borders
  tft.fillRect(0, 0, 320, 5, TFT_BLUE);
  tft.fillRect(0, 235, 320, 5, TFT_BLUE);

  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(85, 25);
  tft.println(F("PAYMENT SUMMARY"));
  tft.drawLine(0, 35, 320, 35, COLOR_LINE);

  // Compact layout (2.8" 320x240)
  const int leftX = 10;
  const int rightX = 180;
  const int lineH = 24;
  int y = 60;

  tft.setFreeFont(&FreeSans9pt7b);

  // Line 1: Account + Used
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(leftX, y);
  tft.print(F("Account: "));
  tft.setTextColor(COLOR_TEXT);
  tft.print(currentBill.accountNo);

  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(rightX, y);
  tft.print(F("Used: "));
  tft.setTextColor(COLOR_AMOUNT);
  tft.print(currentBill.usage);
  tft.setTextColor(COLOR_LABEL);
  tft.print(F("m3"));

  // Line 2: Name
  y += lineH;
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(leftX, y);
  tft.print(F("Name: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(currentBill.customerName);

  // Line 3: Type + Rate (or Min Charge)
  y += lineH;
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(leftX, y);
  tft.print(F("Type: "));
  tft.setTextColor(COLOR_TEXT);
  tft.print(currentBill.customerType);

  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(rightX, y);
  if (currentBill.usage <= currentBill.minM3 && currentBill.minM3 > 0) {
    tft.print(F("MinChg: P"));
    tft.setTextColor(COLOR_TEXT);
    tft.println(currentBill.minCharge, 2);
  } else {
    tft.print(F("Rate: P"));
    tft.setTextColor(COLOR_TEXT);
    tft.print(currentBill.rate, 2);
    tft.setTextColor(COLOR_LABEL);
    tft.println(F("/m3"));
  }

  // Line 4: Billing Period
  int startMonth = now.month() - 1;
  if (startMonth <= 0) startMonth = 12;

  y += lineH;
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(leftX, y);
  tft.print(F("Billing Period: "));
  tft.setTextColor(COLOR_TEXT);
  tft.print(shortMonthName(startMonth));
  tft.print(F(" - "));
  tft.print(shortMonthName(now.month()));
  tft.print(F(" "));
  tft.println(now.year());

  // Optional discount line
  int afterRowsY = y + lineH;
  if (currentBill.deductions > 0.0f) {
    tft.setTextColor(COLOR_LABEL);
    tft.setCursor(leftX, afterRowsY);
    tft.print(F("Discount: P"));
    tft.setTextColor(COLOR_TEXT);
    tft.println(currentBill.deductions, 2);
    afterRowsY += lineH;
  }

  // Overall due (large + centered)
  tft.drawLine(0, afterRowsY, 320, afterRowsY, COLOR_LINE);

  // Smaller "Overall Due" block to fit better on 2.8" screen
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(10, afterRowsY + 22);
  tft.println(F("Overall Due:"));

  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_AMOUNT);
  tft.setCursor(10, afterRowsY + 45);
  tft.print(F("P"));
  tft.println(currentBill.total, 2);

  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(70, 220);
  tft.println(F("Press D to Pay   C to Cancel"));
}

#endif // PAYMENT_SUMMARY_SCREEN_H