// NOTE: Receipt printing relies on shared helpers from bill printing.
#ifndef RECEIPT_PRINTER_H
#define RECEIPT_PRINTER_H

#include "printer/printer_serial.h"
#include "../database/bill_transactions_database.h"
#include "bill_printer.h"  // centerString(), formatDateTime12Hour(), getPeriodCovered(), customFeed(), logo

static String digitsOnly(const String& s) {
  String out;
  out.reserve(s.length());
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    
    if (c >= '0' && c <= '9') out += c;
  }
  return out;
}

// old barcode helper left for reference but no longer used
static String buildOfficeBarcodeData(const ReceiptData& receipt) {
  // Digits-only format for office scanning:
  String accountDigits = digitsOnly(receipt.accountNo);
  long paid = lroundf(receipt.amountPaid);
  if (paid < 0) paid = 0;
  return accountDigits + String(paid);
}

static String limitPrintChars(const String& value, size_t maxChars) {
  if (value.length() <= maxChars) return value;
  return value.substring(0, maxChars);
}

static void printOfficeCopyBarcodeSection(const ReceiptData& receipt) {
  // Office copy stub: cut line, barcode, and key payment fields.
  printer.println(F("8< -----------------------------"));

  // build QR code payload with account (including any prefix letters), amount and datetime
  String qrData = "ACCOUNT:" + receipt.accountNo
                + "|AMOUNT:" + String(receipt.amountPaid, 2)
                + "|DATE:" + receipt.paymentDateTime;

  if (qrData.length() > 0) {
    // blank line before QR code
    printer.println();
    printer.justify('C');

    // print QR code using printer object
    printer.printQRCode(qrData);

    // two blank lines after
    printer.println();
    printer.println();
    printer.justify('L');
  }

  float amountPaid = receipt.amountPaid;
  float change = receipt.change;

  printer.print(F("Customer    : "));
  printer.println(receipt.customerName);
  printer.print(F("Account     : "));
  printer.println(receipt.accountNo);

  printer.print(F("Date & Time : "));
  printer.println(formatDateTime12Hour(receipt.paymentDateTime));
  printer.println();
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
  printer.println(F("Payment Method : Cash"));
  printer.print(F("Collector: "));
  printer.println(receipt.collector);
  printer.boldOff();
  printer.println();
}

void printReceipt() {
  printer.wake();
  printer.setDefault();

  // Snapshot receipt data to avoid partial/mid-print mutations.
  ReceiptData receipt = currentReceipt;
  if (receipt.collector.length() == 0) {
    receipt.collector = COLLECTOR_NAME_VALUE;
  }

  unsigned long used = receipt.currReading - receipt.prevReading;
  float total = receipt.total;
  float amountPaid = receipt.amountPaid;
  float change = receipt.change;

//   customFeed(1);

//   printer.justify('C');
  printer.printBitmap(LOGO_WIDTH, LOGO_HEIGHT, logo);


  printer.println();
  printer.justify('C');
  printer.setSize('M');
  printer.boldOn();
  printer.println(F("OFFICIAL RECEIPT"));
  printer.setSize('S');
  printer.boldOff();
  printer.justify('L');
  printer.println();
//   printer.println(F("--------------------------------"));

  printer.justify('L');
  // Receipt No and Date/Time
  printer.print(F("Ref No      : "));
  printer.println(receipt.receiptNumber);
  printer.print(F("Date & Time : "));
  printer.println(formatDateTime12Hour(receipt.paymentDateTime));
  printer.println(F("--------------------------------"));

  // Customer info
  printer.print(F("Customer : "));
  printer.println(receipt.customerName);
  printer.print(F("Account  : "));
  printer.println(receipt.accountNo);
  printer.print(F("Class    : "));
  printer.println(receipt.customerType);
  printer.print(F("Address  : "));
  printer.println(limitPrintChars(receipt.address, 21));
  printer.print(F("Barangay : "));
  printer.println(F("Makilas"));
  printer.println(F("--------------------------------"));

  // Collector
  printer.justify('L');
  printer.print(F("Collector : "));
  printer.println(receipt.collector);
  printer.println(F("--------------------------------"));

  // Period covered
  printer.justify('C');
  printer.boldOn();
  printer.println(F("Billing Period"));
  printer.boldOff();
  printer.println(getPeriodCovered());
  printer.println();
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
  printer.println();
  printer.justify('L');
  printer.print(F("Rate per cubic    : PHP "));
  printer.println(receipt.rate, 2);
  printer.print(F("Water Charge      : PHP "));
  printer.println(receipt.subtotal, 2);
  if (receipt.deductions > 0) {
    printer.print(F("Less Deductions   : PHP "));
    printer.println(receipt.deductions, 2);
  }
  if (receipt.penalty > 0) {
    printer.print(F("Penalty / Late Fee: PHP "));
    printer.println(receipt.penalty, 2);
  }
  printer.println(F("--------------------------------"));
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
  printer.println(F("Payment Method : Cash"));
  printer.boldOff();
  printer.println();
  // Footer
  printer.justify('C');
  printer.println();
  printer.println(F("Thank you for your payment!"));
  printer.println(F("This serves as an official receipt."));
  printer.println(F("Save Water, Save Life!"));

  // Office stub copy
  printer.println();
  printOfficeCopyBarcodeSection(receipt);

  printer.justify('L');

  customFeed(4);

  vTaskDelay(pdMS_TO_TICKS(500)); // replace delay()
}

#endif  // RECEIPT_PRINTER_H