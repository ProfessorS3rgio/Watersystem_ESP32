#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include "../configuration/config.h"
#include "../managers/sdcard_manager.h"
#include "device_info.h"
#include <sqlite3.h>

void createAllTables();
void initializeDefaultDevice();

static void applyDatabasePragmas() {
  if (!db) return;

  sqlite3_exec(db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
  sqlite3_exec(db, "PRAGMA journal_mode = TRUNCATE;", NULL, NULL, NULL);
  sqlite3_exec(db, "PRAGMA synchronous = NORMAL;", NULL, NULL, NULL);
  sqlite3_exec(db, "PRAGMA temp_store = FILE;", NULL, NULL, NULL);
  sqlite3_exec(db, "PRAGMA cache_size = -4;", NULL, NULL, NULL); // target ~4 KiB page cache
  sqlite3_exec(db, "PRAGMA journal_size_limit = 65536;", NULL, NULL, NULL);
  sqlite3_exec(db, "PRAGMA mmap_size = 0;", NULL, NULL, NULL);
}

static void trimDatabaseMemory() {
  if (!db) return;

  sqlite3_db_release_memory(db);
  sqlite3_release_memory(32768);
}

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
    applyDatabasePragmas();
    createAllTables();
    trimDatabaseMemory();
  }
}

void createAllTables() {
  // Barangay sequence table
  const char *sql_barangay = "CREATE TABLE IF NOT EXISTS barangay_sequence (brgy_id INTEGER PRIMARY KEY AUTOINCREMENT, barangay TEXT UNIQUE, prefix TEXT, next_number INTEGER, synced INTEGER DEFAULT 0, last_sync TEXT, updated_at TEXT);";
  sqlite3_exec(db, sql_barangay, NULL, NULL, NULL);

  // Deductions table
  const char *sql_deductions = "CREATE TABLE IF NOT EXISTS deductions (deduction_id INTEGER PRIMARY KEY, name TEXT, type TEXT, value REAL, synced INTEGER DEFAULT 0, last_sync TEXT, created_at TEXT, updated_at TEXT);";
  sqlite3_exec(db, sql_deductions, NULL, NULL, NULL);

  // Customer types table
  const char *sql_customer_types = "CREATE TABLE IF NOT EXISTS customer_types (type_id INTEGER PRIMARY KEY, type_name TEXT UNIQUE, rate_per_m3 REAL, min_m3 INTEGER DEFAULT 0, min_charge REAL, penalty REAL, synced INTEGER DEFAULT 0, last_sync TEXT, created_at TEXT, updated_at TEXT);";
  sqlite3_exec(db, sql_customer_types, NULL, NULL, NULL);

  // Settings table
  const char *sql_settings = "CREATE TABLE IF NOT EXISTS settings (id INTEGER PRIMARY KEY, bill_due_days INTEGER DEFAULT 5, disconnection_days INTEGER DEFAULT 8, synced INTEGER DEFAULT 0, last_sync TEXT, created_at TEXT, updated_at TEXT);";
  sqlite3_exec(db, sql_settings, NULL, NULL, NULL);

  // Customers table
  const char *sql_customers = "CREATE TABLE IF NOT EXISTS customers (customer_id INTEGER PRIMARY KEY, account_no TEXT UNIQUE, type_id INTEGER, customer_name TEXT, deduction_id INTEGER, brgy_id INTEGER, address TEXT, previous_reading INTEGER, status TEXT DEFAULT 'active', created_at TEXT, updated_at TEXT, FOREIGN KEY(deduction_id) REFERENCES deductions(deduction_id), FOREIGN KEY(type_id) REFERENCES customer_types(type_id), FOREIGN KEY(brgy_id) REFERENCES barangay_sequence(brgy_id));";
  sqlite3_exec(db, sql_customers, NULL, NULL, NULL);
  // Create index on account_no for faster lookups (though UNIQUE already implies it)
  const char *sql_index_customers_account = "CREATE INDEX IF NOT EXISTS idx_customers_account ON customers(account_no);";
  sqlite3_exec(db, sql_index_customers_account, NULL, NULL, NULL);

  // Readings table
  const char *sql_readings = "CREATE TABLE IF NOT EXISTS readings (reading_id INTEGER PRIMARY KEY, customer_id INTEGER, device_uid TEXT, previous_reading INTEGER, current_reading INTEGER, usage_m3 INTEGER, reading_at TEXT, created_at TEXT, updated_at TEXT, synced INTEGER DEFAULT 0, last_sync TEXT, customer_account_number TEXT, FOREIGN KEY(customer_id) REFERENCES customers(customer_id));";
  sqlite3_exec(db, sql_readings, NULL, NULL, NULL);
  // Add device_uid column if not exists
  const char *sql_add_device_uid_readings = "ALTER TABLE readings ADD COLUMN device_uid TEXT;";
  sqlite3_exec(db, sql_add_device_uid_readings, NULL, NULL, NULL); // Ignore error if column exists
  // Create index on customer_account_number for faster lookups
  const char *sql_index_readings_account = "CREATE INDEX IF NOT EXISTS idx_readings_account ON readings(customer_account_number);";
  sqlite3_exec(db, sql_index_readings_account, NULL, NULL, NULL);

  // Bills table
  const char *sql_bills = "CREATE TABLE IF NOT EXISTS bills (bill_id INTEGER PRIMARY KEY, reference_number TEXT UNIQUE, customer_id INTEGER, reading_id INTEGER, device_uid TEXT, bill_date TEXT, rate_per_m3 REAL, charges REAL, penalty REAL, total_due REAL, status TEXT DEFAULT 'Pending', created_at TEXT, updated_at TEXT, synced INTEGER DEFAULT 0, last_sync TEXT, customer_account_number TEXT, FOREIGN KEY(customer_id) REFERENCES customers(customer_id), FOREIGN KEY(reading_id) REFERENCES readings(reading_id));";
  sqlite3_exec(db, sql_bills, NULL, NULL, NULL);
  // Add device_uid column if not exists
  // Create index on customer_account_number for faster lookups
  const char *sql_index_bills_account = "CREATE INDEX IF NOT EXISTS idx_bills_account ON bills(customer_account_number);";
  sqlite3_exec(db, sql_index_bills_account, NULL, NULL, NULL);

  // Bill reference number sequence (persists across reboots)
  // We keep a per-year counter so REF numbers continue incrementing.
  const char *sql_bill_ref_seq =
      "CREATE TABLE IF NOT EXISTS bill_reference_sequence ("
      "year INTEGER PRIMARY KEY, "
      "next_number INTEGER NOT NULL"
      ");";
  sqlite3_exec(db, sql_bill_ref_seq, NULL, NULL, NULL);

  // Bill transactions table
  const char *sql_bill_transactions = "CREATE TABLE IF NOT EXISTS bill_transactions (bill_transaction_id INTEGER PRIMARY KEY, bill_id INTEGER, bill_reference_number TEXT, type TEXT, source TEXT, amount REAL, cash_received REAL, change REAL, transaction_date TEXT, payment_method TEXT, processed_by_device_uid TEXT, notes TEXT, created_at TEXT, updated_at TEXT, synced INTEGER DEFAULT 0, last_sync TEXT, FOREIGN KEY(bill_id) REFERENCES bills(bill_id), FOREIGN KEY(bill_reference_number) REFERENCES bills(reference_number));";
  sqlite3_exec(db, sql_bill_transactions, NULL, NULL, NULL);

  // Device info table
  const char *sql_device_info = "CREATE TABLE IF NOT EXISTS device_info (brgy_id INTEGER, device_mac TEXT UNIQUE, device_uid TEXT, firmware_version TEXT, device_name TEXT, collector TEXT, print_count INTEGER DEFAULT 0, customer_count INTEGER DEFAULT 0, last_sync TEXT, created_at TEXT DEFAULT CURRENT_TIMESTAMP, updated_at TEXT);";
  sqlite3_exec(db, sql_device_info, NULL, NULL, NULL);

  // Re-apply low-memory pragmas after schema setup.
  applyDatabasePragmas();

  // Initialize default device if empty
  initializeDefaultDevice();
  trimDatabaseMemory();
}

void initializeDefaultDevice() {
  if (!db) {
    Serial.println(F("Database not open, skipping default device initialization"));
    return;
  }

  uint64_t mac = ESP.getEfuseMac();
  char macAddress[17];
  snprintf(macAddress, sizeof(macAddress), "%012llX", mac);

  char insert_sql[384];
  snprintf(insert_sql, sizeof(insert_sql),
           "INSERT OR IGNORE INTO device_info (brgy_id, device_mac, device_uid, firmware_version, device_name, collector, print_count, customer_count, last_sync, created_at, updated_at) VALUES (2, '%s', '%s', 'v1.0.0', 'ESP32 Water System', 'Aurelio Macasling', 0, 0, '0', CURRENT_TIMESTAMP, CURRENT_TIMESTAMP);",
           macAddress, macAddress);

  int rc = sqlite3_exec(db, insert_sql, NULL, NULL, NULL);
  if (rc != SQLITE_OK) {
    Serial.print(F("Failed to insert default device record: "));
    Serial.println(sqlite3_errmsg(db));
    return;
  }

  if (sqlite3_changes(db) > 0) {
    Serial.println(F("Initialized default device record"));
  } else {
    Serial.println(F("Device record already exists, skipping initialization"));
  }
}

#endif  // DATABASE_MANAGER_H