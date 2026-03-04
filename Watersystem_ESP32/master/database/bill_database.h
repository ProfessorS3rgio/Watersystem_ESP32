#ifndef BILL_DATABASE_H
#define BILL_DATABASE_H

#include "../configuration/config.h"
#include "customers_database.h"
#include "customer_type_database.h"
#include "deduction_database.h"
#include "device_info.h"
#include "settings_database.h"
#include <time.h>
#include <SD.h>
#include <vector>
#include "database_manager.h"
#include <ArduinoJson.h>

static int getBillDueDaysSetting() {
  Setting* s = getSettings();
  if (s && s->bill_due_days > 0 && s->bill_due_days <= 365) return s->bill_due_days;
  return 5;
}

static int getDisconnectionDaysSetting() {
  Setting* s = getSettings();
  if (s && s->disconnection_days > 0 && s->disconnection_days <= 365) return s->disconnection_days;
  return 8;
}

// Forward declarations
float calculateDeductions(float baseAmount, unsigned long deductionId);
int getLastReadingIdForCustomer(int customerId);
void updateCustomerPreviousReading(int customerId, unsigned long newPreviousReading);
bool hasReadingThisMonth(int customerId);
void updateExistingReading(int readingId, unsigned long currentReading, unsigned long usage);

static int getBuildYear() {
  // __DATE__ format: "Mmm dd yyyy"
  const char *buildDate = __DATE__;
  int len = strlen(buildDate);
  if (len >= 4) {
    int year = atoi(buildDate + (len - 4));
    if (year > 0) return year;
  }
  return 0;
}

static int getCurrentYearFromRTC() {
  // `getCurrentDateTimeString()` is expected to be RTC-backed.
  String dt = getCurrentDateTimeString();
  if (dt.length() >= 4) {
    int year = dt.substring(0, 4).toInt();
    if (year >= 2000 && year <= 2100) return year;
  }

  int buildYear = getBuildYear();
  if (buildYear > 0) {
    Serial.println(F("RTC year invalid; falling back to build year."));
    return buildYear;
  }

  Serial.println(F("RTC year invalid; falling back to year 0."));
  return 0;
}

static int getNextBillReferenceSequence(int year) {
  if (!db) return 1;

  int seq = 1;
  char *errMsg = nullptr;
  // If we're already inside a transaction (e.g., bulk test-data generation),
  // starting a new one will fail and would cause the sequence to always return 1.
  // In that case, just do the read+increment within the existing transaction.
  bool ownsTxn = (sqlite3_get_autocommit(db) != 0);
  int rc = SQLITE_OK;
  if (ownsTxn) {
    rc = sqlite3_exec(db, "BEGIN IMMEDIATE;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
      if (errMsg) sqlite3_free(errMsg);
      return 1;
    }
  }

  // Ensure row exists for this year
  {
    const char *insSql = "INSERT OR IGNORE INTO bill_reference_sequence(year, next_number) VALUES(?, 1);";
    sqlite3_stmt *stmt = nullptr;
    rc = sqlite3_prepare_v2(db, insSql, -1, &stmt, nullptr);
    if (rc == SQLITE_OK) {
      sqlite3_bind_int(stmt, 1, year);
      (void)sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    if (rc != SQLITE_OK) {
      if (ownsTxn) (void)sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
      return 1;
    }
  }

  // Read current sequence
  {
    const char *selSql = "SELECT next_number FROM bill_reference_sequence WHERE year = ?;";
    sqlite3_stmt *stmt = nullptr;
    rc = sqlite3_prepare_v2(db, selSql, -1, &stmt, nullptr);
    if (rc == SQLITE_OK) {
      sqlite3_bind_int(stmt, 1, year);
      if (sqlite3_step(stmt) == SQLITE_ROW) {
        seq = sqlite3_column_int(stmt, 0);
      }
    }
    sqlite3_finalize(stmt);
    if (rc != SQLITE_OK) {
      if (ownsTxn) (void)sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
      return 1;
    }
  }

  // Increment for next time
  {
    const char *updSql = "UPDATE bill_reference_sequence SET next_number = next_number + 1 WHERE year = ?;";
    sqlite3_stmt *stmt = nullptr;
    rc = sqlite3_prepare_v2(db, updSql, -1, &stmt, nullptr);
    if (rc == SQLITE_OK) {
      sqlite3_bind_int(stmt, 1, year);
      (void)sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    if (rc != SQLITE_OK) {
      if (ownsTxn) (void)sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
      return 1;
    }
  }

  if (ownsTxn) (void)sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
  return seq;
}

// ===== BILL DATA STRUCTURE =====
struct BillData {
  String customerName;
  String accountNo;
  String address;
  String collector;
  String dueDate;
  String billDate;  // Added for disconnection date calculation
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
  String refNumber;
  String status;
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
  String customer_account_number;
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
  b.customer_account_number = argv[15] ? argv[15] : "";
  bills.push_back(b);
  return 0;
}

void loadBillsFromDB() {
  bills.clear();
  const char *sql = "SELECT bill_id, reference_number, customer_id, reading_id, device_uid, bill_date, rate_per_m3, charges, penalty, total_due, status, created_at, updated_at, synced, last_sync, customer_account_number FROM bills;";
  sqlite3_exec(db, sql, loadBillCallback, NULL, NULL);
}

std::vector<Bill> getBillsChunk(int offset, int limit) {
  std::vector<Bill> chunk;
  char sql[256];
  sprintf(sql, "SELECT bill_id, reference_number, customer_id, reading_id, device_uid, bill_date, rate_per_m3, charges, penalty, total_due, status, created_at, updated_at, synced, last_sync, customer_account_number FROM bills ORDER BY bill_id LIMIT %d OFFSET %d;", limit, offset);
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
    b.customer_account_number = argv[15] ? argv[15] : "";
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
  
  int year = getCurrentYearFromRTC();
  int seq = getNextBillReferenceSequence(year);

  char ref[24];
  sprintf(ref, "REF%s%d%02d", numberPart.c_str(), year, seq);
  return String(ref);
}

// ===== CALCULATE DUE DATE =====
String calculateDueDate(String billDate, int daysToAdd) {
  // billDate format: "YYYY-MM-DD"
  if (billDate.length() != 10) return billDate; // Invalid format
  
  int year = billDate.substring(0,4).toInt();
  int month = billDate.substring(5,7).toInt();
  int day = billDate.substring(8,10).toInt();
  
  // Add days
  day += daysToAdd;
  
  // Handle month/day overflow
  int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  
  // Check for leap year
  if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) {
    daysInMonth[1] = 29;
  }
  
  while (day > daysInMonth[month - 1]) {
    day -= daysInMonth[month - 1];
    month++;
    if (month > 12) {
      month = 1;
      year++;
    }
  }
  
  char dueDate[11];
  sprintf(dueDate, "%04d-%02d-%02d", year, month, day);
  return String(dueDate);
}

// ===== SAVE BILL TO DATABASE =====
bool saveBillToDB(Bill bill) {
  Serial.print(F("Saving bill for reading "));
  Serial.println(bill.reading_id);
  char sql[1024];
  String nowStr = getCurrentDateTimeString();
  sprintf(sql, "INSERT INTO bills (reference_number, customer_id, reading_id, device_uid, bill_date, rate_per_m3, charges, penalty, total_due, status, created_at, updated_at, synced, last_sync, customer_account_number) VALUES ('%s', %d, %d, '%s', '%s', %f, %f, %f, %f, '%s', '%s', '%s', 0, NULL, '%s');",
          bill.reference_number.c_str(), bill.customer_id, bill.reading_id, bill.device_uid.c_str(), bill.bill_date.c_str(), bill.rate_per_m3, bill.charges, bill.penalty, bill.total_due, bill.status.c_str(), nowStr.c_str(), nowStr.c_str(), bill.customer_account_number.c_str());
  // Serial.println(sql);  // Commented out to save heap memory
  int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
  Serial.print(F("Bill save result: "));
  Serial.println(rc == SQLITE_OK ? "OK" : "FAILED");
  return rc == SQLITE_OK;
}

// ===== FIND BILL BY ACCOUNT =====
int findBillByAccount(String accountNo) {
  int billIndex = -1;
  char sql[256];
  sprintf(sql, "SELECT bill_id FROM bills WHERE customer_account_number = '%s' ORDER BY bill_date DESC LIMIT 1;", accountNo.c_str());
  sqlite3_exec(db, sql, [](void *data, int argc, char **argv, char **azColName) -> int {
    if (argc > 0 && argv[0]) {
      *(int*)data = atoi(argv[0]);
    }
    return 0;
  }, &billIndex, NULL);
  return billIndex;
}

// ===== CHARGE COMPUTATION HELPERS =====
// Applies common billing rules.  The "Officers" class uses a special
// scheme where once usage exceeds the minimum threshold the bill is
// composed of the fixed minimum charge plus the normal per‑m3 rate for
// *all* cubic meters.  (The old logic effectively ignored the minimum
// when usage was above the threshold, which produced incorrect results
// on the meter device.)
static float computeCharges(CustomerType* type, unsigned long usage) {
  if (!type) return 0.0f;
  // officers get the minimum fee plus rate for every m3 when they go over
  // Case-insensitive comparison so "officers", "Officers", "OFFICERS"
  // etc. all map to the special rule.  Trim any accidental whitespace too.
  String tn = type->type_name;
  tn.trim();
  tn.toLowerCase();
  if (tn == "officers") {
    if (usage <= type->min_m3) {
      return type->min_charge;
    }
    // note: do not subtract min_m3 – the requirement is to add the fixed
    // minimum 100‑peso charge and then apply the cubic rate to the entire
    // usage amount.
    return type->min_charge + (usage * type->rate_per_m3);
  }

  // default behaviour for all other customer types
  if (usage <= type->min_m3) {
    return type->min_charge;
  }
  return usage * type->rate_per_m3;
}

// ===== CALCULATE BILL AMOUNT =====
float calculateBillAmount(unsigned long customerTypeId, unsigned long deductionId) {
  int typeIndex = findCustomerTypeById(customerTypeId);
  if (typeIndex == -1) return 0.0;
  CustomerType* type = getCustomerTypeAt(typeIndex);
  if (!type) return 0.0;

  // The original implementation only returned the minimum charge and
  // ignored usage completely.  It is unclear where this helper is used
  // in the firmware, but to stay consistent we keep the old behaviour
  // for now (usage information isn’t available here).
  float baseAmount = type->min_charge; // Minimum charge

  // Apply deductions
  float deductionAmount = calculateDeductions(baseAmount, deductionId);

  return baseAmount - deductionAmount;
}

// ===== GET BILL DATE BY CUSTOMER AND READING =====
String getBillDateByCustomerAndReading(int customerId, int readingId) {
  String billDate = "";
  char sql[256];
  sprintf(sql, "SELECT bill_date FROM bills WHERE customer_id = %d AND reading_id = %d;", customerId, readingId);
  sqlite3_exec(db, sql, [](void *data, int argc, char **argv, char **azColName) -> int {
    String* billDate = static_cast<String*>(data);
    if (argc > 0 && argv[0]) {
      *billDate = argv[0];
    }
    return 0;
  }, &billDate, NULL);
  return billDate;
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
  String query = "UPDATE readings SET current_reading = " + String(currentReading) + ", usage_m3 = " + String(usage) + ", synced = 0, last_sync = NULL, updated_at = '" + nowStr + "' WHERE reading_id = " + String(readingId) + ";";
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
  String query = "UPDATE bills SET charges = " + String(charges, 2) + ", total_due = " + String(totalDue, 2) + ", rate_per_m3 = " + String(rate, 2) + ", synced = 0, last_sync = NULL, updated_at = '" + nowStr + "' WHERE customer_id = " + String(customerId) + " AND reading_id = " + String(readingId) + ";";
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
    if (usage <= 0) return false; // Invalid usage for existing reading
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
    saveReadingToDB(customer->customer_id, customer->previous_reading, currentReading, usage, readingAt, customer->account_no);
    // Get the reading ID (last inserted)
    readingId = getLastReadingIdForCustomer(customer->customer_id);
    Serial.print(F("New reading ID: "));
    Serial.println(readingId);
  }

  // Calculate charges based on usage (includes special Officers logic)
  float charges = computeCharges(customerType, usage);

  // Apply deductions
  float deductionAmount = calculateDeductions(charges, customer->deduction_id);
  float totalDue = charges - deductionAmount;

  // Update existing bill for this reading if it exists
  if (hasExistingReading) {
    updateExistingBill(customer->customer_id, readingId, charges, totalDue, customerType->rate_per_m3);
    Serial.println(F("Updated existing bill"));
    
    // Populate currentBill for display/printing
    String existingBillDate = getBillDateByCustomerAndReading(customer->customer_id, readingId);
    if (existingBillDate == "") {
      existingBillDate = getCurrentDateTimeString().substring(0,10); // Fallback
    }
    currentBill.customerName = customer->customer_name;
    currentBill.accountNo = customer->account_no;
    currentBill.address = customer->address;
    currentBill.collector = COLLECTOR_NAME_VALUE;
    currentBill.dueDate = calculateDueDate(existingBillDate, getBillDueDaysSetting());
    currentBill.billDate = existingBillDate;
    currentBill.prevReading = oldPreviousReading;
    currentBill.currReading = currentReading;
    currentBill.rate = customerType->rate_per_m3;
    currentBill.penalty = 0.0; // Existing bills don't have penalty in update
    currentBill.subtotal = charges;
    currentBill.deductions = deductionAmount;
    currentBill.total = totalDue;
    currentBill.usage = usage;
    currentBill.customerType = customerType->type_name;
    currentBill.minCharge = customerType->min_charge;
    currentBill.minM3 = customerType->min_m3;
    currentBill.readingDateTime = readingAt;
    // For existing bills, we need to get the reference number
    String refNum = "";
    char sql[256];
    sprintf(sql, "SELECT reference_number FROM bills WHERE customer_id = %d AND reading_id = %d;", customer->customer_id, readingId);
    sqlite3_exec(db, sql, [](void *data, int argc, char **argv, char **azColName) -> int {
      String* refNum = static_cast<String*>(data);
      if (argc > 0 && argv[0]) {
        *refNum = argv[0];
      }
      return 0;
    }, &refNum, NULL);
    currentBill.refNumber = refNum;
  }

  // Update customer's previous reading
  updateCustomerPreviousReading(customer->customer_id, currentReading);

  // Update in-memory customer previous reading
  if (currentCustomer) {
    currentCustomer->previous_reading = currentReading;
  }
  if (!customer) return false;

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
    bill.customer_account_number = customer->account_no;
    
    // Calculate due date based on settings
    String dueDateStr = calculateDueDate(bill.bill_date, getBillDueDaysSetting());

    // Save bill to DB
    if (saveBillToDB(bill)) {
      if (!g_bulkInsertMode) {
        bills.push_back(bill);
        Serial.println(F("New bill created successfully"));
        
        // Populate currentBill for display/printing
        currentBill.customerName = customer->customer_name;
        currentBill.accountNo = customer->account_no;
        currentBill.address = customer->address;
        currentBill.collector = COLLECTOR_NAME_VALUE;
        currentBill.dueDate = dueDateStr;
        currentBill.billDate = bill.bill_date;
        currentBill.prevReading = oldPreviousReading;
        currentBill.currReading = currentReading;
        currentBill.rate = customerType->rate_per_m3;
        currentBill.penalty = bill.penalty;
        currentBill.subtotal = bill.charges;
        currentBill.deductions = deductionAmount;
        currentBill.total = bill.total_due;
        currentBill.usage = usage;
        currentBill.customerType = customerType->type_name;
        currentBill.minCharge = customerType->min_charge;
        currentBill.minM3 = customerType->min_m3;
        currentBill.readingDateTime = readingAt;
        currentBill.refNumber = bill.reference_number;
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

bool getBillForCustomer(String accountNo) {
  int customerIndex = findCustomerByAccount(accountNo);
  if (customerIndex == -1) return false;
  Customer* customer = getCustomerAt(customerIndex);
  if (!customer) return false;

  // Query for the latest bill for this customer
  const char* sql =
      "SELECT "
      "  b.reference_number, b.bill_date, b.rate_per_m3, b.charges, b.penalty, b.total_due, b.status, "
      "  r.previous_reading, r.current_reading, r.usage_m3, "
      "  ct.type_name, ct.min_charge, ct.min_m3, "
      "  d.name, d.type, d.value, "
      "  r.reading_at "
      "FROM bills b "
      "LEFT JOIN readings r ON b.reading_id = r.reading_id "
      "LEFT JOIN customers c ON b.customer_id = c.customer_id "
      "LEFT JOIN customer_types ct ON c.type_id = ct.type_id "
      "LEFT JOIN deductions d ON c.deduction_id = d.deduction_id "
      "WHERE b.customer_account_number = ? "
      "ORDER BY b.bill_date DESC LIMIT 1;";

  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.print(F("Failed to prepare statement: "));
    Serial.println(sqlite3_errmsg(db));
    return false;
  }

  sqlite3_bind_text(stmt, 1, accountNo.c_str(), -1, SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    // Set currentBill
    currentBill.customerName = customer->customer_name;
    currentBill.accountNo = customer->account_no;
    currentBill.address = customer->address;
    // Always populate collector name for downstream printing.
    currentBill.collector = COLLECTOR_NAME_VALUE;
    currentBill.refNumber = (const char*)sqlite3_column_text(stmt, 0);
    currentBill.billDate = (const char*)sqlite3_column_text(stmt, 1);
    currentBill.dueDate = calculateDueDate(currentBill.billDate, getBillDueDaysSetting());
    currentBill.rate = sqlite3_column_double(stmt, 2);
    currentBill.subtotal = sqlite3_column_double(stmt, 3);
    currentBill.penalty = sqlite3_column_double(stmt, 4);
    currentBill.total = sqlite3_column_double(stmt, 5);
    currentBill.status = (const char*)sqlite3_column_text(stmt, 6);
    currentBill.prevReading = sqlite3_column_int(stmt, 7);
    currentBill.currReading = sqlite3_column_int(stmt, 8);
    currentBill.usage = sqlite3_column_int(stmt, 9);
    currentBill.customerType = (const char*)sqlite3_column_text(stmt, 10);
    currentBill.minCharge = sqlite3_column_double(stmt, 11);
    currentBill.minM3 = sqlite3_column_int(stmt, 12);
    currentBill.deductionName = (const char*)sqlite3_column_text(stmt, 13);
    // Stored total_due already reflects discounts; compute for display.
    float computedDeduction = (currentBill.subtotal + currentBill.penalty) - currentBill.total;
    currentBill.deductions = (computedDeduction > 0.0f) ? computedDeduction : 0.0f;
    currentBill.readingDateTime = (const char*)sqlite3_column_text(stmt, 16);

    sqlite3_finalize(stmt);
    return true;
  } else {
    sqlite3_finalize(stmt);
    return false;
  }
}

#endif  // BILL_DATABASE_H
