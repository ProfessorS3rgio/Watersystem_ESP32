#ifndef BLE_PRINTER_TEST_H
#define BLE_PRINTER_TEST_H

#include <Arduino.h>

#include "bill_printer.h"
#include "../bluetooth/ble_slave_manager.h"

static void prepareSampleBillForBlePrintTest() {
  const String now = getCurrentDateTimeString();

  currentBill.refNumber = F("TEST-2026-0001");
  currentBill.readingDateTime = now;
  currentBill.billDate = now.substring(0, 10);
  currentBill.dueDate = currentBill.billDate;
  currentBill.customerName = F("JUAN DELA CRUZ");
  currentBill.accountNo = F("MKL-001");
  currentBill.customerType = F("Residential");
  currentBill.address = F("Purok 1, Makilas");
  currentBill.collector = COLLECTOR_NAME_VALUE;
  currentBill.prevReading = 120;
  currentBill.currReading = 138;
  currentBill.usage = currentBill.currReading - currentBill.prevReading;
  currentBill.rate = 15.00f;
  currentBill.minCharge = 0.0f;
  currentBill.minM3 = 0;
  currentBill.subtotal = 270.00f;
  currentBill.deductions = 20.00f;
  currentBill.deductionName = F("Senior Discount");
  currentBill.penalty = 0.00f;
  currentBill.total = 250.00f;
  currentBill.status = F("Pending");
}

static bool printSampleBillViaBleSlave() {
  if (!blePrepareForPrint(8000)) {
    Serial.println(F("[BLE] Slave printer not ready; sample print skipped"));
    return false;
  }

  prepareSampleBillForBlePrintTest();

  printBill();
  Serial.println(F("[BLE] Sample bill sent to slave printer"));
  return true;
}

#endif  // BLE_PRINTER_TEST_H