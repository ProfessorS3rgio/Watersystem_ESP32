#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include "../configuration/config.h"
#include "../managers/sdcard_manager.h"
#include "device_info.h"
#include <sqlite3.h>

void createAllTables();
void initializeDefaultDevice();

void initDatabase() {
  if (!db) {
    // Ensure SD card is ready before opening database
    if (!isSDCardReady()) {
      Serial.println(F("SD card not ready, cannot open database"));
      return;
    }
    
    int rc = sqlite3_open(DB_PATH, &db);
    if (rc) {
      Serial.printf("Can't open database: %s\n", sqlite3_errmsg(db));
      return;
    }
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
    createAllTables();
  }
}

void createAllTables() {
  // Barangay sequence table
  const char *sql_barangay = "CREATE TABLE IF NOT EXISTS barangay_sequence (brgy_id INTEGER PRIMARY KEY AUTOINCREMENT, barangay TEXT UNIQUE, prefix TEXT, next_number INTEGER, updated_at TEXT);";
  sqlite3_exec(db, sql_barangay, NULL, NULL, NULL);

  // Deductions table
  const char *sql_deductions = "CREATE TABLE IF NOT EXISTS deductions (deduction_id INTEGER PRIMARY KEY, name TEXT, type TEXT, value REAL, created_at TEXT, updated_at TEXT);";
  sqlite3_exec(db, sql_deductions, NULL, NULL, NULL);

  // Customer types table
  const char *sql_customer_types = "CREATE TABLE IF NOT EXISTS customer_types (type_id INTEGER PRIMARY KEY, type_name TEXT UNIQUE, rate_per_m3 REAL, min_m3 INTEGER DEFAULT 0, min_charge REAL, penalty REAL, created_at TEXT, updated_at TEXT);";
  sqlite3_exec(db, sql_customer_types, NULL, NULL, NULL);

  // Customers table
  const char *sql_customers = "CREATE TABLE IF NOT EXISTS customers (customer_id INTEGER PRIMARY KEY, account_no TEXT UNIQUE, type_id INTEGER, customer_name TEXT, deduction_id INTEGER, brgy_id INTEGER, address TEXT, previous_reading INTEGER, status TEXT DEFAULT 'active', created_at TEXT, updated_at TEXT, FOREIGN KEY(deduction_id) REFERENCES deductions(deduction_id), FOREIGN KEY(type_id) REFERENCES customer_types(type_id), FOREIGN KEY(brgy_id) REFERENCES barangay_sequence(brgy_id));";
  sqlite3_exec(db, sql_customers, NULL, NULL, NULL);

  // Readings table
  const char *sql_readings = "CREATE TABLE IF NOT EXISTS readings (reading_id INTEGER PRIMARY KEY, customer_id INTEGER, device_uid TEXT, previous_reading INTEGER, current_reading INTEGER, usage_m3 INTEGER, reading_at TEXT, created_at TEXT, updated_at TEXT, FOREIGN KEY(customer_id) REFERENCES customers(customer_id));";
  sqlite3_exec(db, sql_readings, NULL, NULL, NULL);
  // Add device_uid column if not exists
  const char *sql_add_device_uid_readings = "ALTER TABLE readings ADD COLUMN device_uid TEXT;";
  sqlite3_exec(db, sql_add_device_uid_readings, NULL, NULL, NULL); // Ignore error if column exists

  // Bills table
  const char *sql_bills = "CREATE TABLE IF NOT EXISTS bills (bill_id INTEGER PRIMARY KEY, reference_number TEXT UNIQUE, customer_id INTEGER, reading_id INTEGER, device_uid TEXT, bill_date TEXT, rate_per_m3 REAL, charges REAL, penalty REAL, total_due REAL, status TEXT DEFAULT 'Pending', created_at TEXT, updated_at TEXT, FOREIGN KEY(customer_id) REFERENCES customers(customer_id), FOREIGN KEY(reading_id) REFERENCES readings(reading_id));";
  sqlite3_exec(db, sql_bills, NULL, NULL, NULL);
  // Add device_uid column if not exists
  const char *sql_add_device_uid_bills = "ALTER TABLE bills ADD COLUMN device_uid TEXT;";
  sqlite3_exec(db, sql_add_device_uid_bills, NULL, NULL, NULL); // Ignore error if column exists

  // Bill transactions table
  const char *sql_bill_transactions = "CREATE TABLE IF NOT EXISTS bill_transactions (bill_transaction_id INTEGER PRIMARY KEY, bill_id INTEGER, bill_reference_number TEXT, type TEXT, source TEXT, amount REAL, cash_received REAL, change REAL, transaction_date TEXT, payment_method TEXT, processed_by_user_id INTEGER, notes TEXT, created_at TEXT, updated_at TEXT, FOREIGN KEY(bill_id) REFERENCES bills(bill_id));";
  sqlite3_exec(db, sql_bill_transactions, NULL, NULL, NULL);

  // Device info table
  const char *sql_device_info = "CREATE TABLE IF NOT EXISTS device_info (brgy_id INTEGER, device_mac TEXT UNIQUE, device_uid TEXT, firmware_version TEXT, device_name TEXT, print_count INTEGER DEFAULT 0, customer_count INTEGER DEFAULT 0, last_sync TEXT, created_at TEXT DEFAULT CURRENT_TIMESTAMP, updated_at TEXT);";
  sqlite3_exec(db, sql_device_info, NULL, NULL, NULL);

  // Optimize SQLite for low memory ESP32
sqlite3_exec(db, "PRAGMA journal_mode = WAL;", NULL, NULL, NULL);
sqlite3_exec(db, "PRAGMA synchronous = NORMAL;", NULL, NULL, NULL);
sqlite3_exec(db, "PRAGMA temp_store = MEMORY;", NULL, NULL, NULL);
sqlite3_exec(db, "PRAGMA cache_size = -10;", NULL, NULL, NULL); // ~40KB
sqlite3_exec(db, "PRAGMA mmap_size = 0;", NULL, NULL, NULL);

  // Initialize default device if empty
  initializeDefaultDevice();
}

void initializeDefaultDevice() {
  // Insert or replace default device record
  String macAddress = getDeviceUID();
  char insert_sql[512];
  sprintf(insert_sql, "INSERT OR REPLACE INTO device_info (brgy_id, device_mac, device_uid, firmware_version, device_name, print_count, customer_count, last_sync, updated_at) VALUES (2, '%s', '%s', 'v1.0.0', 'ESP32 Water System', 0, 0, '0', datetime('now'));", macAddress.c_str(), macAddress.c_str());
  sqlite3_exec(db, insert_sql, NULL, NULL, NULL);
  Serial.println(F("Initialized default device record"));
}

#endif  // DATABASE_MANAGER_H