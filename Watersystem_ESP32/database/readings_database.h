#ifndef READINGS_DATABASE_H
#define READINGS_DATABASE_H

#include <SD.h>
#include <time.h>
#include "../configuration/config.h"
#include "../managers/sdcard_manager.h"
#include "customers_database.h"
#include <sqlite3.h>
#include <vector>
#include "database_manager.h"

// Device ID for this ESP32 device (Makilas barangay)
static const int DEVICE_ID = 2;

static long g_timeOffsetSeconds = 0; // epochNow ~= millis()/1000 + offset

// ===== READING DATA STRUCTURE =====
struct Reading {
  int reading_id;
  int customer_id;
  unsigned long previous_reading;
  unsigned long current_reading;
  unsigned long usage_m3;
  String reading_at;
  String created_at;
  String updated_at;
};

// ===== READINGS DATABASE =====
std::vector<Reading> readings;

static int loadReadingCallback(void *data, int argc, char **argv, char **azColName) {
  Reading r;
  r.reading_id = atoi(argv[0]);
  r.customer_id = atoi(argv[1]);
  r.previous_reading = strtoul(argv[2], NULL, 10);
  r.current_reading = strtoul(argv[3], NULL, 10);
  r.usage_m3 = strtoul(argv[4], NULL, 10);
  r.reading_at = argv[5];
  r.created_at = argv[6];
  r.updated_at = argv[7];
  readings.push_back(r);
  return 0;
}

void loadReadingsFromDB() {
  readings.clear();
  const char *sql = "SELECT reading_id, customer_id, previous_reading, current_reading, usage_m3, reading_at, created_at, updated_at FROM readings ORDER BY reading_id;";
  sqlite3_exec(db, sql, loadReadingCallback, NULL, NULL);
}

void saveReadingToDB(int customer_id, unsigned long previous_reading, unsigned long current_reading, unsigned long usage_m3, String reading_at) {
  char sql[512];
  sprintf(sql, "INSERT INTO readings (customer_id, previous_reading, current_reading, usage_m3, reading_at, created_at, updated_at) VALUES (%d, %lu, %lu, %lu, '%s', datetime('now'), datetime('now'));", customer_id, previous_reading, current_reading, usage_m3, reading_at.c_str());
  sqlite3_exec(db, sql, NULL, NULL, NULL);
}

bool hasReadingForCustomerInYearMonth(int customer_id, int year, int month) {
  char sql[256];
  sprintf(sql, "SELECT COUNT(*) FROM readings WHERE customer_id = %d AND strftime('%%Y', reading_at) = '%04d' AND strftime('%%m', reading_at) = '%02d';", customer_id, year, month);
  // For simplicity, assume no reading if not loaded, but since we load all, check vector
  for (const auto& r : readings) {
    if (r.customer_id == customer_id) {
      // Parse reading_at for year month
      // For now, return false to allow new reading
    }
  }
  return false; // Placeholder
}

void exportReadingsForSync() {
  // Export readings for sync
  Serial.println(F("BEGIN_READINGS"));
  for (const auto& r : readings) {
    Serial.printf("READING|%d|%d|%lu|%lu|%lu|%s\n", r.reading_id, r.customer_id, r.previous_reading, r.current_reading, r.usage_m3, r.reading_at.c_str());
  }
  Serial.println(F("END_READINGS"));
}

static uint32_t deviceEpochNow() {
  return (uint32_t)((long)(millis() / 1000) + g_timeOffsetSeconds);
}

// For time offset, keep SD for now or migrate to settings
static bool loadDeviceTimeOffsetFromDB() {
  if (!db) return false;

  const char *sql = "SELECT value FROM device_info WHERE key = 'time_offset';";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) return false;

  bool success = false;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *value = (const char*)sqlite3_column_text(stmt, 0);
    if (value) {
      g_timeOffsetSeconds = atoi(value);
      Serial.print(F("Loaded time offset from DB: "));
      Serial.println(g_timeOffsetSeconds);
      success = true;
    }
  }

  sqlite3_finalize(stmt);
  return success;
}

static bool saveDeviceTimeOffsetToDB() {
  if (!db) return false;

  char sql[256];
  sprintf(sql, "INSERT OR REPLACE INTO device_info (key, value, created_at, updated_at) VALUES ('time_offset', '%d', datetime('now'), datetime('now'));", g_timeOffsetSeconds);

  int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
  if (rc == SQLITE_OK) {
    Serial.print(F("Saved time offset to DB: "));
    Serial.println(g_timeOffsetSeconds);
    return true;
  }
  return false;
}

void initReadingsDatabase() {
  loadReadingsFromDB();
  loadDeviceTimeOffsetFromDB(); // Load from DB
}

// ===== HAS READING FOR ACCOUNT THIS MONTH =====
bool hasReadingForAccountThisMonth(String accountNo) {
  // Find customer_id by account
  int customerIndex = findCustomerByAccount(accountNo);
  if (customerIndex == -1) return false;
  Customer* c = getCustomerAt(customerIndex);
  if (!c) return false;
  int customer_id = c->customer_id;

  // Check if reading exists for this month
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int year = t->tm_year + 1900;
  int month = t->tm_mon + 1;

  return hasReadingForCustomerInYearMonth(customer_id, year, month);
}

// ===== RECORD READING FOR CUSTOMER INDEX =====
bool recordReadingForCustomerIndex(int customerIndex, unsigned long currentReading) {
  Customer* c = getCustomerAt(customerIndex);
  if (!c) return false;

  unsigned long previous = c->previous_reading;
  unsigned long usage = (currentReading > previous) ? (currentReading - previous) : 0;

  // Update customer previous_reading
  char sql[256];
  sprintf(sql, "UPDATE customers SET previous_reading = %lu, updated_at = datetime('now') WHERE customer_id = %d;", currentReading, c->customer_id);
  sqlite3_exec(db, sql, NULL, NULL, NULL);

  // Insert reading
  saveReadingToDB(c->customer_id, previous, currentReading, usage, "datetime('now')");

  // Reload customers to update vector
  loadCustomersFromDB();

  return true;
}

// ===== MARK ALL READINGS SYNCED =====
bool markAllReadingsSynced() {
  const char *sql = "UPDATE readings SET updated_at = datetime('now');";
  int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
  return rc == SQLITE_OK;
}

void setDeviceEpoch(uint32_t epoch) {
  // Set the time offset based on provided epoch
  // Assuming epoch is the current time from server
  uint32_t nowMillis = millis() / 1000;
  g_timeOffsetSeconds = epoch - nowMillis;
  saveDeviceTimeOffsetToDB(); // Save to DB
}

#endif  // READINGS_DATABASE_H
