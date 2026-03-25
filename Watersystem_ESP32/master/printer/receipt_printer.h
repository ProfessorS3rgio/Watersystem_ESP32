// NOTE: Receipt printing relies on shared helpers from bill printing.
#ifndef RECEIPT_PRINTER_H
#define RECEIPT_PRINTER_H

#include <NimBLEDevice.h>
#include "../database/bill_transactions_database.h"
#include "bill_printer.h"

// BLE client objects defined in main sketch (master)
extern NimBLEClient* pBleClient;
extern NimBLERemoteCharacteristic* pBleCharacteristic;
extern bool bleSend(const String &cmd);
extern bool blePrepareForPrint(uint32_t timeoutMs);
extern bool bleCheckPaperPresent(uint32_t timeoutMs);
extern void bleShutdownAfterPrint();

void printReceipt() {
  blePrepareForPrint(6000);

  // forward over BLE if connected to slave
  if (pBleCharacteristic && pBleClient && pBleClient->isConnected()) {
    if (!bleCheckPaperPresent(1500)) {
      Serial.println(F("Receipt print blocked: no paper detected on slave"));
      bleShutdownAfterPrint();
      return;
    }

    ReceiptData receipt = currentReceipt;
    String msg = String("PRINT_RECEIPT|") + receipt.receiptNumber + "|" + receipt.paymentDateTime
                 + "|" + receipt.customerName + "|" + receipt.accountNo
                 + "|" + receipt.customerType + "|" + receipt.address
                 + "|" + receipt.collector + "|" + String(receipt.prevReading)
                 + "|" + String(receipt.currReading) + "|" + String(receipt.rate, 2)
                 + "|" + String(receipt.subtotal, 2) + "|" + String(receipt.deductions, 2)
                 + "|" + String(receipt.penalty, 2) + "|" + String(receipt.total, 2)
                 + "|" + String(receipt.amountPaid, 2) + "|" + String(receipt.change, 2)
                 + "\n";
    if (bleSend(msg)) {
      Serial.println(F("Receipt forwarded to BLE slave for printing"));
      bleShutdownAfterPrint();
      return;
    }
  }

  Serial.println(F("Receipt print skipped: BLE slave unavailable"));
  bleShutdownAfterPrint();
}

#endif  // RECEIPT_PRINTER_H