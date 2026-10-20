#ifndef PRN_TEST_H
#define PRN_TEST_H

#include <Arduino.h>
#include "printer_serial.h"
#include "bill_printer.h"
#include "../database/bill_database.h"

extern ThermalPrinter printer;
extern BillData currentBill;

// Keep PRN_TEST experiments in this file so production PRINT_BILL and
// PRINT_RECEIPT layouts are not changed while the new design is evaluated.
inline void printPrnTestHeading(bool keepFontB = false) {
  printer.setDefault();
  printer.justify('C');
  printer.setFont('B');
  printer.boldOn();

  // QR204 serial text is not UTF-8. Its common CP437-compatible character
  // table uses byte 0xA4 for the lowercase n with tilde in "Doña".
  printer.print(F("Do"));
  printer.write(0xA4);
  printer.println(F("a Josefa Makilas Buluan Caparan"));
  printer.println(F("Water System Association"));
  printer.boldOff();
  printer.println(F("Makilas Ipil Zamboanga Sibugay"));

  if (!keepFontB) {
    printer.setFont('A');
  }
  printer.justify('L');
  printer.println();

  printer.justify('C');
  printer.boldOn();
  printer.println(F("STATEMENT OF ACCOUNT"));
  printer.boldOff();
  printer.justify('L');
  printer.println();
}

inline void loadPrnTestData() {
  currentBill.refNumber = "PRN-TEST-0001";
  currentBill.readingDateTime = getCurrentDateTimeString();
  currentBill.customerName = "JUAN DELA CRUZ";
  currentBill.accountNo = "ACC-000123";
  currentBill.customerType = "Residential";
  currentBill.address = "Purok 1, Makilas";
  currentBill.collector = "TEST COLLECTOR";
  currentBill.prevReading = 1234;
  currentBill.currReading = 1256;
  currentBill.rate = 15.50f;
  currentBill.subtotal = 341.00f;
  currentBill.deductions = 50.00f;
  currentBill.deductionName = "Senior";
  currentBill.penalty = 10.00f;
  currentBill.total = 301.00f;
  currentBill.billDate = getCurrentDateTimeString();
}

static String prnTestFitText(const String& value, size_t maxChars = 21) {
  return value.length() <= maxChars ? value : value.substring(0, maxChars);
}

static float prnTestAfterDueSurcharge(const BillData& bill) {
  String type = bill.customerType;
  type.toLowerCase();
  String deduction = bill.deductionName;
  deduction.toLowerCase();

  if (type == "residential") return deduction.indexOf("senior") >= 0 ? 20.0f : 50.0f;
  if (type == "commercial") return 50.0f;
  if (type == "officers" || type == "communical faucet" || type == "communal faucet") return 20.0f;
  return 0.0f;
}

// Test-only copy of the bill layout. Change this function freely while
// evaluating fonts, wording, spacing, and alignment; production printBill()
// is never called or modified by PRN_TEST.
inline void printPrnTestBillBody() {
  const unsigned long used = currentBill.currReading - currentBill.prevReading;
  const float total = currentBill.total;

  printer.justify('L');
  printer.print(F("Ref No      : "));
  printer.println(currentBill.refNumber);
  printer.print(F("Date & Time : "));
  printer.println(formatDateTime12Hour(currentBill.readingDateTime));
  printer.println(F("------------------------------------------"));

  printer.print(F("Customer : "));
  printer.println(prnTestFitText(currentBill.customerName));
  printer.print(F("Account  : "));
  printer.println(currentBill.accountNo);
  printer.print(F("Class    : "));
  printer.println(currentBill.customerType);
  printer.print(F("Address  : "));
  printer.println(prnTestFitText(currentBill.address));
  printer.print(F("Barangay : "));
  printer.println(F("Dona Josefa"));
  printer.println(F("-----------------------------------------"));

  printer.justify('C');
  printer.println(F("Temp Reader"));
  printer.println(currentBill.collector);
  printer.println(F("-----------------------------------------"));

  printer.boldOn();
  printer.println(F("Period Covered"));
  printer.boldOff();
  printer.println(getPeriodCovered());
  printer.println();

  printer.boldOn();
  printer.println(F("METER READINGS"));
  printer.boldOff();
  const String previous = String(currentBill.prevReading);
  const String present = String(currentBill.currReading);
  const String usage = String(used);
  printer.println(centerString("Prev", 10) + centerString("Present", 12) + centerString("Usage", 10));
  printer.println(centerString(previous, 10) + centerString(present, 12) + centerString(usage, 10));
  printer.println(F("-----------------------------------------"));

  printer.boldOn();
  printer.println(F("BILLING"));
  printer.boldOff();
  printer.println();
  printer.justify('L');
  printer.print(F("Rate/m3      : PHP "));
  printer.println(currentBill.rate, 2);
  printer.print(F("Water Charge : PHP "));
  printer.println(currentBill.subtotal, 2);
  if (currentBill.deductions > 0) {
    printer.print(currentBill.deductionName.length() > 0 ? currentBill.deductionName : String("Deduction"));
    printer.print(F(" : PHP -"));
    printer.println(currentBill.deductions, 2);
  }
  if (currentBill.penalty > 0) {
    printer.print(F("Penalty  : PHP "));
    printer.println(currentBill.penalty, 2);
  }
  printer.println();
  printer.println(F("=========================================="));

  printer.justify('C');
  printer.setSize('M');
  printer.boldOn();
  printer.println(F("*TOTAL AMOUNT*"));
  printer.setSize('L');
  printer.print(F("PHP "));
  printer.println(total, 2);
  printer.boldOff();
  printer.setSize('S');
  printer.println(F("=========================================="));

  printer.println();
  printer.justify('L');
  printer.boldOn();
  printer.print(F("Due Date      : "));
  printer.println(calculateDueDate(currentBill.billDate, getBillDueDaysSetting()));
  printer.print(F("Disconnection : "));
  printer.println(calculateDueDate(currentBill.billDate, getDisconnectionDaysSetting()));
  printer.print(F("After Due Date: PHP "));
  printer.println(total + prnTestAfterDueSurcharge(currentBill), 2);
  printer.boldOff();
  printer.println(F("------------------------------------------"));

  printer.println();
  printer.println(F("REMAINDERS:"));
  printer.println(F("- Please pay by the due date to avoid disconnection."));
  printer.println(F("- Bring this bill when paying."));
  printer.println(F("- For inquiries, contact the office."));

  printer.justify('C');
  printer.println();
  printer.println(F("This bill also serves as"));
  printer.println(F("Notice of Disconnection!"));
  printer.println();
  printer.println(F("Thank you!"));
  printer.println(F("Save Water, Save Life!"));
  printer.justify('L');

  for (int line = 0; line < 4; ++line) printer.write('\n');
  vTaskDelay(pdMS_TO_TICKS(500));
}

inline void printPrnTest() {
  loadPrnTestData();
  // Keep the compact header font active throughout the experimental bill.
  printPrnTestHeading(true);
  printPrnTestBillBody();

  // Do not let the PRN_TEST font selection leak into later production jobs.
  printer.setFont('A');
}

// Header-only test for checking alignment, font size, and special characters
// without consuming paper on the complete sample bill.
inline void printPrnHeaderOnly() {
  printPrnTestHeading();
}

#endif  // PRN_TEST_H
