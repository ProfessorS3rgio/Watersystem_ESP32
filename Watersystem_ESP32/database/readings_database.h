#ifndef READINGS_DATABASE_H
#define READINGS_DATABASE_H

#include <SD.h>
#include <time.h>
#include <sys/time.h>
#include "../configuration/config.h"
#include "../managers/sdcard_manager.h"
#include "customers_database.h"
#include <sqlite3.h>
#include <vector>
#include "database_manager.h"
#include <ArduinoJson.h>

// Device ID for this ESP32 device (Makilas barangay)
static const int DEVICE_ID = 2;

static long g_timeOffsetSeconds = 0; // epochNow ~= millis()/1000 + offset

// ===== READING DATA STRUCTURE =====
struct Reading {
  int reading_id;
  int customer_id;
  String device_uid;
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
  r.device_uid = argv[2];
  r.previous_reading = strtoul(argv[3], NULL, 10);
  r.current_reading = strtoul(argv[4], NULL, 10);
  r.usage_m3 = strtoul(argv[5], NULL, 10);
  r.reading_at = argv[6];
  r.created_at = argv[7];
  r.updated_at = argv[8];
  readings.push_back(r);
  return 0;
}

void loadReadingsFromDB() {
  readings.clear();
  const char *sql = "SELECT reading_id, customer_id, device_uid, previous_reading, current_reading, usage_m3, reading_at, created_at, updated_at FROM readings ORDER BY reading_id;";
  sqlite3_exec(db, sql, loadReadingCallback, NULL, NULL);
}

static uint32_t deviceEpochNow() {
  return (uint32_t)((long)(millis() / 1000) + g_timeOffsetSeconds);
}

void saveReadingToDB(int customer_id, unsigned long previous_reading, unsigned long current_reading, unsigned long usage_m3, String reading_at) {
  char sql[512];
  String deviceUID = getDeviceUID();
  String timestamp = String(deviceEpochNow());
  sprintf(sql, "INSERT INTO readings (customer_id, device_uid, previous_reading, current_reading, usage_m3, reading_at, created_at, updated_at) VALUES (%d, '%s', %lu, %lu, %lu, '%s', datetime('now'), datetime('now'));", customer_id, deviceUID.c_str(), previous_reading, current_reading, usage_m3, timestamp.c_str());
  // Serial.println(sql);  // Commented out to save heap memory
  int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
  Serial.print(F("Reading save result: "));
  Serial.println(rc == SQLITE_OK ? "OK" : "FAILED");
}

bool hasReadingForCustomerInYearMonth(int customer_id, int year, int month) {
  char sql[256];
  sprintf(sql, "SELECT COUNT(*) FROM readings WHERE customer_id = %d AND strftime('%%Y', datetime(reading_at, 'unixepoch')) = '%04d' AND strftime('%%m', datetime(reading_at, 'unixepoch')) = '%02d';", customer_id, year, month);
  // For simplicity, assume no reading if not loaded, but since we load all, check vector
  for (const auto& r : readings) {
    if (r.customer_id == customer_id) {
      // Parse reading_at for year month
      // For now, return false to allow new reading
    }
  }
  return false; // Placeholder
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
  // loadReadingsFromDB(); // Skip loading readings at boot to avoid heap exhaustion. Load on demand.
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

  // Update in-memory customer previous reading
  if (currentCustomer) {
    currentCustomer->previous_reading = currentReading;
  }

  return true;
}

// ===== MARK ALL READINGS SYNCED =====
bool markAllReadingsSynced() {
  const char *sql = "UPDATE readings SET updated_at = datetime('now');";
  int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
  return rc == SQLITE_OK;
}

void setDeviceEpoch(uint32_t epoch) {
  // Set the ESP32 system time
  struct timeval tv;
  tv.tv_sec = epoch;
  tv.tv_usec = 0;
  settimeofday(&tv, NULL);

  // Also maintain offset for compatibility
  uint32_t nowMillis = millis() / 1000;
  g_timeOffsetSeconds = epoch - nowMillis;
  saveDeviceTimeOffsetToDB(); // Save to DB

  Serial.print(F("System time set to: "));
  Serial.println(epoch);
}

#endif  // READINGS_DATABASE_H
