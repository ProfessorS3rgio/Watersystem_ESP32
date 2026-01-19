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
  c.customer_id = atoi(argv[0]);
  c.account_no = argv[1];
  c.type_id = atoi(argv[2]);
  c.customer_name = argv[3];
  c.deduction_id = atoi(argv[4]);
  c.brgy_id = atoi(argv[5]);
  c.address = argv[6];
  c.previous_reading = strtoul(argv[7], NULL, 10);
  c.status = argv[8];
  c.created_at = argv[9];
  c.updated_at = argv[10];
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
  const char *sql = "SELECT customer_id, account_no, type_id, customer_name, deduction_id, brgy_id, address, previous_reading, status, created_at, updated_at FROM customers;";
  sqlite3_exec(db, sql, loadCustomerCallback, NULL, NULL);
}

void loadBarangaysFromDB() {
  barangays.clear();
  const char *sql = "SELECT brgy_id, barangay, prefix, next_number, updated_at FROM barangay_sequence;";
  sqlite3_exec(db, sql, loadBarangayCallback, NULL, NULL);
}

void initCustomersDatabase() {
  loadCustomersFromDB();
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
  char sql[512];
  sprintf(sql, "INSERT OR REPLACE INTO customers (account_no, customer_name, address, previous_reading, status, type_id, deduction_id, brgy_id, created_at, updated_at) VALUES ('%s', '%s', '%s', %lu, '%s', %lu, %lu, %lu, datetime('now'), datetime('now'));",
          accountNo.c_str(), name.c_str(), address.c_str(), prev, status.c_str(), typeId, deductionId, brgyId);
  int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
  if (rc == SQLITE_OK) {
    loadCustomersFromDB(); // Reload to update vector
    return true;
  }
  return false;
}

// ===== REMOVE CUSTOMER BY ACCOUNT =====
bool removeCustomerByAccount(const String& accountNumber) {
  char sql[256];
  sprintf(sql, "DELETE FROM customers WHERE account_no = '%s';", accountNumber.c_str());
  int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
  if (rc == SQLITE_OK) {
    loadCustomersFromDB(); // Reload to update vector
    return true;
  }
  return false;
}

// ===== GET CUSTOMER COUNT =====
int getCustomerCount() {
  return customers.size();
}

#endif  // CUSTOMERS_DATABASE_H
