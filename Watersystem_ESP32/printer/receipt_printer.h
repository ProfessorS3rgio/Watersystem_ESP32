// NOTE: Receipt printing relies on shared helpers from bill printing.
#ifndef RECEIPT_PRINTER_H
#define RECEIPT_PRINTER_H

#include <Adafruit_Thermal.h>
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

static String buildOfficeBarcodeData(const ReceiptData& receipt) {
  // Digits-only format for office scanning:
  // barcode = <accountDigits><amountPaidDigits>
  // Example: account=M-1000, paid=8000 => 10008000
  // Example: account=M-001,  paid=8000 => 0018000
  String accountDigits = digitsOnly(receipt.accountNo);
  long paid = lroundf(receipt.amountPaid);
  if (paid < 0) paid = 0;
  return accountDigits + String(paid);
}

static void printOfficeCopyBarcodeSection(const ReceiptData& receipt) {
  // Office copy stub: cut line, barcode, and key payment fields.
  printer.println(F("8< -----------------------------"));

  String barcodeData = buildOfficeBarcodeData(receipt);

  if (barcodeData.length() > 0) {
    printer.println(F(""));
    printer.justify('C');

    // Hide HRI text (the digits printed under the barcode)
    printer.write(0x1D);
    printer.write('H');
    printer.write((uint8_t)0x00);
    // Prefer library barcode helper when CODE93 is available.
    #if defined(CODE93)
      printer.printBarcode(barcodeData.c_str(), CODE93);
    #else
      // Fallback: CODE39 via ESC/POS (still digits-only data)
      // HRI hidden
      printer.write(0x1D);
      printer.write('H');
      printer.write((uint8_t)0x00);
      // Height
      printer.write(0x1D);
      printer.write('h');
      printer.write((uint8_t)80);
      // Module width
      printer.write(0x1D);
      printer.write('w');
      printer.write((uint8_t)2);

      // Print CODE39: GS k m data NUL
      printer.write(0x1D);
      printer.write('k');
      printer.write((uint8_t)0x04); // m=4 => CODE39
      for (size_t i = 0; i < barcodeData.length(); i++) {
        printer.write((uint8_t)barcodeData[i]);
      }
      printer.write((uint8_t)0x00);
    #endif

    printer.println(F(""));
    printer.println(F(""));
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
  printer.println(F("Payment Method: Cash"));
  printer.print(F("Collector: "));
  printer.println(receipt.collector);
  printer.boldOff();
  printer.println(F(""));
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


  printer.println(F("")); 
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
  printer.println(receipt.address);
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
  printer.println(F("Payment Method: Cash"));
  printer.boldOff();
  printer.println(F(""));
  // Footer
  printer.justify('C');
  printer.println(F(""));
  printer.println(F("Thank you for your payment!"));
  printer.println(F("This serves as an official receipt."));
  printer.println(F("Save Water, Save Life!"));

  // Office stub copy
  printer.println(F(""));
  printOfficeCopyBarcodeSection(receipt);

  printer.justify('L');

  customFeed(4);

  vTaskDelay(pdMS_TO_TICKS(500)); // replace delay()
}

#endif  // RECEIPT_PRINTER_H