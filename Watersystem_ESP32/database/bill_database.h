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

// Forward declarations
float calculateDeductions(float baseAmount, unsigned long deductionId);
int getLastReadingIdForCustomer(int customerId);
void updateCustomerPreviousReading(int customerId, unsigned long newPreviousReading);
bool hasReadingThisMonth(int customerId);
void updateExistingReading(int customerId, unsigned long currentReading, unsigned long usage);

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
  String bill_no;
  String bill_date;
  float rate_per_m3;
  float charges;
  float penalty;
  float total_due;
  String status;
  String created_at;
  String updated_at;
};

// ===== BILL DATABASE =====
std::vector<Bill> bills;
BillData currentBill; // Global for current bill display

static int loadBillCallback(void *data, int argc, char **argv, char **azColName) {
  Bill b;
  b.bill_id = atoi(argv[0]);
  b.reference_number = argv[1];
  b.customer_id = atoi(argv[2]);
  b.reading_id = atoi(argv[3]);
  b.bill_no = argv[4];
  b.bill_date = argv[5];
  b.rate_per_m3 = atof(argv[6]);
  b.charges = atof(argv[7]);
  b.penalty = atof(argv[8]);
  b.total_due = atof(argv[9]);
  b.status = argv[10];
  b.created_at = argv[11];
  b.updated_at = argv[12];
  bills.push_back(b);
  return 0;
}

void loadBillsFromDB() {
  bills.clear();
  const char *sql = "SELECT bill_id, reference_number, customer_id, reading_id, bill_no, bill_date, rate_per_m3, charges, penalty, total_due, status, created_at, updated_at FROM bills;";
  sqlite3_exec(db, sql, loadBillCallback, NULL, NULL);
}

void initBillsDatabase() {
  loadBillsFromDB();
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
  char sql[1024];
  sprintf(sql, "INSERT INTO bills (reference_number, customer_id, reading_id, bill_no, bill_date, rate_per_m3, charges, penalty, total_due, status, created_at, updated_at) VALUES ('%s', %d, %d, '%s', '%s', %f, %f, %f, %f, '%s', datetime('now'), datetime('now'));",
          bill.reference_number.c_str(), bill.customer_id, bill.reading_id, bill.bill_no.c_str(), bill.bill_date.c_str(), bill.rate_per_m3, bill.charges, bill.penalty, bill.total_due, bill.status.c_str());
  int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
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
  String query = "SELECT COUNT(*) FROM readings WHERE customer_id = " + String(customerId) + " AND strftime('%Y-%m', reading_at) = strftime('%Y-%m', 'now');";
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
void updateExistingReading(int customerId, unsigned long currentReading, unsigned long usage) {
  String query = "UPDATE readings SET current_reading = " + String(currentReading) + ", usage_m3 = " + String(usage) + ", updated_at = datetime('now') WHERE customer_id = " + String(customerId) + " AND strftime('%Y-%m', reading_at) = strftime('%Y-%m', 'now');";
  sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
}

// ===== UPDATE EXISTING BILL =====
void updateExistingBill(int customerId, int readingId, float charges, float totalDue, float rate) {
  String query = "UPDATE bills SET charges = " + String(charges, 2) + ", total_due = " + String(totalDue, 2) + ", rate_per_m3 = " + String(rate, 2) + ", updated_at = datetime('now') WHERE customer_id = " + String(customerId) + " AND reading_id = " + String(readingId) + ";";
  sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
}

// ===== GENERATE BILL FOR CUSTOMER =====
bool generateBillForCustomer(String accountNo, unsigned long currentReading) {
  int customerIndex = findCustomerByAccount(accountNo);
  if (customerIndex == -1) return false;
  Customer* customer = getCustomerAt(customerIndex);
  if (!customer) return false;

  // Calculate usage
  unsigned long usage = currentReading - customer->previous_reading;
  if (usage <= 0) return false; // Invalid usage

  // Get customer type
  int typeIndex = findCustomerTypeById(customer->type_id);
  if (typeIndex == -1) return false;
  CustomerType* customerType = getCustomerTypeAt(typeIndex);
  if (!customerType) return false;

  // Calculate charges
  float charges = 0.0;
  if (usage <= customerType->min_m3) {
    charges = customerType->min_charge;
  } else {
    charges = usage * customerType->rate_per_m3;
  }

  // Apply deductions
  float deductionAmount = calculateDeductions(charges, customer->deduction_id);
  float totalDue = charges - deductionAmount;

  // Check if customer already has reading this month
  bool hasExistingReading = hasReadingThisMonth(customer->customer_id);
  int readingId;
  String readingAt = "2026-01-19 14:00:00"; // Current timestamp

  if (hasExistingReading) {
    // Update existing reading
    updateExistingReading(customer->customer_id, currentReading, usage);
    readingId = getLastReadingIdForCustomer(customer->customer_id);
    // Update existing bill
    updateExistingBill(customer->customer_id, readingId, charges, totalDue, customerType->rate_per_m3);
  } else {
    // Save new reading to database
    saveReadingToDB(customer->customer_id, customer->previous_reading, currentReading, usage, readingAt);
    // Get the reading ID (last inserted)
    readingId = getLastReadingIdForCustomer(customer->customer_id);
  }

  // Update customer's previous reading
  updateCustomerPreviousReading(customer->customer_id, currentReading);

  // Populate currentBill for display
  currentBill.customerName = customer->customer_name;
  currentBill.accountNo = customer->account_no;
  currentBill.address = customer->address;
  currentBill.prevReading = customer->previous_reading;
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
    // Create bill record
    Bill bill;
    bill.reference_number = generateBillReferenceNumber(customer->account_no);
    bill.customer_id = customer->customer_id;
    bill.reading_id = readingId;
    bill.bill_no = "BILL-" + String(CURRENT_YEAR);
    bill.bill_date = "2026-01-19";
    bill.rate_per_m3 = customerType->rate_per_m3;
    bill.charges = charges;
    bill.penalty = 0.0;
    bill.total_due = totalDue;
    bill.status = "Pending";

    // Save bill to DB
    if (saveBillToDB(bill)) {
      bills.push_back(bill);
      return true;
    }
    return false;
  }

  return true; // Successfully updated existing reading/bill
}

#endif  // BILL_DATABASE_H
