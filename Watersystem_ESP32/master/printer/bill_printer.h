#ifndef BILL_PRINTER_H
#define BILL_PRINTER_H

#include "printer/printer_serial.h"
#include "../database/bill_database.h"  // For BillData currentBill
#include "../database/device_info.h"   // For getDeviceInfoValue
#include "../configuration/config.h"    // For YIELD_WDT
#include "../configuration/logo.h"      // For logo bitmap

// External printer object (defined in main .ino)
extern ThermalPrinter printer;

void printBill();
String getPeriodCovered();
void customFeed(int lines);
String centerString(String s, int width);
String formatDateTime12Hour(String dateTimeStr);
String calculateDueDate(String billDate, int daysToAdd);

static String fitAddressForPrint(const String& address, size_t maxChars = 21) {
  if (address.length() <= maxChars) {
    return address;
  }
  return address.substring(0, maxChars);
}

void printBill() {
  printer.wake();
  printer.setDefault();
  YIELD_WDT();

  unsigned long used = currentBill.currReading - currentBill.prevReading;
  float total = currentBill.total;

//   customFeed(1);
  YIELD_WDT();
  printer.printBitmap(LOGO_WIDTH, LOGO_HEIGHT, logo);
  YIELD_WDT();  // 🚨 REQUIRED

  // blank line after logo
  printer.println();
  printer.justify('C');
  // try medium height for the headline
  printer.setSize('S');
  printer.boldOn();
  printer.println(F("STATEMENT OF ACCOUNT"));
  printer.boldOff();
  printer.setSize('S');   // restore normal for following lines
  printer.justify('L');
  printer.println();


  YIELD_WDT();

  printer.justify('L');
  // Ref No and Date/Time
  printer.print(F("Ref No      : "));
  printer.println(currentBill.refNumber);
  printer.print(F("Date & Time : "));
  printer.println(formatDateTime12Hour(currentBill.readingDateTime));
  printer.println(F("--------------------------------"));
  YIELD_WDT();

  // Customer info
  printer.print(F("Customer : "));
  printer.println(currentBill.customerName);
  printer.print(F("Account  : "));
  printer.println(currentBill.accountNo);
  printer.print(F("Class    : "));
  printer.println(currentBill.customerType);
  printer.print(F("Address  : "));
  printer.println(fitAddressForPrint(currentBill.address));
  printer.print(F("Barangay : "));
  printer.println(F("Makilas"));
  printer.println(F("--------------------------------"));
  YIELD_WDT();

  // Collector
  printer.justify('C');
  printer.println(F("Collector"));
  printer.println(currentBill.collector);
//   printer.println(F(""));
  YIELD_WDT();
  printer.println(F("--------------------------------"));
  YIELD_WDT();

  // Period covered
  printer.justify('C');
  printer.boldOn();
  printer.println(F("Period Covered"));
  printer.boldOff();
  printer.println(getPeriodCovered());
  printer.println();
  YIELD_WDT();

  // Meter readings
  printer.justify('C');
  printer.boldOn();
  printer.println(F("METER READINGS"));
  printer.boldOff();
  // Replicate Python alignment: center in fields of 10,12,10
  String prevStr = String(currentBill.prevReading);
  String presStr = String(currentBill.currReading);
  String usageStr = String(used);
  String header_cols = centerString("Prev", 10) + centerString("Present", 12) + centerString("Usage", 10);
  String values_cols = centerString(prevStr, 10) + centerString(presStr, 12) + centerString(usageStr, 10);
  printer.println(header_cols);
  printer.println(values_cols);
  printer.println(F("--------------------------------"));
  YIELD_WDT();
// %10lu%8lu%7lu
  // Rate , deduction & Penalty
  printer.justify('C');
  printer.boldOn();
  printer.println(F("BILLING"));
  printer.boldOff();
  printer.println();
  YIELD_WDT();
  printer.justify('L');  printer.print(F("Rate/m3      : PHP "));
  printer.println(currentBill.rate, 2);
  printer.print(F("Water Charge : PHP "));
  printer.println(currentBill.subtotal, 2);
  if (currentBill.deductions > 0) {
    printer.print(currentBill.deductionName);
    printer.print(F(" : PHP -"));
    printer.println(currentBill.deductions, 2);
  }
  if (currentBill.penalty > 0) {
    printer.print(F("Penalty  : PHP "));
    printer.println(currentBill.penalty, 2);
  }
  printer.println();
  printer.println(F("================================"));
  YIELD_WDT();

  // Total
  printer.justify('C');
  // use medium height for emphasis
  printer.setSize('M');
  printer.boldOn();
  printer.println(F("*TOTAL AMOUNT*"));
  printer.setSize('L');
  printer.print(F("PHP "));
  printer.println(total, 2);
  printer.boldOff();
  printer.setSize('S');
  printer.println(F("================================"));
  YIELD_WDT();

  // Due and Disconnect dates
  printer.println();
  printer.justify('L');
  printer.boldOn();
  printer.print(F("Due Date      : "));
  String dueDate = calculateDueDate(currentBill.billDate, getBillDueDaysSetting());
  printer.println(dueDate);
  printer.print(F("Disconnection : "));
  String disconnectionDate = calculateDueDate(currentBill.billDate, getDisconnectionDaysSetting());
  printer.println(disconnectionDate);
  printer.boldOff();
  printer.println(F("--------------------------------"));
  YIELD_WDT();

  // Reminders
  printer.println();
  printer.println(F("REMAINDERS:"));
  printer.println(F("- Please pay by the due date to avoid disconnection."));
  printer.println(F("- Bring this bill when paying."));
  printer.println(F("- For inquiries, contact the office."));
 
  YIELD_WDT();

  // Footer
  printer.justify('C');
  printer.println();
  printer.println(F("Please pay on or before due date"));
  printer.println(F("to avoid penalties."));
  printer.println(F(""));
  printer.println(F("Thank you!"));
  printer.println(F("Save Water, Save Life!"));
  printer.justify('L');
  YIELD_WDT();

  customFeed(4);
  YIELD_WDT();

  vTaskDelay(pdMS_TO_TICKS(500)); // ✅ replace delay()
}

String getPeriodCovered() {
  // Get current date from RTC
  String currentDate = getCurrentDateTimeString().substring(0,10); // YYYY-MM-DD
  int year = currentDate.substring(0,4).toInt();
  int month = currentDate.substring(5,7).toInt();
  
  // Previous month
  int prevMonth = month - 1;
  int prevYear = year;
  if (prevMonth == 0) {
    prevMonth = 12;
    prevYear = year - 1;
  }
  
  String months[13] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  
  String prevMonthStr = months[prevMonth];
  String currMonthStr = months[month];
  
  return prevMonthStr + " " + String(prevYear) + " - " + currMonthStr + " " + String(year);
}

void customFeed(int lines) {
  for (int i = 0; i < lines; i++) {
    printer.write(0x0A);
  }
}

String centerString(String s, int width) {
  int len = s.length();
  int totalPad = width - len;
  int leftPad = totalPad / 2;
  int rightPad = totalPad - leftPad;
  String result = "";
  for (int i = 0; i < leftPad; i++) result += " ";
  result += s;
  for (int i = 0; i < rightPad; i++) result += " ";
  return result;
}

String formatDateTime12Hour(String dateTimeStr) {
  // Input: "2026-02-04 19:04:07"
  // Output: "2026-02-04 7:04PM"
  if (dateTimeStr.length() < 19) return dateTimeStr; // Invalid format
  
  int year = dateTimeStr.substring(0,4).toInt();
  int month = dateTimeStr.substring(5,7).toInt();
  int day = dateTimeStr.substring(8,10).toInt();
  int hour = dateTimeStr.substring(11,13).toInt();
  int minute = dateTimeStr.substring(14,16).toInt();
  
  String ampm = (hour >= 12) ? "PM" : "AM";
  int hour12 = hour % 12;
  if (hour12 == 0) hour12 = 12;
  
  char formatted[20];
  sprintf(formatted, "%04d-%02d-%02d %d:%02d%s", year, month, day, hour12, minute, ampm.c_str());
  return String(formatted);
}

#endif  // BILL_PRINTER_H