#ifndef DATABASE_VIEWER_H
#define DATABASE_VIEWER_H

#include <Arduino.h>
#include "bill_database.h"

// Function to print bills list
void printBillsList() {
  Serial.println(F("===== BILLS DATABASE ====="));
  Serial.println(F("ID | Ref Number    | CustID | ReadID | Bill No       | Date       | Rate | Charges | Penalty | Total | Status | Created"));
  Serial.println(F("---|---------------|--------|--------|---------------|------------|------|---------|---------|-------|--------|--------"));

  for (size_t i = 0; i < bills.size(); ++i) {
    const Bill& b = bills[i];
    Serial.print(b.bill_id); Serial.print(F(" | "));
    Serial.print(b.reference_number); Serial.print(F(" | "));
    Serial.print(b.customer_id); Serial.print(F(" | "));
    Serial.print(b.reading_id); Serial.print(F(" | "));
    Serial.print(b.bill_no); Serial.print(F(" | "));
    Serial.print(b.bill_date); Serial.print(F(" | "));
    Serial.print(b.rate_per_m3, 2); Serial.print(F(" | "));
    Serial.print(b.charges, 2); Serial.print(F(" | "));
    Serial.print(b.penalty, 2); Serial.print(F(" | "));
    Serial.print(b.total_due, 2); Serial.print(F(" | "));
    Serial.print(b.status); Serial.print(F(" | "));
    Serial.println(b.created_at);
  }
  Serial.println(F("====================================="));
}

// Function to display all database data via Serial
void displayAllDatabaseData() {
  Serial.println(F("=== CUSTOMERS DATABASE ==="));
  printCustomersList();
  Serial.println();

  Serial.println(F("=== DEDUCTIONS DATABASE ==="));
  printDeductionsList();
  Serial.println();

  Serial.println(F("=== CUSTOMER TYPES DATABASE ==="));
  printCustomerTypesList();
  Serial.println();

  Serial.println(F("=== BILLS DATABASE ==="));
  printBillsList();
  Serial.println();
}

#endif