#ifndef BILL_DATABASE_H
#define BILL_DATABASE_H

#include "../configuration/config.h"
#include "customers_database.h"
#include "customer_type_database.h"
#include "deduction_database.h"
#include "device_info.h"
#include <time.h>
#include <SD.h>
#include <vector>
#include "database_manager.h"
#include <ArduinoJson.h>

// Forward declarations
float calculateDeductions(float baseAmount, unsigned long deductionId);
int getLastReadingIdForCustomer(int customerId);
void updateCustomerPreviousReading(int customerId, unsigned long newPreviousReading);
bool hasReadingThisMonth(int customerId);
void updateExistingReading(int readingId, unsigned long currentReading, unsigned long usage);

const int CURRENT_YEAR = 2026;

// ===== BILL DATA STRUCTURE =====
struct BillData {
  String customerName;
  String accountNo;
  String address;
  String collector;
  String dueDate;
  unsigned long prevReading;
  unsigned long currReading;
  float rate;
  float penalty;
  float subtotal;
  float deductions;
  float total;
  unsigned long usage;
  String customerType;
  float minCharge;
  unsigned long minM3;
  String deductionName;
  String readingDateTime;
};

// ===== BILL STRUCTURE FOR STORAGE =====
struct Bill {
  int bill_id;
  String reference_number;
  int customer_id;
  int reading_id;
  String device_uid;
  String bill_date;
  float rate_per_m3;
  float charges;
  float penalty;
  float total_due;
  String status;
  String created_at;
  String updated_at;
  bool synced;
  String last_sync;
};

// ===== BILL DATABASE =====
std::vector<Bill> bills;
BillData currentBill; // Global for current bill display
// Bulk insert mode: when true, avoid adding generated bills to the in-memory `bills` vector
bool g_bulkInsertMode = false;

static int loadBillCallback(void *data, int argc, char **argv, char **azColName) {
  Bill b;
  b.bill_id = atoi(argv[0]);
  b.reference_number = argv[1];
  b.customer_id = atoi(argv[2]);
  b.reading_id = atoi(argv[3]);
  b.device_uid = argv[4];
  b.bill_date = argv[5];
  b.rate_per_m3 = atof(argv[6]);
  b.charges = atof(argv[7]);
  b.penalty = atof(argv[8]);
  b.total_due = atof(argv[9]);
  b.status = argv[10];
  b.created_at = argv[11];
  b.updated_at = argv[12];
  b.synced = atoi(argv[13]);
  b.last_sync = argv[14] ? argv[14] : "";
  bills.push_back(b);
  return 0;
}

void loadBillsFromDB() {
  bills.clear();
  const char *sql = "SELECT bill_id, reference_number, customer_id, reading_id, device_uid, bill_date, rate_per_m3, charges, penalty, total_due, status, created_at, updated_at, synced, last_sync FROM bills;";
  sqlite3_exec(db, sql, loadBillCallback, NULL, NULL);
}

std::vector<Bill> getBillsChunk(int offset, int limit) {
  std::vector<Bill> chunk;
  char sql[256];
  sprintf(sql, "SELECT bill_id, reference_number, customer_id, reading_id, device_uid, bill_date, rate_per_m3, charges, penalty, total_due, status, created_at, updated_at, synced, last_sync FROM bills ORDER BY bill_id LIMIT %d OFFSET %d;", limit, offset);
  sqlite3_exec(db, sql, [](void *data, int argc, char **argv, char **azColName) -> int {
    std::vector<Bill>* chunk = static_cast<std::vector<Bill>*>(data);
    Bill b;
    b.bill_id = atoi(argv[0]);
    b.reference_number = argv[1];
    b.customer_id = atoi(argv[2]);
    b.reading_id = atoi(argv[3]);
    b.device_uid = argv[4];
    b.bill_date = argv[5];
    b.rate_per_m3 = atof(argv[6]);
    b.charges = atof(argv[7]);
    b.penalty = atof(argv[8]);
    b.total_due = atof(argv[9]);
    b.status = argv[10];
    b.created_at = argv[11];
    b.updated_at = argv[12];
    b.synced = atoi(argv[13]);
    b.last_sync = argv[14] ? argv[14] : "";
    chunk->push_back(b);
    return 0;
  }, &chunk, NULL);
  return chunk;
}

int getTotalBills() {
  int count = 0;
  sqlite3_exec(db, "SELECT COUNT(*) FROM bills;", [](void *data, int argc, char **argv, char **azColName) -> int {
    int* count = static_cast<int*>(data);
    *count = atoi(argv[0]);
    return 0;
  }, &count, NULL);
  return count;
}

void initBillsDatabase() {
  // loadBillsFromDB(); // Skip loading bills at boot to avoid heap exhaustion. Load on demand.
}

// ===== GENERATE BILL REFERENCE NUMBER =====
String generateBillReferenceNumber(String accountNo) {
  // Extract number part from account (e.g., "M-002" -> "002")
  String numberPart = "";
  for (char c : accountNo) {
    if (isdigit(c)) {
      numberPart += c;
    }
  }
  
  // Pad to 3 digits
  while (numberPart.length() < 3) {
    numberPart = "0" + numberPart;
  }
  
  static int nextBillId = 1;
  char ref[20];
  sprintf(ref, "REF%s%d%02d", numberPart.c_str(), CURRENT_YEAR, nextBillId++);
  return String(ref);
}

// ===== SAVE BILL TO DATABASE =====
bool saveBillToDB(Bill bill) {
  Serial.print(F("Saving bill for reading "));
  Serial.println(bill.reading_id);
  char sql[1024];
  String nowStr = getCurrentDateTimeString();
  sprintf(sql, "INSERT INTO bills (reference_number, customer_id, reading_id, device_uid, bill_date, rate_per_m3, charges, penalty, total_due, status, created_at, updated_at, synced, last_sync) VALUES ('%s', %d, %d, '%s', '%s', %f, %f, %f, %f, '%s', '%s', '%s', 0, NULL);",
          bill.reference_number.c_str(), bill.customer_id, bill.reading_id, bill.device_uid.c_str(), bill.bill_date.c_str(), bill.rate_per_m3, bill.charges, bill.penalty, bill.total_due, bill.status.c_str(), nowStr.c_str(), nowStr.c_str());
  // Serial.println(sql);  // Commented out to save heap memory
  int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
  Serial.print(F("Bill save result: "));
  Serial.println(rc == SQLITE_OK ? "OK" : "FAILED");
  return rc == SQLITE_OK;
}

// ===== FIND BILL BY ACCOUNT =====
int findBillByAccount(String accountNo) {
  int customerIndex = findCustomerByAccount(accountNo);
  if (customerIndex == -1) return -1;
  Customer* c = getCustomerAt(customerIndex);
  if (!c) return -1;
  int customer_id = c->customer_id;

  for (size_t i = 0; i < bills.size(); i++) {
    if (bills[i].customer_id == customer_id) {
      return i;
    }
  }
  return -1;
}

// ===== CALCULATE BILL AMOUNT =====
float calculateBillAmount(unsigned long customerTypeId, unsigned long deductionId) {
  int typeIndex = findCustomerTypeById(customerTypeId);
  if (typeIndex == -1) return 0.0;
  CustomerType* type = getCustomerTypeAt(typeIndex);
  if (!type) return 0.0;

  float baseAmount = type->min_charge; // Minimum charge

  // Apply deductions
  float deductionAmount = calculateDeductions(baseAmount, deductionId);

  return baseAmount - deductionAmount;
}

// ===== CALCULATE DEDUCTIONS =====
float calculateDeductions(float baseAmount, unsigned long deductionId) {
  if (deductionId == 0) return 0.0;

  int deductionIndex = findDeductionById(deductionId);
  if (deductionIndex == -1) return 0.0;
  Deduction* deduction = getDeductionAt(deductionIndex);
  if (!deduction) return 0.0;

  if (deduction->type == "percentage") {
    return baseAmount * (deduction->value / 100.0);
  } else if (deduction->type == "fixed") {
    return deduction->value;
  }
  return 0.0;
}

// ===== GET LAST READING ID FOR CUSTOMER =====
int getLastReadingIdForCustomer(int customerId) {
  String query = "SELECT reading_id FROM readings WHERE customer_id = " + String(customerId) + " ORDER BY reading_id DESC LIMIT 1;";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("SQL error: %s\n", sqlite3_errmsg(db));
    return 0;
  }
  
  int readingId = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    readingId = sqlite3_column_int(stmt, 0);
  }
  sqlite3_finalize(stmt);
  return readingId;
}

// ===== UPDATE CUSTOMER PREVIOUS READING =====
void updateCustomerPreviousReading(int customerId, unsigned long newPreviousReading) {
  String query = "UPDATE customers SET previous_reading = " + String(newPreviousReading) + " WHERE customer_id = " + String(customerId) + ";";
  sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
}

// ===== CHECK IF CUSTOMER HAS READING THIS MONTH =====
bool hasReadingThisMonth(int customerId) {
  String query = "SELECT COUNT(*) FROM readings WHERE customer_id = " + String(customerId) + ";";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("SQL error: %s\n", sqlite3_errmsg(db));
    return false;
  }
  
  bool hasReading = false;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    hasReading = sqlite3_column_int(stmt, 0) > 0;
  }
  sqlite3_finalize(stmt);
  return hasReading;
}

// ===== UPDATE EXISTING READING =====
void updateExistingReading(int readingId, unsigned long currentReading, unsigned long usage) {
  String nowStr = getCurrentDateTimeString();
  String query = "UPDATE readings SET current_reading = " + String(currentReading) + ", usage_m3 = " + String(usage) + ", updated_at = '" + nowStr + "' WHERE reading_id = " + String(readingId) + ";";
  sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
}

// ===== GET EXISTING READING ID THIS MONTH =====
int getExistingReadingIdThisMonth(int customerId) {
  String query = "SELECT reading_id FROM readings WHERE customer_id = " + String(customerId) + " ORDER BY reading_id DESC LIMIT 1;";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("SQL error: %s\n", sqlite3_errmsg(db));
    return 0;
  }
  
  int readingId = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    readingId = sqlite3_column_int(stmt, 0);
  }
  sqlite3_finalize(stmt);
  return readingId;
}

// ===== GET EXISTING READING DATA THIS MONTH =====
bool getExistingReadingDataThisMonth(int customerId, unsigned long& prevReading, unsigned long& currReading, unsigned long& usage) {
  String query = "SELECT previous_reading, current_reading, usage_m3 FROM readings WHERE customer_id = " + String(customerId) + " ORDER BY reading_id DESC LIMIT 1;";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("SQL error: %s\n", sqlite3_errmsg(db));
    return false;
  }
  
  bool found = false;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    prevReading = sqlite3_column_int(stmt, 0);
    currReading = sqlite3_column_int(stmt, 1);
    usage = sqlite3_column_int(stmt, 2);
    found = true;
  }
  sqlite3_finalize(stmt);
  return found;
}

// ===== UPDATE EXISTING BILL =====
void updateExistingBill(int customerId, int readingId, float charges, float totalDue, float rate) {
  Serial.print(F("Updating bill for customer "));
  Serial.print(customerId);
  Serial.print(F(", reading "));
  Serial.println(readingId);
  String nowStr = getCurrentDateTimeString();
  String query = "UPDATE bills SET charges = " + String(charges, 2) + ", total_due = " + String(totalDue, 2) + ", rate_per_m3 = " + String(rate, 2) + ", updated_at = '" + nowStr + "' WHERE customer_id = " + String(customerId) + " AND reading_id = " + String(readingId) + ";";
  // Serial.println(query);  // Commented out to save heap memory
  sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
}

// ===== GENERATE BILL FOR CUSTOMER =====
bool generateBillForCustomer(String accountNo, unsigned long currentReading) {
  int customerIndex = findCustomerByAccount(accountNo);
  if (customerIndex == -1) return false;
  Customer* customer = getCustomerAt(customerIndex);
  if (!customer) return false;

  // Save old previous reading for display
  unsigned long oldPreviousReading = customer->previous_reading;

  // Calculate usage - this will be recalculated if existing reading
  unsigned long usage = currentReading - customer->previous_reading;
  if (usage <= 0) return false; // Invalid usage

  // Get customer type
  int typeIndex = findCustomerTypeById(customer->type_id);
  if (typeIndex == -1) return false;
  CustomerType* customerType = getCustomerTypeAt(typeIndex);
  if (!customerType) return false;

  // Check if customer already has reading this month
  bool hasExistingReading = hasReadingThisMonth(customer->customer_id);
  int readingId;
  String readingAt = getCurrentDateTimeString(); // Use current RTC time

  unsigned long existingCurrReading = 0;
  unsigned long existingUsage = 0;

  if (hasExistingReading) {
    Serial.println(F("Updating existing reading..."));
    // Get existing reading data
    if (getExistingReadingDataThisMonth(customer->customer_id, oldPreviousReading, existingCurrReading, existingUsage)) {
      Serial.print(F("Existing prev: "));
      Serial.print(oldPreviousReading);
      Serial.print(F(", curr: "));
      Serial.println(existingCurrReading);
    }
    // Recalculate usage based on existing previous reading
    usage = currentReading - oldPreviousReading;
    Serial.print(F("New usage: "));
    Serial.println(usage);
    // Get reading ID first
    readingId = getExistingReadingIdThisMonth(customer->customer_id);
    Serial.print(F("Reading ID to update: "));
    Serial.println(readingId);
    // Update existing reading
    updateExistingReading(readingId, currentReading, usage);
  } else {
    Serial.println(F("Creating new reading..."));
    // Save new reading to database
    saveReadingToDB(customer->customer_id, customer->previous_reading, currentReading, usage, readingAt);
    // Get the reading ID (last inserted)
    readingId = getLastReadingIdForCustomer(customer->customer_id);
    Serial.print(F("New reading ID: "));
    Serial.println(readingId);
  }

  // Calculate charges based on usage
  float charges = 0.0;
  if (usage <= customerType->min_m3) {
    charges = customerType->min_charge;
  } else {
    charges = usage * customerType->rate_per_m3;
  }

  // Apply deductions
  float deductionAmount = calculateDeductions(charges, customer->deduction_id);
  float totalDue = charges - deductionAmount;

  // Update existing bill for this reading if it exists
  if (hasExistingReading) {
    updateExistingBill(customer->customer_id, readingId, charges, totalDue, customerType->rate_per_m3);
    Serial.println(F("Updated existing bill"));
  }

  // Update customer's previous reading
  updateCustomerPreviousReading(customer->customer_id, currentReading);

  // Update in-memory customer previous reading
  if (currentCustomer) {
    currentCustomer->previous_reading = currentReading;
  }
  if (!customer) return false;

  // Populate currentBill for display
  currentBill.customerName = customer->customer_name;
  currentBill.accountNo = customer->account_no;
  currentBill.address = customer->address;
  currentBill.prevReading = oldPreviousReading;
  currentBill.currReading = currentReading;
  currentBill.usage = usage;
  currentBill.rate = customerType->rate_per_m3;
  currentBill.minCharge = customerType->min_charge;
  currentBill.minM3 = customerType->min_m3;
  currentBill.customerType = customerType->type_name;
  currentBill.subtotal = charges;
  currentBill.deductions = deductionAmount;
  currentBill.total = totalDue;
  currentBill.penalty = 0.0;
  currentBill.subtotal = charges;
  currentBill.dueDate = "2026-02-15"; // 30 days from now
  currentBill.readingDateTime = readingAt;

  // Get deduction name if any
  if (customer->deduction_id > 0) {
    int dedIndex = findDeductionById(customer->deduction_id);
    if (dedIndex != -1) {
      Deduction* ded = getDeductionAt(dedIndex);
      if (ded) {
        currentBill.deductionName = ded->name;
      }
    }
  }

  // Only create new bill if this is a new reading
  if (!hasExistingReading) {
    Serial.println(F("Creating new bill..."));
    // Create bill record
    Bill bill;
    bill.reference_number = generateBillReferenceNumber(customer->account_no);
    bill.customer_id = customer->customer_id;
    bill.reading_id = readingId;
    bill.device_uid = getDeviceUID();
    bill.bill_date = getCurrentDateTimeString().substring(0,10);
    bill.rate_per_m3 = customerType->rate_per_m3;
    bill.charges = charges;
    bill.penalty = 0.0;
    bill.total_due = totalDue;
    bill.status = "Pending";

    // Save bill to DB
    if (saveBillToDB(bill)) {
      if (!g_bulkInsertMode) {
        bills.push_back(bill);
        Serial.println(F("New bill created successfully"));
      }
      return true;
    } else {
      if (!g_bulkInsertMode) {
        Serial.println(F("Failed to save new bill"));
      }
      return false;
    }
  }

  return true; // Successfully updated existing reading/bill
}

// ===== MARK ALL BILLS SYNCED =====
bool markAllBillsSynced() {
  String nowStr = getCurrentDateTimeString();
  char sql[128];
  sprintf(sql, "UPDATE bills SET synced = 1, last_sync = '%s', updated_at = '%s';", nowStr.c_str(), nowStr.c_str());
  int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
  return rc == SQLITE_OK;
}

#endif  // BILL_DATABASE_H
