#ifndef BILL_PRINTER_H
#define BILL_PRINTER_H

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>
#include "../database/bill_database.h"

extern BLEClient* pBleClient;
extern BLERemoteCharacteristic* pBleCharacteristic;

extern bool bleSend(const String &cmd);
extern bool blePrepareForPrint(uint32_t timeoutMs);
extern void bleShutdownAfterPrint();

inline void printBill() {
  if (!blePrepareForPrint(6000)) {
    Serial.println(F("[BLE] Bill print skipped: slave printer not ready"));
    return;
  }

  if (!(pBleCharacteristic && pBleClient && pBleClient->isConnected())) {
    Serial.println(F("[BLE] Bill print skipped: slave printer disconnected"));
    bleShutdownAfterPrint();
    return;
  }

  String msg = String("PRINT_BILL|") + currentBill.refNumber + "|" + currentBill.readingDateTime
               + "|" + currentBill.customerName + "|" + currentBill.accountNo
               + "|" + currentBill.customerType + "|" + currentBill.address
               + "|" + currentBill.collector + "|" + String(currentBill.prevReading)
               + "|" + String(currentBill.currReading) + "|" + String(currentBill.rate, 2)
               + "|" + String(currentBill.subtotal, 2);

  if (currentBill.deductions > 0) {
    msg += "|" + String(currentBill.deductions, 2);
  }
  if (currentBill.penalty > 0) {
    msg += "|" + String(currentBill.penalty, 2);
  }

  msg += "|" + String(currentBill.total, 2);

  if (currentBill.billDate.length() > 0) {
    msg += "|" + currentBill.billDate;
  }

  msg += "\n";

  if (bleSend(msg)) {
    Serial.println(F("Bill forwarded to BLE slave for printing"));
  } else {
    Serial.println(F("[BLE] Bill print failed to send to slave"));
  }

  bleShutdownAfterPrint();
}

#endif  // BILL_PRINTER_H