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
  bool synced;
  String last_sync;
  String customer_account_number;
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
  r.synced = atoi(argv[9]);
  r.last_sync = argv[10] ? argv[10] : "";
  r.customer_account_number = argv[11] ? argv[11] : "";
  readings.push_back(r);
  return 0;
}

void loadReadingsFromDB() {
  readings.clear();
  const char *sql = "SELECT reading_id, customer_id, device_uid, previous_reading, current_reading, usage_m3, reading_at, created_at, updated_at, synced, last_sync, customer_account_number FROM readings ORDER BY reading_id;";
  sqlite3_exec(db, sql, loadReadingCallback, NULL, NULL);
}

static uint32_t deviceEpochNow() {
  return (uint32_t)((long)(millis() / 1000) + g_timeOffsetSeconds);
}

void saveReadingToDB(int customer_id, unsigned long previous_reading, unsigned long current_reading, unsigned long usage_m3, String reading_at, String account_no) {
  char sql[512];
  String deviceUID = getDeviceUID();
  String timestamp = String(deviceEpochNow());
  String nowStr = getCurrentDateTimeString();
  sprintf(sql, "INSERT INTO readings (customer_id, device_uid, previous_reading, current_reading, usage_m3, reading_at, created_at, updated_at, synced, last_sync, customer_account_number) VALUES (%d, '%s', %lu, %lu, %lu, '%s', '%s', '%s', 0, NULL, '%s');", customer_id, deviceUID.c_str(), previous_reading, current_reading, usage_m3, timestamp.c_str(), nowStr.c_str(), nowStr.c_str(), account_no.c_str());
  // Serial.println(sql);  // Commented out to save heap memory
  int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
  Serial.print(F("Reading save result: "));
  Serial.println(rc == SQLITE_OK ? "OK" : "FAILED");
}

bool hasReadingForCustomerInYearMonth(int customer_id, int year, int month) {
  if (!db) return false;

  char sql[256];
  sprintf(sql,
          "SELECT COUNT(*) FROM readings WHERE customer_id = %d "
          "AND strftime('%%Y', datetime(reading_at, 'unixepoch')) = '%04d' "
          "AND strftime('%%m', datetime(reading_at, 'unixepoch')) = '%02d';",
          customer_id, year, month);

  sqlite3_stmt *stmt = nullptr;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) return false;

  bool hasReading = false;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    hasReading = sqlite3_column_int(stmt, 0) > 0;
  }
  sqlite3_finalize(stmt);
  return hasReading;
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
  String nowStr = getCurrentDateTimeString();
  sprintf(sql, "INSERT OR REPLACE INTO device_info (key, value, created_at, updated_at) VALUES ('time_offset', '%d', '%s', '%s');", g_timeOffsetSeconds, nowStr.c_str(), nowStr.c_str());

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

// ===== NORMALIZE DATES TO YYYY-MM-DD =====
static bool parseYmdDate(const char *ymd, int &year, int &month, int &day) {
  if (!ymd || strlen(ymd) != 10) return false;
  year = String(ymd).substring(0, 4).toInt();
  month = String(ymd).substring(5, 7).toInt();
  day = String(ymd).substring(8, 10).toInt();
  if (year < 2000 || year > 2099) return false;
  if (month < 1 || month > 12) return false;
  if (day < 1 || day > 31) return false;
  return true;
}

bool normalizeAllReadingAndBillDatesToDate(const char *ymd) {
  if (!db) return false;
  int targetYear = 0;
  int targetMonth = 0;
  int targetDay = 0;
  if (!parseYmdDate(ymd, targetYear, targetMonth, targetDay)) {
    Serial.println(F("Date normalize failed: invalid date"));
    return false;
  }
  auto execOrLog = [](const char *sql) -> bool {
    char *errMsg = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
      if (errMsg) {
        Serial.print(F("Date normalize failed: "));
        Serial.println(errMsg);
        sqlite3_free(errMsg);
      }
      return false;
    }
    return true;
  };

  auto buildTargetDateTime = [targetYear, targetMonth, targetDay](const char *src, char *dst, size_t dstSize) {
    char timeBuf[9] = "00:00:00";
    if (src) {
      size_t len = strlen(src);
      if (len >= 19) {
        memcpy(timeBuf, src + 11, 8);
        timeBuf[8] = '\0';
      }
    }
    snprintf(dst, dstSize, "%04d-%02d-%02d %s", targetYear, targetMonth, targetDay, timeBuf);
  };

  if (!execOrLog("BEGIN;")) return false;

  sqlite3_stmt *selectReadings = nullptr;
  sqlite3_stmt *updateReadings = nullptr;
  const char *selReadingsSql = "SELECT reading_id, reading_at, created_at, updated_at FROM readings;";
  const char *updReadingsSql =
    "UPDATE readings SET reading_at = ?, created_at = ?, updated_at = ? WHERE reading_id = ?;";

  int rc = sqlite3_prepare_v2(db, selReadingsSql, -1, &selectReadings, nullptr);
  if (rc != SQLITE_OK) {
    execOrLog("ROLLBACK;");
    return false;
  }
  rc = sqlite3_prepare_v2(db, updReadingsSql, -1, &updateReadings, nullptr);
  if (rc != SQLITE_OK) {
    sqlite3_finalize(selectReadings);
    execOrLog("ROLLBACK;");
    return false;
  }

  int updatedReadings = 0;
  while ((rc = sqlite3_step(selectReadings)) == SQLITE_ROW) {
    int readingId = sqlite3_column_int(selectReadings, 0);
    const char *readingAt = (const char*)sqlite3_column_text(selectReadings, 1);
    const char *createdAt = (const char*)sqlite3_column_text(selectReadings, 2);
    const char *updatedAt = (const char*)sqlite3_column_text(selectReadings, 3);

    time_t epoch = (time_t)atol(readingAt ? readingAt : "0");
    struct tm *t = localtime(&epoch);
    if (t) {
      t->tm_year = targetYear - 1900;
      t->tm_mon = targetMonth - 1;
      t->tm_mday = targetDay;
      epoch = mktime(t);
    }

    char readingAtBuf[16];
    snprintf(readingAtBuf, sizeof(readingAtBuf), "%ld", (long)epoch);

    char createdBuf[20];
    char updatedBuf[20];
    buildTargetDateTime(createdAt, createdBuf, sizeof(createdBuf));
    buildTargetDateTime(updatedAt, updatedBuf, sizeof(updatedBuf));

    sqlite3_reset(updateReadings);
    sqlite3_bind_text(updateReadings, 1, readingAtBuf, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(updateReadings, 2, createdBuf, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(updateReadings, 3, updatedBuf, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(updateReadings, 4, readingId);

    if (sqlite3_step(updateReadings) != SQLITE_DONE) {
      sqlite3_finalize(selectReadings);
      sqlite3_finalize(updateReadings);
      execOrLog("ROLLBACK;");
      return false;
    }
    updatedReadings++;
  }

  sqlite3_finalize(selectReadings);
  sqlite3_finalize(updateReadings);

  sqlite3_stmt *selectBills = nullptr;
  sqlite3_stmt *updateBills = nullptr;
  const char *selBillsSql = "SELECT bill_id, created_at, updated_at FROM bills;";
  const char *updBillsSql =
    "UPDATE bills SET bill_date = ?, created_at = ?, updated_at = ? WHERE bill_id = ?;";

  rc = sqlite3_prepare_v2(db, selBillsSql, -1, &selectBills, nullptr);
  if (rc != SQLITE_OK) {
    execOrLog("ROLLBACK;");
    return false;
  }
  rc = sqlite3_prepare_v2(db, updBillsSql, -1, &updateBills, nullptr);
  if (rc != SQLITE_OK) {
    sqlite3_finalize(selectBills);
    execOrLog("ROLLBACK;");
    return false;
  }

  int updatedBills = 0;
  while ((rc = sqlite3_step(selectBills)) == SQLITE_ROW) {
    int billId = sqlite3_column_int(selectBills, 0);
    const char *createdAt = (const char*)sqlite3_column_text(selectBills, 1);
    const char *updatedAt = (const char*)sqlite3_column_text(selectBills, 2);

    char createdBuf[20];
    char updatedBuf[20];
    buildTargetDateTime(createdAt, createdBuf, sizeof(createdBuf));
    buildTargetDateTime(updatedAt, updatedBuf, sizeof(updatedBuf));

    sqlite3_reset(updateBills);
    sqlite3_bind_text(updateBills, 1, ymd, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(updateBills, 2, createdBuf, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(updateBills, 3, updatedBuf, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(updateBills, 4, billId);

    if (sqlite3_step(updateBills) != SQLITE_DONE) {
      sqlite3_finalize(selectBills);
      sqlite3_finalize(updateBills);
      execOrLog("ROLLBACK;");
      return false;
    }
    updatedBills++;
  }

  sqlite3_finalize(selectBills);
  sqlite3_finalize(updateBills);

  if (!execOrLog("COMMIT;")) return false;

  Serial.print(F("Date normalize complete. Readings updated: "));
  Serial.print(updatedReadings);
  Serial.print(F(", Bills updated: "));
  Serial.println(updatedBills);
  return true;
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
  String nowStr = getCurrentDateTimeString();
  sprintf(sql, "UPDATE customers SET previous_reading = %lu, updated_at = '%s' WHERE customer_id = %d;", currentReading, nowStr.c_str(), c->customer_id);
  sqlite3_exec(db, sql, NULL, NULL, NULL);

  // Insert reading
  saveReadingToDB(c->customer_id, previous, currentReading, usage, "datetime('now')", c->account_no);

  // Update in-memory customer previous reading
  if (currentCustomer) {
    currentCustomer->previous_reading = currentReading;
  }

  return true;
}

// ===== MARK ALL READINGS SYNCED =====
bool markAllReadingsSynced() {
  String nowStr = getCurrentDateTimeString();
  char sql[128];
  sprintf(sql, "UPDATE readings SET synced = 1, last_sync = '%s', updated_at = '%s';", nowStr.c_str(), nowStr.c_str());
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
