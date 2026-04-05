#ifndef BILL_PRINTER_H
#define BILL_PRINTER_H

#include <NimBLEDevice.h>
#include "../database/bill_database.h"  // For BillData currentBill

// BLE client objects defined in main sketch (master)
extern NimBLEClient* pBleClient;
extern NimBLERemoteCharacteristic* pBleCharacteristic;

// helper to transmit a command string over BLE (master app)
extern bool bleSend(const String &cmd);
extern bool bleSendBillCommand(const String &cmd, uint32_t ackTimeoutMs, uint8_t maxAttempts);
extern bool blePrepareForPrint(uint32_t timeoutMs);
extern bool bleCheckPaperPresent(uint32_t timeoutMs);
extern void bleShutdownAfterPrint();

void printBill();

void printBill() {
  blePrepareForPrint(6000);

  auto sanitizeField = [](String value) {
    value.replace("|", "/");
    value.replace("\r", " ");
    value.replace("\n", " ");
    return value;
  };

  // BLE-only print path: forward to slave printer.
  if (pBleCharacteristic && pBleClient && pBleClient->isConnected()) {
    if (!bleCheckPaperPresent(1500)) {
      Serial.println(F("Bill print blocked: no paper detected on slave"));
      bleShutdownAfterPrint();
      return;
    }

    String billDate = sanitizeField(currentBill.billDate);
    String readingDateTime = sanitizeField(currentBill.readingDateTime);
    String customerName = sanitizeField(currentBill.customerName);
    String accountNo = sanitizeField(currentBill.accountNo);
    String customerType = sanitizeField(currentBill.customerType);
    String address = sanitizeField(currentBill.address);
    String collector = sanitizeField(currentBill.collector);

    if (billDate.length() < 10 && readingDateTime.length() >= 10) {
      billDate = readingDateTime.substring(0, 10);
    }
    if (readingDateTime.length() < 10) {
      if (billDate.length() >= 10) {
        readingDateTime = billDate + " 00:00:00";
      } else {
        readingDateTime = getCurrentDateTimeString();
      }
    }

    const int dueDayOfMonth = getBillDueDaysSetting();
    const int disconnectDayOfMonth = getDisconnectionDaysSetting();

    String deductionLabel = sanitizeField(currentBill.deductionName);
    if (currentBill.deductions > 0.0f && deductionLabel.length() == 0) {
      deductionLabel = "Deduction";
    }

    String msg;
    msg.reserve(384);
    msg += "PRINT_BILL";
    msg += "|";
    msg += sanitizeField(currentBill.refNumber);
    msg += "|";
    msg += readingDateTime;
    msg += "|";
    msg += customerName;
    msg += "|";
    msg += accountNo;
    msg += "|";
    msg += customerType;
    msg += "|";
    msg += address;
    msg += "|";
    msg += collector;
    msg += "|";
    msg += String(currentBill.prevReading);
    msg += "|";
    msg += String(currentBill.currReading);
    msg += "|";
    msg += String(currentBill.rate, 2);
    msg += "|";
    msg += String(currentBill.subtotal, 2);
    msg += "|";
    msg += deductionLabel;
    msg += "|";
    msg += String(currentBill.deductions, 2);
    msg += "|";
    msg += String(currentBill.penalty, 2);
    msg += "|";
    msg += String(currentBill.total, 2);
    msg += "|";
    msg += billDate;
    msg += "|";
    msg += String(dueDayOfMonth);
    msg += "|";
    msg += String(disconnectDayOfMonth);

    Serial.print(F("[BLE] PRINT_BILL payload len="));
    Serial.println(msg.length());
    msg += "\n";
    if (bleSendBillCommand(msg, 4500, 3)) {
      Serial.println(F("Bill forwarded to BLE slave for printing"));
      // Give slave a short window to process the last BLE chunk before disconnect.
      delay(120);
      bleShutdownAfterPrint();
      return;
    }
  }

  Serial.println(F("Bill print skipped: BLE slave unavailable"));
  bleShutdownAfterPrint();
}

#endif  // BILL_PRINTER_H