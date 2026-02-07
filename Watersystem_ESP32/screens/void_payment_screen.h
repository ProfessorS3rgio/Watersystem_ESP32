#ifndef VOID_PAYMENT_SCREEN_H
#define VOID_PAYMENT_SCREEN_H

#include "../configuration/config.h"
#include "../database/customers_database.h"
#include "../database/bill_database.h"
#include "../database/bill_transactions_database.h"
#include <SD.h>
#include <RTClib.h>

// ===== EXTERNAL OBJECTS =====
extern TFT_eSPI tft;
extern Customer* currentCustomer;
extern BillTransaction currentTransaction;
extern RTC_DS3231 rtc;

// ===== FUNCTIONS FROM OTHER MODULES =====
bool getBillForCustomer(String accountNo);
bool isSDCardReady();
void deselectTftSelectSd();

#ifndef WS_SHORT_MONTH_NAME_DEFINED
#define WS_SHORT_MONTH_NAME_DEFINED
static const char* shortMonthName(int month) {
  static const char* months[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };
  if (month < 1 || month > 12) return "";
  return months[month - 1];
}
#endif

void displayVoidPaymentScreen() {
  tft.fillScreen(COLOR_BG);
  
  Customer* cust = currentCustomer;
  if (cust == nullptr) return;
  
  // Get existing bill for customer
  bool billRetrieved = getBillForCustomer(cust->account_no);
  if (!billRetrieved) {
    // Handle error - no bill found
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(1);
    tft.setCursor(20, 50);
    tft.println(F("No outstanding bill found!"));
    tft.setCursor(20, 70);
    tft.println(F("Cannot void payment."));
    return;
  }
  
  // Get latest transaction for the bill
  bool transRetrieved = getLatestTransactionForBill(currentBill.refNumber, currentTransaction);
  if (!transRetrieved) {
    // No transaction found
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(1);
    tft.setCursor(20, 50);
    tft.println(F("No payment found for this bill!"));
    tft.setCursor(20, 70);
    tft.println(F("Cannot void payment."));
    return;
  }
  
  DateTime now = rtc.now();
  
  // Welcome-screen style fonts + simple borders
  tft.fillRect(0, 0, 320, 5, TFT_BLUE);
  tft.fillRect(0, 235, 320, 5, TFT_BLUE);

  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(75, 25);
  tft.println(F("VOID PAYMENT"));
  tft.drawLine(0, 35, 320, 35, COLOR_LINE);

  // Compact layout (2.8" 320x240)
  const int leftX = 10;
  const int rightX = 180;
  const int lineH = 24;
  int y = 60;

  tft.setFreeFont(&FreeSans9pt7b);

  // Line 1: Account + Amount Received
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(leftX, y);
  tft.print(F("Account: "));
  tft.setTextColor(COLOR_TEXT);
  tft.print(currentBill.accountNo);

  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(rightX, y);
  tft.print(F("Received: P"));
  tft.setTextColor(COLOR_AMOUNT);
  tft.print(currentTransaction.cash_received, 2);

  // Line 2: Name + Change
  y += lineH;
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(leftX, y);
  tft.print(F("Name: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(currentBill.customerName);

  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(rightX, y);
  tft.print(F("Change: P"));
  tft.setTextColor(COLOR_AMOUNT);
  tft.print(currentTransaction.change, 2);

  // Line 3: Type + Date
  y += lineH;
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(leftX, y);
  tft.print(F("Type: "));
  tft.setTextColor(COLOR_TEXT);
  String typeDisplay = currentBill.customerType;
  typeDisplay.toLowerCase();
  if (typeDisplay == "communal faucet") {
    tft.print(F("Communal"));
  } else {
    tft.print(currentBill.customerType);
  }

  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(rightX, y);
  tft.print(F("Date: "));
  tft.setTextColor(COLOR_TEXT);
  // Parse transaction_date, assume YYYY-MM-DD HH:MM:SS
  String dateStr = currentTransaction.transaction_date;
  if (dateStr.length() >= 10) {
    tft.print(dateStr.substring(5,7)); // MM
    tft.print("/");
    tft.print(dateStr.substring(8,10)); // DD
    tft.print("/");
    tft.print(dateStr.substring(2,4)); // YY
  }

  // Line 4: Billing Period + Usage
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

  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(rightX, y);
  tft.print(F("Usage: "));
  tft.setTextColor(COLOR_AMOUNT);
  tft.print(currentBill.usage);
  tft.setTextColor(COLOR_LABEL);
  tft.print(F("m3"));

  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(70, 220);
  tft.println(F("Press D to Void   C to Cancel"));
}

#endif // VOID_PAYMENT_SCREEN_H