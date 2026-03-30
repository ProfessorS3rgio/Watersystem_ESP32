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
extern bool bleCheckPaperPresent(uint32_t timeoutMs);
extern void bleShutdownAfterPrint();

void printBill();

void printBill() {
  blePrepareForPrint(6000);

  // BLE-only print path: forward to slave printer.
  if (pBleCharacteristic && pBleClient && pBleClient->isConnected()) {
    if (!bleCheckPaperPresent(1500)) {
      Serial.println(F("Bill print blocked: no paper detected on slave"));
      bleShutdownAfterPrint();
      return;
    }

    String billDate = currentBill.billDate;
    if (billDate.length() < 10 && currentBill.readingDateTime.length() >= 10) {
      billDate = currentBill.readingDateTime.substring(0, 10);
    }

    const int dueDayOfMonth = getBillDueDaysSetting();
    const int disconnectDayOfMonth = getDisconnectionDaysSetting();

    String msg = String("PRINT_BILL|") + currentBill.refNumber + "|" + currentBill.readingDateTime \
                 + "|" + currentBill.customerName + "|" + currentBill.accountNo \
                 + "|" + currentBill.customerType + "|" + currentBill.address \
                 + "|" + currentBill.collector + "|" + String(currentBill.prevReading) \
                 + "|" + String(currentBill.currReading) + "|" + String(currentBill.rate, 2) \
                 + "|" + String(currentBill.subtotal, 2)
                 + "|" + String(currentBill.deductions, 2)
                 + "|" + String(currentBill.penalty, 2)
                 + "|" + String(currentBill.total, 2)
                 + "|" + billDate
                 + "|" + String(dueDayOfMonth)
                 + "|" + String(disconnectDayOfMonth);
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