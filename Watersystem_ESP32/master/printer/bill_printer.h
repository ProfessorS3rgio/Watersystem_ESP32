#ifndef BILL_PRINTER_H
#define BILL_PRINTER_H

#include <NimBLEDevice.h>
#include "../database/bill_database.h"  // For BillData currentBill

// BLE client objects defined in main sketch (master)
extern NimBLEClient* pBleClient;
extern NimBLERemoteCharacteristic* pBleCharacteristic;

// helper to transmit a command string over BLE (master app)
extern bool bleSend(const String &cmd);
extern bool blePrepareForPrint(uint32_t timeoutMs);
extern void bleShutdownAfterPrint();

void printBill();

void printBill() {
  blePrepareForPrint(6000);

  // BLE-only print path: forward to slave printer.
  if (pBleCharacteristic && pBleClient && pBleClient->isConnected()) {
    String msg = String("PRINT_BILL|") + currentBill.refNumber + "|" + currentBill.readingDateTime \
                 + "|" + currentBill.customerName + "|" + currentBill.accountNo \
                 + "|" + currentBill.customerType + "|" + currentBill.address \
                 + "|" + currentBill.collector + "|" + String(currentBill.prevReading) \
                 + "|" + String(currentBill.currReading) + "|" + String(currentBill.rate, 2) \
                 + "|" + String(currentBill.subtotal, 2);
    if (currentBill.deductions > 0) {
      msg += "|" + String(currentBill.deductions, 2);
    }
    if (currentBill.penalty > 0) {
      msg += "|" + String(currentBill.penalty, 2);
    }
    msg += "|" + String(currentBill.total, 2);
    // include bill date if available
    if (currentBill.billDate.length() > 0) {
      msg += "|" + currentBill.billDate;
    }
    msg += "\n";
    if (bleSend(msg)) {
      Serial.println(F("Bill forwarded to BLE slave for printing"));
      bleShutdownAfterPrint();
      return;
    }
  }

  Serial.println(F("Bill print skipped: BLE slave unavailable"));
  bleShutdownAfterPrint();
}

#endif  // BILL_PRINTER_H