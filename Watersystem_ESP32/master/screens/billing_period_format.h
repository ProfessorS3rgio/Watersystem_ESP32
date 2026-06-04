#ifndef BILLING_PERIOD_FORMAT_H
#define BILLING_PERIOD_FORMAT_H

#include <Arduino.h>
#include <RTClib.h>

static inline String formatBillingPeriod(const String& billDate, const String& readingDateTime, const DateTime& fallbackNow) {
  String dateStr = "";
  if (billDate.length() >= 10) {
    dateStr = billDate.substring(0, 10);
  } else if (readingDateTime.length() >= 10) {
    dateStr = readingDateTime.substring(0, 10);
  }

  int year = 0;
  int month = 0;
  int day = 0;
  if (dateStr.length() >= 10) {
    year = dateStr.substring(0, 4).toInt();
    month = dateStr.substring(5, 7).toInt();
    day = dateStr.substring(8, 10).toInt();
  } else {
    year = fallbackNow.year();
    month = fallbackNow.month();
    day = fallbackNow.day();
  }

  if (month < 1 || month > 12) {
    return String("N/A");
  }

  int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) {
    daysInMonth[1] = 29;
  }

  if (day < 1 || day > daysInMonth[month - 1]) {
    return String("N/A");
  }

  static const char* months[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };

  const char* monthStr = months[month - 1];
  return String(monthStr) + " 1, " + String(year) + " - " + monthStr + " " + String(day) + ", " + String(year);
}

#endif  // BILLING_PERIOD_FORMAT_H
