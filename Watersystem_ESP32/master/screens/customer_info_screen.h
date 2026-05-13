#ifndef CUSTOMER_INFO_SCREEN_H
#define CUSTOMER_INFO_SCREEN_H

#include "../configuration/config.h"
#include "../database/customers_database.h"
#include "../database/readings_database.h"
#include "../database/bill_database.h"

// ===== EXTERNAL OBJECTS =====
extern TFT_eSPI tft;
extern Customer* currentCustomer;
extern unsigned long correctPreviousReading;

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

static String getPeriodCoveredText() {
  String nowStr = getCurrentDateTimeString();
  if (nowStr.length() < 7) {
    return String("Period: N/A");
  }

  int year = nowStr.substring(0, 4).toInt();
  int month = nowStr.substring(5, 7).toInt();
  if (month < 1 || month > 12) {
    return String("Period: N/A");
  }

  int prevMonth = month - 1;
  int prevYear = year;
  if (prevMonth == 0) {
    prevMonth = 12;
    prevYear--;
  }

  return String("Period: ") + shortMonthName(prevMonth) + " " + String(prevYear)
       + " - " + shortMonthName(month) + " " + String(year);
}

static String truncateText(const String& s, int maxLen) {
  if ((int)s.length() <= maxLen) return s;
  if (maxLen <= 3) return s.substring(0, maxLen);
  return s.substring(0, maxLen - 3) + "...";
}

void displayCustomerInfo() {
  tft.fillScreen(COLOR_BG);
  
  Customer* cust = currentCustomer;
  if (cust == nullptr) return;

  // Payment-summary style header + borders
  tft.fillRect(0, 0, 320, 5, TFT_BLUE);
  tft.fillRect(0, 235, 320, 5, TFT_BLUE);

  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(95, 25);
  tft.println(F("CUSTOMER INFO"));
  tft.drawLine(0, 35, 320, 35, COLOR_LINE);

  // Compact layout (2.8" 320x240)
  const int leftX = 10;
  const int rightX = 180;
  const int lineH = 24;
  int y = 60;

  tft.setFreeFont(&FreeSans9pt7b);

  // Row 1: Account + Status
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(leftX, y);
  tft.print(F("Account: "));
  tft.setTextColor(COLOR_TEXT);
  tft.print(cust->account_no);

  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(rightX, y);
  tft.print(F("Status: "));
  if (String(cust->status) == "inactive") {
    tft.setTextColor(TFT_RED);
    tft.println(F("Inactive"));
  } else {
    tft.setTextColor(TFT_GREEN);
    tft.println(F("Active"));
  }

  // Row 2: Name
  y += lineH;
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(leftX, y);
  tft.print(F("Name: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(truncateText(cust->customer_name, 22));

  // Row 3: Address
  y += lineH;
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(leftX, y);
  tft.print(F("Address: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(truncateText(cust->address, 20));

  // Row 4: Period
  y += lineH;
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(leftX, y);
  tft.println(getPeriodCoveredText());

  // Divider
  int afterInfoY = y + 12;
  tft.drawLine(0, afterInfoY, 320, afterInfoY, COLOR_LINE);
  
  // Check if customer has existing reading and show correct previous reading
  unsigned long displayPrevReading = cust->previous_reading;
  unsigned long existingPrev = 0;
  unsigned long existingCurr = 0;
  unsigned long existingUsage = 0;
  bool hasReading = hasReadingThisMonth(cust->customer_id);
  bool hasPendingPrev = (!hasReading) && hasPendingBillForCustomerPrevMonth(cust->customer_id);
  if (hasReading) {
    if (getExistingReadingDataThisMonth(cust->customer_id, existingPrev, existingCurr, existingUsage)) {
      displayPrevReading = existingPrev;
    }
  }
  
  correctPreviousReading = displayPrevReading;

  // Reading block
  int readingY = afterInfoY + 20;
  if (hasReading) {
    tft.setTextColor(COLOR_LABEL);
    tft.setCursor(leftX, readingY);
    tft.print(F("Previous:"));
    tft.setTextColor(COLOR_AMOUNT);
    tft.print(displayPrevReading);
    tft.setTextColor(COLOR_LABEL);
    tft.print(F("  Present:"));
    tft.setTextColor(COLOR_AMOUNT);
    tft.print(existingCurr);
  } else {
    tft.setTextColor(COLOR_LABEL);
    tft.setCursor(leftX, readingY);
    tft.print(F("Previous Reading: "));
    tft.setTextColor(COLOR_AMOUNT);
    tft.println(displayPrevReading);
  }

  if (hasReading) {
    tft.setTextColor(TFT_RED);
    tft.setCursor(leftX, readingY + 20);
    tft.println(F("Reading already done this month"));
  } else if (hasPendingPrev) {
    tft.setTextColor(TFT_RED);
    tft.setCursor(leftX, readingY + 20);
    tft.println(F("Previous month bill pending"));
  }

  // Footer hints (aligned like payment summary)
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(8, 220);
  if (hasReading) {
    tft.println(F("D-Continue  A-Reprint  C-Cancel"));
  } else if (hasPendingPrev) {
    tft.println(F("C-Cancel"));
  } else {
    tft.println(F("D-Enter Reading   C-Cancel"));
  }
}

#endif // CUSTOMER_INFO_SCREEN_H