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

// ===== BARANGAY SEQUENCE STRUCTURE =====
struct BarangaySeq {
  int brgy_id;
  String barangay;
  String prefix;
  int next_number;
  String updated_at;
};

// ===== CUSTOMER DATABASE =====
std::vector<Customer> customers;
std::vector<BarangaySeq> barangays;

static int loadCustomerCallback(void *data, int argc, char **argv, char **azColName) {
  Customer c;
  // Defensive: check argc and NULL argv pointers to avoid crashes on malformed rows
  c.customer_id = (argc > 0 && argv[0]) ? atoi(argv[0]) : 0;
  c.account_no = (argc > 1 && argv[1]) ? String(argv[1]) : String("");
  c.type_id = (argc > 2 && argv[2]) ? atoi(argv[2]) : 1;
  c.customer_name = (argc > 3 && argv[3]) ? String(argv[3]) : String("");
  c.deduction_id = (argc > 4 && argv[4]) ? atoi(argv[4]) : 0;  // 0 means no deduction
  c.brgy_id = (argc > 5 && argv[5]) ? atoi(argv[5]) : 1;
  c.address = (argc > 6 && argv[6]) ? String(argv[6]) : String("");
  c.previous_reading = (argc > 7 && argv[7]) ? strtoul(argv[7], NULL, 10) : 0UL;
  c.status = (argc > 8 && argv[8]) ? String(argv[8]) : String("active");
  c.created_at = (argc > 9 && argv[9]) ? String(argv[9]) : String("");
  c.updated_at = (argc > 10 && argv[10]) ? String(argv[10]) : String("");
  customers.push_back(c);
  return 0;
}

static int loadBarangayCallback(void *data, int argc, char **argv, char **azColName) {
  BarangaySeq b;
  b.brgy_id = atoi(argv[0]);
  b.barangay = argv[1];
  b.prefix = argv[2];
  b.next_number = atoi(argv[3]);
  b.updated_at = argv[4];
  barangays.push_back(b);
  return 0;
}

void loadCustomersFromDB() {
  customers.clear();
  // Reserve capacity to reduce repeated reallocations when loading many rows.
  int count = 0;
  sqlite3_stmt* stmt = NULL;
  const char* countSql = "SELECT COUNT(*) FROM customers;";
  if (sqlite3_prepare_v2(db, countSql, -1, &stmt, NULL) == SQLITE_OK) {
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
  }
  if (count > 0) customers.reserve((size_t)count + 8);

  // Use a prepared statement and step through rows to have better control
  const char *sql = "SELECT customer_id, account_no, type_id, customer_name, deduction_id, brgy_id, address, previous_reading, status, created_at, updated_at FROM customers;";
  sqlite3_stmt* s = NULL;
  int rc = sqlite3_prepare_v2(db, sql, -1, &s, NULL);
  if (rc != SQLITE_OK) {
    Serial.print(F("SQLite prepare failed in loadCustomersFromDB: "));
    Serial.println(sqlite3_errmsg(db));
    return;
  }

  int rowIndex = 0;
  while ((rc = sqlite3_step(s)) == SQLITE_ROW) {
    // If heap dangerously low, abort loading to avoid crash. Caller can retry later.
    if (ESP.getFreeHeap() < 30000) {
      Serial.println(F("Warning: low heap during customer load; stopping early to avoid abort."));
      break;
    }

    Customer c;
    // Read columns safely
    const unsigned char* col0 = sqlite3_column_text(s, 0);
    const unsigned char* col1 = sqlite3_column_text(s, 1);
    const unsigned char* col2 = sqlite3_column_text(s, 2);
    const unsigned char* col3 = sqlite3_column_text(s, 3);
    const unsigned char* col4 = sqlite3_column_text(s, 4);
    const unsigned char* col5 = sqlite3_column_text(s, 5);
    const unsigned char* col6 = sqlite3_column_text(s, 6);
    const unsigned char* col7 = sqlite3_column_text(s, 7);
    const unsigned char* col8 = sqlite3_column_text(s, 8);
    const unsigned char* col9 = sqlite3_column_text(s, 9);
    const unsigned char* col10 = sqlite3_column_text(s, 10);

    c.customer_id = col0 ? atoi((const char*)col0) : 0;
    c.account_no = col1 ? String((const char*)col1) : String("");
    c.type_id = col2 ? atoi((const char*)col2) : 1;
    c.customer_name = col3 ? String((const char*)col3) : String("");
    c.deduction_id = col4 ? atoi((const char*)col4) : 0;
    c.brgy_id = col5 ? atoi((const char*)col5) : 1;
    c.address = col6 ? String((const char*)col6) : String("");
    c.previous_reading = col7 ? strtoul((const char*)col7, NULL, 10) : 0UL;
    c.status = col8 ? String((const char*)col8) : String("active");
    c.created_at = col9 ? String((const char*)col9) : String("");
    c.updated_at = col10 ? String((const char*)col10) : String("");

    customers.push_back(c);
    rowIndex++;

    // Log progress occasionally
    if ((rowIndex % 100) == 0) {
      Serial.print(F("Loaded customers: "));
      Serial.println(rowIndex);
      Serial.printf("Heap free during load: %d\n", ESP.getFreeHeap());
    }
  }

  if (count > (int)customers.size()) {
    Serial.printf("[WARN] Only loaded %d of %d customers due to heap limits.\n", customers.size(), count);
  }
  sqlite3_finalize(s);
}

void loadBarangaysFromDB() {
  barangays.clear();
  const char *sql = "SELECT brgy_id, barangay, prefix, next_number, updated_at FROM barangay_sequence;";
  sqlite3_exec(db, sql, loadBarangayCallback, NULL, NULL);
}

void initCustomersDatabase() {
  Serial.printf("Heap free before load: %d\n", ESP.getFreeHeap());
  // loadCustomersFromDB(); // Skip loading customers at boot to avoid heap exhaustion. Load on demand.
  loadBarangaysFromDB();
  // If barangays empty, insert defaults
  if (barangays.empty()) {
    const char *inserts[] = {
      "INSERT INTO barangay_sequence (barangay, prefix, next_number, updated_at) VALUES ('Dona Josefa', 'D', 1, datetime('now'));",
      "INSERT INTO barangay_sequence (barangay, prefix, next_number, updated_at) VALUES ('Makilas', 'M', 1, datetime('now'));",
      "INSERT INTO barangay_sequence (barangay, prefix, next_number, updated_at) VALUES ('Buluan', 'B', 1, datetime('now'));",
      "INSERT INTO barangay_sequence (barangay, prefix, next_number, updated_at) VALUES ('Caparan', 'C', 1, datetime('now'));"
    };
    for (auto sql : inserts) {
      sqlite3_exec(db, sql, NULL, NULL, NULL);
    }
    loadBarangaysFromDB();
  }
  #if WS_SERIAL_VERBOSE
    Serial.print(F("Loaded "));
    Serial.print(customers.size());
    Serial.println(F(" customers from database."));
  #endif
  Serial.printf("Heap free after load: %d\n", ESP.getFreeHeap());
}

String generateAccountNumber(String barangayName) {
  for (auto &b : barangays) {
    if (b.barangay == barangayName) {
      String num = String(b.next_number);
      while (num.length() < 3) num = "0" + num;
      String account = b.prefix + "-" + num;
      b.next_number++;
      char sql[256];
      sprintf(sql, "UPDATE barangay_sequence SET next_number = %d, updated_at = datetime('now') WHERE brgy_id = %d;", b.next_number, b.brgy_id);
      sqlite3_exec(db, sql, NULL, NULL, NULL);
      return account;
    }
  }
  return "";
}

String getBarangayName(int brgy_id) {
  for (const auto& b : barangays) {
    if (b.brgy_id == brgy_id) return b.barangay;
  }
  return "";
}

// ===== FIND CUSTOMER BY ACCOUNT =====
int findCustomerByAccount(String accountNumber) {
  if (customers.empty()) {
    Serial.println(F("Loading customers on demand..."));
    loadCustomersFromDB();
  }
  for (size_t i = 0; i < customers.size(); i++) {
    if (customers[i].account_no == accountNumber) {
      return i;
    }
  }
  return -1;
}

// ===== GET CUSTOMER AT INDEX =====
Customer* getCustomerAt(int index) {
  if (index >= 0 && index < (int)customers.size()) {
    return &customers[index];
  }
  return nullptr;
}

// ===== GET BARANGAY PREFIX FOR CURRENT DEVICE =====
String getCurrentBarangayPrefix() {
  for (const auto& b : barangays) {
    if (b.brgy_id == BRGY_ID_VALUE) {
      return b.prefix;
    }
  }
  return ""; // Not found
}

// ===== EXPORT CUSTOMERS FOR SYNC =====
void exportCustomersForSync() {
  if (customers.empty()) {
    Serial.println(F("Loading customers for export..."));
    loadCustomersFromDB();
  }
  Serial.println(F("BEGIN_CUSTOMERS"));
  for (const auto& c : customers) {
    Serial.printf("CUST|%s|%s|%s|%lu|%s|%d|%d|%d\n",
                  c.account_no.c_str(), c.customer_name.c_str(), c.address.c_str(),
                  c.previous_reading, c.status.c_str(), c.type_id, c.deduction_id, c.brgy_id);
  }
  Serial.println(F("END_CUSTOMERS"));
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

// ===== GET CUSTOMER COUNT =====
int getCustomerCount() {
  return customers.size();
}

#endif  // CUSTOMERS_DATABASE_H
