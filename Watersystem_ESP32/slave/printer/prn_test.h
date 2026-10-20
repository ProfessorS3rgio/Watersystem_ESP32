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

inline void printPrnTest() {
  loadPrnTestData();
  // Keep the compact header font active throughout the experimental bill.
  printPrnTestHeading(true);
  printBillContent();

  // Do not let the PRN_TEST font selection leak into later production jobs.
  printer.setFont('A');
}

// Header-only test for checking alignment, font size, and special characters
// without consuming paper on the complete sample bill.
inline void printPrnHeaderOnly() {
  printPrnTestHeading();
}

#endif  // PRN_TEST_H
