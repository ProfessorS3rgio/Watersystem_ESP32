// NOTE: Receipt printing relies on shared helpers from bill printing.
#ifndef RECEIPT_PRINTER_H
#define RECEIPT_PRINTER_H

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>
#include "../database/bill_transactions_database.h"

// BLE client objects defined in main sketch (master)
extern BLEClient* pBleClient;
extern BLERemoteCharacteristic* pBleCharacteristic;
extern bool bleSend(const String &cmd);
extern bool blePrepareForPrint(uint32_t timeoutMs);
extern void bleShutdownAfterPrint();

void printReceipt() {
  if (!blePrepareForPrint(6000)) {
    Serial.println(F("[BLE] Receipt print skipped: slave printer not ready"));
    return;
  }

  if (!(pBleCharacteristic && pBleClient && pBleClient->isConnected())) {
    Serial.println(F("[BLE] Receipt print skipped: slave printer disconnected"));
    bleShutdownAfterPrint();
    return;
  }

  ReceiptData receipt = currentReceipt;
  if (receipt.collector.length() == 0) {
    receipt.collector = COLLECTOR_NAME_VALUE;
  }

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
  } else {
    Serial.println(F("[BLE] Receipt print failed to send to slave"));
  }

  bleShutdownAfterPrint();
}

#endif  // RECEIPT_PRINTER_H