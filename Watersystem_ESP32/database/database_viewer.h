#ifndef DATABASE_VIEWER_H
#define DATABASE_VIEWER_H

#include <Arduino.h>
#include <sqlite3.h>
#include "database_manager.h"
#include <SD.h>

// ===== CALLBACK FUNCTIONS FOR PRINTING =====

// Bills callback
static int printBillCallback(void *data, int argc, char **argv, char **azColName) {
  Serial.print(atoi(argv[0])); Serial.print(F(" | "));
  Serial.print(argv[1]); Serial.print(F(" | "));
  Serial.print(atoi(argv[2])); Serial.print(F(" | "));
  Serial.print(atoi(argv[3])); Serial.print(F(" | "));
  Serial.print(argv[4]); Serial.print(F(" | "));
  Serial.print(argv[5]); Serial.print(F(" | "));
  Serial.print(atof(argv[6]), 2); Serial.print(F(" | "));
  Serial.print(atof(argv[7]), 2); Serial.print(F(" | "));
  Serial.print(atof(argv[8]), 2); Serial.print(F(" | "));
  Serial.print(atof(argv[9]), 2); Serial.print(F(" | "));
  Serial.print(argv[10]); Serial.print(F(" | "));
  Serial.println(argv[11]);
  return 0;
}

// Customers callback
static int printCustomerCallback(void *data, int argc, char **argv, char **azColName) {
  Serial.print(atoi(argv[0])); Serial.print(F(" | "));
  Serial.print(argv[1]); Serial.print(F(" | "));
  Serial.print(argv[2]); Serial.print(F(" | "));
  Serial.print(atoi(argv[3])); Serial.print(F(" | "));
  Serial.print(argv[4] ? argv[4] : "NULL"); Serial.print(F(" | "));
  Serial.print(argv[5]); Serial.print(F(" | "));
  Serial.print(atoi(argv[6])); Serial.print(F(" | "));
  Serial.print(argv[7]); Serial.print(F(" | "));
  Serial.println(argv[8] ? argv[8] : "N/A");
  return 0;
}

// Deductions callback
static int printDeductionCallback(void *data, int argc, char **argv, char **azColName) {
  Serial.print(atoi(argv[0])); Serial.print(F(" | "));
  Serial.print(argv[1]); Serial.print(F(" | "));
  Serial.print(argv[2]); Serial.print(F(" | "));
  Serial.print(atof(argv[3]), 2); Serial.print(F(" | "));
  Serial.println(argv[4]);
  return 0;
}

// Customer Types callback
static int printCustomerTypeCallback(void *data, int argc, char **argv, char **azColName) {
  Serial.print(atoi(argv[0])); Serial.print(F(" | "));
  Serial.print(argv[1]); Serial.print(F(" | "));
  Serial.print(atof(argv[2]), 2); Serial.print(F(" | "));
  Serial.print(atoi(argv[3])); Serial.print(F(" | "));
  Serial.print(atof(argv[4]), 2); Serial.print(F(" | "));
  Serial.println(argv[5]);
  return 0;
}

// Readings callback
static int printReadingCallback(void *data, int argc, char **argv, char **azColName) {
  Serial.print(atoi(argv[0])); Serial.print(F(" | "));
  Serial.print(atoi(argv[1])); Serial.print(F(" | "));
  Serial.print(argv[2]); Serial.print(F(" | "));
  Serial.print(atoi(argv[3])); Serial.print(F(" | "));
  Serial.print(atoi(argv[4])); Serial.print(F(" | "));
  Serial.print(atoi(argv[5])); Serial.print(F(" | "));
  Serial.println(argv[6]);
  return 0;
}

// Barangays callback
static int printBarangayCallback(void *data, int argc, char **argv, char **azColName) {
  Serial.print(atoi(argv[0])); Serial.print(F(" | "));
  Serial.print(argv[1]); Serial.print(F(" | "));
  Serial.print(argv[2]); Serial.print(F(" | "));
  Serial.print(atoi(argv[3])); Serial.print(F(" | "));
  Serial.println(argv[4]);
  return 0;
}

// Device Info callback
static int printDeviceInfoCallback(void *data, int argc, char **argv, char **azColName) {
  for (int i = 0; i < argc; i++) {
    Serial.print(argv[i] ? argv[i] : "NULL");
    if (i < argc - 1) Serial.print(F(" | "));
  }
  Serial.println();
  return 0;
}

// Bill Transactions callback
static int printBillTransactionCallback(void *data, int argc, char **argv, char **azColName) {
  Serial.print(atoi(argv[0])); Serial.print(F(" | "));
  Serial.print(atoi(argv[1])); Serial.print(F(" | "));
  Serial.print(argv[2]); Serial.print(F(" | "));
  Serial.print(argv[3]); Serial.print(F(" | "));
  Serial.print(argv[4]); Serial.print(F(" | "));
  Serial.print(argv[5]); Serial.print(F(" | "));
  Serial.print(atof(argv[6]), 2); Serial.print(F(" | "));
  Serial.print(atof(argv[7]), 2); Serial.print(F(" | "));
  Serial.print(atof(argv[8]), 2); Serial.print(F(" | "));
  Serial.print(argv[9]); Serial.print(F(" | "));
  Serial.print(argv[10]); Serial.print(F(" | "));
  Serial.print(atoi(argv[11])); Serial.print(F(" | "));
  Serial.println(argv[12] ? argv[12] : "NULL");
  return 0;
}

// ===== PRINT FUNCTIONS =====

// Function to print bills list
void printBillsList() {
  Serial.println(F("===== BILLS DATABASE ====="));
  Serial.println(F("ID | Ref Number    | CustID | ReadID | DeviceUID | Date       | Rate | Charges | Penalty | Total | Status | Created"));
  Serial.println(F("---|---------------|--------|--------|-----------|------------|------|---------|---------|-------|--------|--------"));
  const char *sql = "SELECT bill_id, reference_number, customer_id, reading_id, device_uid, bill_date, rate_per_m3, charges, penalty, total_due, status, created_at FROM bills;";
  sqlite3_exec(db, sql, printBillCallback, NULL, NULL);
  Serial.println(F("====================================================================================================="));
}

// Function to print customers list
void printCustomersList() {
  Serial.println(F("===== CUSTOMERS DATABASE ====="));
  Serial.println(F("ID | Account  | Name              | Type | Deduction | Address         | Prev Read | Status | Barangay"));
  Serial.println(F("---|----------|-------------------|------|----------|-----------------|----------|--------|---------"));
  const char *sql = "SELECT c.customer_id, c.account_no, c.customer_name, c.type_id, c.deduction_id, c.address, c.previous_reading, c.status, b.barangay FROM customers c LEFT JOIN barangay_sequence b ON c.brgy_id = b.brgy_id;";
  sqlite3_exec(db, sql, printCustomerCallback, NULL, NULL);
  Serial.println(F("================================"));
}

// Function to print deductions list
void printDeductionsList() {
  Serial.println(F("===== DEDUCTIONS DATABASE ====="));
  Serial.println(F("ID | Name          | Type      | Value | Created"));
  Serial.println(F("---|---------------|-----------|-------|--------"));
  const char *sql = "SELECT deduction_id, name, type, value, created_at FROM deductions;";
  sqlite3_exec(db, sql, printDeductionCallback, NULL, NULL);
  Serial.println(F("================================"));
}

// Function to print customer types list
void printCustomerTypesList() {
  Serial.println(F("===== CUSTOMER TYPES DATABASE ====="));
  Serial.println(F("ID | Name          | Rate/m3 | Min m3 | Min Charge | Created"));
  Serial.println(F("---|---------------|---------|--------|-----------|--------"));
  const char *sql = "SELECT type_id, type_name, rate_per_m3, min_m3, min_charge, created_at FROM customer_types;";
  sqlite3_exec(db, sql, printCustomerTypeCallback, NULL, NULL);
  Serial.println(F("====================================="));
}

// Function to print readings list
void printReadingsList() {
  Serial.println(F("===== READINGS DATABASE ====="));
  Serial.println(F("ID | CustID | DeviceUID | Prev | Curr | Usage | Reading At"));
  Serial.println(F("---|--------|-----------|------|------|-------|-----------"));
  const char *sql = "SELECT reading_id, customer_id, device_uid, previous_reading, current_reading, usage_m3, reading_at FROM readings;";
  sqlite3_exec(db, sql, printReadingCallback, NULL, NULL);
  Serial.println(F("========================================================"));
}

// Function to print barangays list
void printBarangaysList() {
  Serial.println(F("===== BARANGAYS DATABASE ====="));
  Serial.println(F("ID | Barangay    | Prefix | Next Num | Updated"));
  Serial.println(F("---|-------------|--------|----------|--------"));
  const char *sql = "SELECT brgy_id, barangay, prefix, next_number, updated_at FROM barangay_sequence;";
  sqlite3_exec(db, sql, printBarangayCallback, NULL, NULL);
  Serial.println(F("================================"));
}

// Forward declaration - actual implementation in main .ino file
extern void printDeviceInfoFromPSV();

// Function to print device info list - calls main .ino function
void printDeviceInfoList() {
  // Call the function in main .ino where SD was initialized
  // This avoids SD library access issues from included header files
  printDeviceInfoFromPSV();
}

// Function to print bill transactions list
void printBillTransactionsList() {
  Serial.println(F("===== BILL TRANSACTIONS DATABASE ====="));
  Serial.println(F("ID | BillID | RefNum | Type | Source | Amount | CashRec | Change | TransDate | PayMethod | ProcByDev | Notes"));
  Serial.println(F("---|--------|--------|------|--------|--------|---------|--------|-----------|----------|----------|------"));
  const char *sql = "SELECT bill_transaction_id, bill_id, bill_reference_number, type, source, amount, cash_received, change, transaction_date, payment_method, processed_by_device_uid, notes FROM bill_transactions;";
  sqlite3_exec(db, sql, printBillTransactionCallback, NULL, NULL);
  Serial.println(F("================================"));
}

// Function to display all database data via Serial
void displayAllDatabaseData() {
  Serial.println(F("=== DEVICE INFO DATABASE ==="));
  printDeviceInfoList();
  Serial.println();

  Serial.println(F("=== BARANGAYS DATABASE ==="));
  printBarangaysList();
  Serial.println();

  Serial.println(F("=== DEDUCTIONS DATABASE ==="));
  printDeductionsList();
  Serial.println();

  Serial.println(F("=== CUSTOMER TYPES DATABASE ==="));
  printCustomerTypesList();
  Serial.println();

  Serial.println(F("=== CUSTOMERS DATABASE ==="));
  printCustomersList();
  Serial.println();

  Serial.println(F("=== READINGS DATABASE ==="));
  printReadingsList();
  Serial.println();

  Serial.println(F("=== BILLS DATABASE ==="));
  printBillsList();
  Serial.println();

  Serial.println(F("=== BILL TRANSACTIONS DATABASE ==="));
  printBillTransactionsList();
  Serial.println();
}

#endif  // DATABASE_VIEWER_H