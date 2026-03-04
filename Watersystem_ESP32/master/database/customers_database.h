#ifndef CUSTOMERS_DATABASE_H
#define CUSTOMERS_DATABASE_H

#include <SD.h>
#include "../configuration/config.h"
#include "device_info.h"
#include "sync_utils.h"
#include <sqlite3.h>
#include <vector>
#include "database_manager.h"

// ===== CUSTOMER DATA STRUCTURE =====
struct Customer {
  int customer_id;
  String account_no;
  int type_id;
  String customer_name;
  int deduction_id;
  int brgy_id;
  String address;
  unsigned long previous_reading;
  String status;
  String created_at;
  String updated_at;
};

// ===== CUSTOMER DATABASE =====
// std::vector<Customer> customers;  // Removed to save memory, load on demand
Customer* currentCustomer = nullptr;  // Single customer loaded on demand
float paymentAmount = 0.0;  // Payment amount for transactions
std::vector<Customer> allCustomers;  // For test data generation

// ===== GET CUSTOMER COUNT =====
int getCustomerCount() {
  const char* sql = "SELECT COUNT(*) FROM customers;";
  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.print(F("Failed to prepare getCustomerCount: "));
    Serial.println(sqlite3_errmsg(db));
    return 0;
  }
  rc = sqlite3_step(stmt);
  int count = 0;
  if (rc == SQLITE_ROW) {
    count = sqlite3_column_int(stmt, 0);
  }
  sqlite3_finalize(stmt);
  return count;
}

// ===== GET RANDOM ACCOUNT NO =====
static String getRandomAccountNo() {
  int count = getCustomerCount();
  if (count == 0) return "";
  int offset = random(0, count);
  const char* sql = "SELECT account_no FROM customers LIMIT 1 OFFSET ?;";
  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.print(F("Failed to prepare getRandomAccountNo: "));
    Serial.println(sqlite3_errmsg(db));
    return "";
  }
  sqlite3_bind_int(stmt, 1, offset);
  rc = sqlite3_step(stmt);
  String account = "";
  if (rc == SQLITE_ROW) {
    account = String((const char*)sqlite3_column_text(stmt, 0));
  }
  sqlite3_finalize(stmt);
  return account;
}

// ===== GET PREVIOUS READING FOR ACCOUNT =====
static unsigned long getPreviousReadingForAccount(String account) {
  const char* sql = "SELECT previous_reading FROM customers WHERE account_no = ?;";
  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    return 0;
  }
  sqlite3_bind_text(stmt, 1, account.c_str(), -1, SQLITE_STATIC);
  rc = sqlite3_step(stmt);
  unsigned long prev = 0;
  if (rc == SQLITE_ROW) {
    prev = (unsigned long)sqlite3_column_int64(stmt, 0);
  }
  sqlite3_finalize(stmt);
  return prev;
}

// ===== LOAD ALL CUSTOMERS =====
static int loadCustomerCallback(void *data, int argc, char **argv, char **azColName) {
  Customer c;
  c.customer_id = atoi(argv[0]);
  c.account_no = argv[1];
  c.customer_name = argv[2];
  c.address = argv[3];
  c.previous_reading = strtoul(argv[4], NULL, 10);
  c.status = argv[5];
  c.type_id = atoi(argv[6]);
  c.deduction_id = atoi(argv[7]);
  c.brgy_id = atoi(argv[8]);
  c.created_at = argv[9];
  c.updated_at = argv[10];
  allCustomers.push_back(c);
  // Yield to prevent watchdog reset during loading
  YIELD_WDT();
  return 0;
}

void loadAllCustomers() {
  allCustomers.clear();
  const char *sql = "SELECT customer_id, account_no, customer_name, address, previous_reading, status, type_id, deduction_id, brgy_id, created_at, updated_at FROM customers;";
  sqlite3_exec(db, sql, loadCustomerCallback, NULL, NULL);
}

void initCustomersDatabase() {
  Serial.printf("Heap free before load: %d\n", ESP.getFreeHeap());
  // loadCustomersFromDB(); // Skip loading customers at boot to avoid heap exhaustion. Load on demand.
  // loadAllCustomers();  // Load all for test data - commented out to avoid heap exhaustion
  #if WS_SERIAL_VERBOSE
    Serial.print(F("Loaded "));
    Serial.print(allCustomers.size());
    Serial.println(F(" customers from database."));
  #endif
  Serial.printf("Heap free after load: %d\n", ESP.getFreeHeap());
}

// ===== FIND CUSTOMER BY ACCOUNT =====
int findCustomerByAccount(String accountNumber) {
  // Query DB directly to avoid loading all customers
  const char* sql = "SELECT customer_id, account_no, type_id, customer_name, deduction_id, brgy_id, address, previous_reading, status, created_at, updated_at FROM customers WHERE account_no = ?;";
  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.print(F("Failed to prepare findCustomerByAccount: "));
    Serial.println(sqlite3_errmsg(db));
    return -1;
  }
  sqlite3_bind_text(stmt, 1, accountNumber.c_str(), -1, SQLITE_STATIC);
  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    // Free previous customer if exists
    if (currentCustomer) {
      delete currentCustomer;
    }
    currentCustomer = new Customer();
    currentCustomer->customer_id = sqlite3_column_int(stmt, 0);
    currentCustomer->account_no = String((const char*)sqlite3_column_text(stmt, 1));
    currentCustomer->type_id = sqlite3_column_int(stmt, 2);
    currentCustomer->customer_name = String((const char*)sqlite3_column_text(stmt, 3));
    currentCustomer->deduction_id = sqlite3_column_int(stmt, 4);
    currentCustomer->brgy_id = sqlite3_column_int(stmt, 5);
    currentCustomer->address = String((const char*)sqlite3_column_text(stmt, 6));
    currentCustomer->previous_reading = (unsigned long)sqlite3_column_int64(stmt, 7);
    currentCustomer->status = String((const char*)sqlite3_column_text(stmt, 8));
    currentCustomer->created_at = String((const char*)sqlite3_column_text(stmt, 9));
    currentCustomer->updated_at = String((const char*)sqlite3_column_text(stmt, 10));
    sqlite3_finalize(stmt);
    return 0;  // Return 0 as the index for currentCustomer
  }
  sqlite3_finalize(stmt);
  return -1;
}

// ===== GET CUSTOMER AT INDEX =====
Customer* getCustomerAt(int index) {
  if (index == 0 && currentCustomer) {
    return currentCustomer;
  }
  return nullptr;
}

// ===== UPSERT CUSTOMER FROM SYNC =====
bool upsertCustomerFromSync(const String& accountNo, const String& name, const String& address, unsigned long prev, const String& status, unsigned long typeId, unsigned long deductionId, unsigned long brgyId) {
  const char* sql = "INSERT OR REPLACE INTO customers (account_no, customer_name, address, previous_reading, status, type_id, deduction_id, brgy_id, created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, datetime('now'), datetime('now'));";

  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.print(F("SQLite prepare error: "));
    Serial.println(sqlite3_errmsg(db));
    return false;
  }

  sqlite3_bind_text(stmt, 1, accountNo.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, address.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 4, prev);
  sqlite3_bind_text(stmt, 5, status.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 6, typeId);

  if (deductionId == 0) {
    sqlite3_bind_null(stmt, 7);
  } else {
    sqlite3_bind_int64(stmt, 7, deductionId);
  }

  sqlite3_bind_int64(stmt, 8, brgyId);

  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (rc != SQLITE_DONE) {
    Serial.print(F("SQLite step error: "));
    Serial.println(sqlite3_errmsg(db));
    return false;
  }

  return true;
}

// ===== REMOVE CUSTOMER BY ACCOUNT =====
bool removeCustomerByAccount(const String& accountNumber) {
  char sql[256];
  sprintf(sql, "DELETE FROM customers WHERE account_no = '%s';", accountNumber.c_str());
  int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
  if (rc == SQLITE_OK) {
    return true;
  }
  return false;
}
#endif  // CUSTOMERS_DATABASE_H
