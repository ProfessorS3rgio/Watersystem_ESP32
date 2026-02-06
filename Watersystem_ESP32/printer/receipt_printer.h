// NOTE: Receipt printing relies on shared helpers from bill printing.
#ifndef RECEIPT_PRINTER_H
#define RECEIPT_PRINTER_H

#include <Adafruit_Thermal.h>
#include "../database/bill_transactions_database.h"
#include "bill_printer.h"  // centerString(), formatDateTime12Hour(), getPeriodCovered(), customFeed(), logo

void printReceipt() {
  printer.wake();
  printer.setDefault();

  unsigned long used = currentReceipt.currReading - currentReceipt.prevReading;
  float total = currentReceipt.total;
  float amountPaid = currentReceipt.amountPaid;
  float change = currentReceipt.change;

//   customFeed(1);

//   printer.justify('C');
  printer.printBitmap(LOGO_WIDTH, LOGO_HEIGHT, logo);

//   printer.justify('R');
//   printer.setSize('S');
//   printer.println(F("DONA JOSEFA MAKILAS"));
//   printer.println(F("Water & Sanitation"));
//   printer.justify('L');
//   printer.printBitmap(LOGO_WIDTH, LOGO_HEIGHT, logo);
//   printer.println(F("Assoc. Bulu-an, IPIL"));
//   printer.println(F("TIN: 464-252-005"));
//   printer.println(F(""));
//   printer.println(F("--------------------------------"));
  printer.justify('C');
  printer.setSize('M');
  printer.boldOn();
  printer.println(F("OFFICIAL RECEIPT"));
  printer.setSize('S');
  printer.boldOff();
  printer.justify('L');
  printer.println(F(""));
//   printer.println(F("--------------------------------"));

  printer.justify('L');
  // Receipt No and Date/Time
  printer.print(F("Receipt No  : "));
  printer.println(currentReceipt.receiptNumber);
  printer.print(F("Date/Time   : "));
  printer.println(formatDateTime12Hour(currentReceipt.paymentDateTime));
  printer.println(F("--------------------------------"));

  // Customer info
  printer.print(F("Customer : "));
  printer.println(currentReceipt.customerName);
  printer.print(F("Account  : "));
  printer.println(currentReceipt.accountNo);
  printer.print(F("Class    : "));
  printer.println(currentReceipt.customerType);
  printer.print(F("Address  : "));
  printer.println(currentReceipt.address);
  printer.print(F("Barangay : "));
  printer.println(F("Makilas"));
  printer.println(F("--------------------------------"));

  // Collector
  printer.justify('C');
  printer.println(F("Collector"));
  printer.println(currentReceipt.collector);
  printer.println(F("--------------------------------"));

  // Period covered
  printer.justify('C');
  printer.boldOn();
  printer.println(F("Billing Period"));
  printer.boldOff();
  printer.println(getPeriodCovered());
  printer.println(F(""));
  printer.println(F("--------------------------------"));

  // Meter readings
  printer.justify('C');
  printer.boldOn();
  printer.println(F("METER READINGS"));
  printer.boldOff();
  String prevStr = String(currentReceipt.prevReading);
  String presStr = String(currentReceipt.currReading);
  String usageStr = String(used);
  String header_cols = centerString("Prev", 10) + centerString("Present", 12) + centerString("Usage", 10);
  String values_cols = centerString(prevStr, 10) + centerString(presStr, 12) + centerString(usageStr, 10);
  printer.println(header_cols);
  printer.println(values_cols);
  printer.println(F("--------------------------------"));

  // Billing details
  printer.justify('C');
  printer.boldOn();
  printer.println(F("BILLING"));
  printer.boldOff();
  printer.println(F(""));
  printer.justify('L');
  printer.print(F("Rate per cubic       : PHP "));
  printer.println(currentReceipt.rate, 2);
  printer.print(F("Water Charge      : PHP "));
  printer.println(currentReceipt.subtotal, 2);
  if (currentReceipt.deductions > 0) {
    printer.print(F("Less Deductions   : PHP "));
    printer.println(currentReceipt.deductions, 2);
  }
  if (currentReceipt.penalty > 0) {
    printer.print(F("Penalty / Late Fee: PHP "));
    printer.println(currentReceipt.penalty, 2);
  }
  printer.println(F("----------------------------"));
  printer.print(F("Total Amount Due  : PHP "));
  printer.println(total, 2);
  printer.println(F("================================"));

  // Payment details
  printer.println(F(""));
  printer.justify('L');
  printer.boldOn();
  printer.print(F("Amount Received: PHP "));
  printer.boldOff();
  printer.println(amountPaid, 2);
  printer.boldOn();
  if (change > 0) {
    printer.print(F("Change Returned: PHP "));
    printer.boldOff();
    printer.println(change, 2);
    printer.boldOn();
  }
  printer.print(F("Payment Method: Cash"));
  printer.boldOff();
  printer.println(F(""));
  // Footer
  printer.justify('C');
  printer.println(F(""));
  printer.println(F("Thank you for your payment!"));
  printer.println(F("This serves as an official receipt."));
  printer.println(F("Save Water, Save Life!"));
  printer.justify('L');

  customFeed(4);

  vTaskDelay(pdMS_TO_TICKS(500)); // replace delay()
}

#endif  // RECEIPT_PRINTER_H