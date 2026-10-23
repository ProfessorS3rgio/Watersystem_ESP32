#ifndef READINGS_DATABASE_H
#define READINGS_DATABASE_H

#include <SD.h>
#include <time.h>
#include <stdlib.h>
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
  if (ymd[4] != '-' || ymd[7] != '-') return false;
  year = String(ymd).substring(0, 4).toInt();
  month = String(ymd).substring(5, 7).toInt();
  day = String(ymd).substring(8, 10).toInt();
  if (year < 2000 || year > 2099) return false;
  if (month < 1 || month > 12) return false;
  const int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  const bool leapYear = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
  if (day < 1 || day > daysInMonth[month - 1] + (month == 2 && leapYear ? 1 : 0)) return false;
  return true;
}

static bool normalizeDateIsSelected(const char *sourceDate, const char *selectedDates) {
  if (!selectedDates || !selectedDates[0]) return true;
  char dates[256];
  strncpy(dates, selectedDates, sizeof(dates) - 1);
  dates[sizeof(dates) - 1] = '\0';
  char *token = strtok(dates, ",");
  while (token) {
    while (*token == ' ') ++token;
    if (strcmp(sourceDate, token) == 0) return true;
    token = strtok(nullptr, ",");
  }
  return false;
}

// The web inspector displays device epochs in Philippine time. Set the same
// timezone before localtime()/mktime() so a date chosen in the UI matches the
// date used by the ESP32 normalizer.
static void normalizeUsePhilippineTime() {
  setenv("TZ", "PST-8", 1); // Philippine Standard Time: UTC+08:00
  tzset();
}

// Keep source-date selection consistent with the reading exporter. Some legacy
// rows have an invalid reading_at value (for example, 87), while created_at is
// the only reliable date available for those rows.
static bool normalizeIsValidEpoch(time_t epoch) {
  return epoch >= (time_t)1577836800 && epoch < (time_t)4102444800;
}

static bool normalizeSqlDateTime(const char *value, struct tm &result) {
  if (!value || strlen(value) < 19) return false;
  int year, month, day, hour, minute, second;
  if (sscanf(value, "%4d-%2d-%2d %2d:%2d:%2d", &year, &month, &day, &hour, &minute, &second) != 6) return false;
  int validatedYear, validatedMonth, validatedDay;
  char datePart[11];
  snprintf(datePart, sizeof(datePart), "%04d-%02d-%02d", year, month, day);
  if (!parseYmdDate(datePart, validatedYear, validatedMonth, validatedDay)
      || hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) return false;
  memset(&result, 0, sizeof(result));
  result.tm_year = year - 1900;
  result.tm_mon = month - 1;
  result.tm_mday = day;
  result.tm_hour = hour;
  result.tm_min = minute;
  result.tm_sec = second;
  result.tm_isdst = -1;
  return true;
}

static bool normalizeReadingSource(const char *readingAt, const char *createdAt, char *sourceDate, size_t sourceDateSize, struct tm &sourceTime) {
  const time_t epoch = (time_t)atoll(readingAt ? readingAt : "0");
  if (normalizeIsValidEpoch(epoch)) {
    struct tm *localTime = localtime(&epoch);
    if (localTime) {
      sourceTime = *localTime;
      snprintf(sourceDate, sourceDateSize, "%04d-%02d-%02d", sourceTime.tm_year + 1900, sourceTime.tm_mon + 1, sourceTime.tm_mday);
      return true;
    }
  }

  if (!normalizeSqlDateTime(createdAt, sourceTime)) return false;
  snprintf(sourceDate, sourceDateSize, "%04d-%02d-%02d", sourceTime.tm_year + 1900, sourceTime.tm_mon + 1, sourceTime.tm_mday);
  return true;
}

bool normalizeAllReadingAndBillDatesToDate(const char *ymd, const char *selectedDates = nullptr) {
  if (!db) return false;
  normalizeUsePhilippineTime();
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
    "UPDATE readings SET reading_at = ?, created_at = ?, updated_at = ?, synced = 0, last_sync = NULL WHERE reading_id = ?;";

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

    char sourceDate[11] = "";
    struct tm sourceTime;
    if (!normalizeReadingSource(readingAt, createdAt, sourceDate, sizeof(sourceDate), sourceTime)
        || !normalizeDateIsSelected(sourceDate, selectedDates)) continue;

    sourceTime.tm_year = targetYear - 1900;
    sourceTime.tm_mon = targetMonth - 1;
    sourceTime.tm_mday = targetDay;
    sourceTime.tm_isdst = -1;
    const time_t targetEpoch = mktime(&sourceTime);
    if (!normalizeIsValidEpoch(targetEpoch)) continue;

    char readingAtBuf[16];
    snprintf(readingAtBuf, sizeof(readingAtBuf), "%ld", (long)targetEpoch);

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
  // A bill follows the date of the reading it was generated from. Use that
  // linked reading for selected-date matching so a late-issued bill moves with
  // its reading instead of being compared against a different bill_date.
  const char *selBillsSql =
    "SELECT b.bill_id, b.bill_date, b.created_at, b.updated_at, r.reading_at, r.created_at "
    "FROM bills b LEFT JOIN readings r ON r.reading_id = b.reading_id;";
  const char *updBillsSql =
    "UPDATE bills SET bill_date = ?, created_at = ?, updated_at = ?, synced = 0, last_sync = NULL WHERE bill_id = ?;";

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
    const char *billDate = (const char*)sqlite3_column_text(selectBills, 1);
    const char *createdAt = (const char*)sqlite3_column_text(selectBills, 2);
    const char *updatedAt = (const char*)sqlite3_column_text(selectBills, 3);
    const char *readingAt = (const char*)sqlite3_column_text(selectBills, 4);
    const char *readingCreatedAt = (const char*)sqlite3_column_text(selectBills, 5);

    char createdBuf[20];
    char updatedBuf[20];
    char sourceDate[11] = "";
    struct tm sourceTime;
    const bool linkedReadingHasSelectedDate = normalizeReadingSource(readingAt, readingCreatedAt, sourceDate, sizeof(sourceDate), sourceTime)
      && normalizeDateIsSelected(sourceDate, selectedDates);
    if (!linkedReadingHasSelectedDate
        && billDate && strlen(billDate) >= 10) {
      strncpy(sourceDate, billDate, 10);
      sourceDate[10] = '\0';
    }
    // Readings are normalized before their bills. Match the bill's own original
    // date as well, otherwise a linked reading has already moved off the
    // selected source date by the time this loop runs.
    const bool billDateIsSelected = billDate && strlen(billDate) >= 10
      && normalizeDateIsSelected(billDate, selectedDates);
    if (!linkedReadingHasSelectedDate && !billDateIsSelected) continue;
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

// Repairs explicitly selected device readings. This is intentionally ID-based:
// after a bulk normalization, source dates are no longer safe identifiers.
bool normalizeReadingAndLinkedBillDatesByIds(const char *ymd, const char *readingIds) {
  if (!db || !readingIds || !readingIds[0]) return false;
  normalizeUsePhilippineTime();
  int targetYear = 0, targetMonth = 0, targetDay = 0;
  if (!parseYmdDate(ymd, targetYear, targetMonth, targetDay)) return false;

  char *error = nullptr;
  if (sqlite3_exec(db, "BEGIN;", nullptr, nullptr, &error) != SQLITE_OK) {
    if (error) sqlite3_free(error);
    return false;
  }
  auto rollback = []() { sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr); };

  sqlite3_stmt *findReading = nullptr, *updateReading = nullptr, *findBills = nullptr, *updateBill = nullptr;
  const char *findReadingSql = "SELECT reading_at, created_at, updated_at FROM readings WHERE reading_id = ?;";
  const char *updateReadingSql = "UPDATE readings SET reading_at = ?, created_at = ?, updated_at = ?, synced = 0, last_sync = NULL WHERE reading_id = ?;";
  const char *findBillsSql = "SELECT bill_id, created_at, updated_at FROM bills WHERE reading_id = ?;";
  const char *updateBillSql = "UPDATE bills SET bill_date = ?, created_at = ?, updated_at = ?, synced = 0, last_sync = NULL WHERE bill_id = ?;";
  if (sqlite3_prepare_v2(db, findReadingSql, -1, &findReading, nullptr) != SQLITE_OK
      || sqlite3_prepare_v2(db, updateReadingSql, -1, &updateReading, nullptr) != SQLITE_OK
      || sqlite3_prepare_v2(db, findBillsSql, -1, &findBills, nullptr) != SQLITE_OK
      || sqlite3_prepare_v2(db, updateBillSql, -1, &updateBill, nullptr) != SQLITE_OK) {
    if (findReading) sqlite3_finalize(findReading); if (updateReading) sqlite3_finalize(updateReading);
    if (findBills) sqlite3_finalize(findBills); if (updateBill) sqlite3_finalize(updateBill);
    rollback(); return false;
  }

  char ids[1024];
  strncpy(ids, readingIds, sizeof(ids) - 1); ids[sizeof(ids) - 1] = '\0';
  int updatedReadings = 0, updatedBills = 0;
  bool completed = false;
  for (char *token = strtok(ids, ","); token; token = strtok(nullptr, ",")) {
    char *end = nullptr;
    const long readingId = strtol(token, &end, 10);
    if (readingId <= 0 || !end || *end != '\0') continue;

    sqlite3_reset(findReading); sqlite3_clear_bindings(findReading);
    sqlite3_bind_int(findReading, 1, (int)readingId);
    if (sqlite3_step(findReading) != SQLITE_ROW) continue;
    const char *readingAt = (const char*)sqlite3_column_text(findReading, 0);
    const char *createdAt = (const char*)sqlite3_column_text(findReading, 1);
    const char *updatedAt = (const char*)sqlite3_column_text(findReading, 2);
    char sourceDate[11] = ""; struct tm sourceTime;
    if (!normalizeReadingSource(readingAt, createdAt, sourceDate, sizeof(sourceDate), sourceTime)) continue;
    sourceTime.tm_year = targetYear - 1900; sourceTime.tm_mon = targetMonth - 1; sourceTime.tm_mday = targetDay; sourceTime.tm_isdst = -1;
    const time_t targetEpoch = mktime(&sourceTime);
    if (!normalizeIsValidEpoch(targetEpoch)) continue;
    char epoch[16], created[20], updated[20];
    snprintf(epoch, sizeof(epoch), "%ld", (long)targetEpoch);
    char timePart[9] = "00:00:00";
    if (createdAt && strlen(createdAt) >= 19) { memcpy(timePart, createdAt + 11, 8); timePart[8] = '\0'; }
    snprintf(created, sizeof(created), "%s %s", ymd, timePart);
    strncpy(timePart, "00:00:00", sizeof(timePart));
    if (updatedAt && strlen(updatedAt) >= 19) { memcpy(timePart, updatedAt + 11, 8); timePart[8] = '\0'; }
    snprintf(updated, sizeof(updated), "%s %s", ymd, timePart);
    sqlite3_reset(updateReading); sqlite3_clear_bindings(updateReading);
    sqlite3_bind_text(updateReading, 1, epoch, -1, SQLITE_TRANSIENT); sqlite3_bind_text(updateReading, 2, created, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(updateReading, 3, updated, -1, SQLITE_TRANSIENT); sqlite3_bind_int(updateReading, 4, (int)readingId);
    if (sqlite3_step(updateReading) != SQLITE_DONE) { rollback(); goto cleanup; }
    updatedReadings++;

    sqlite3_reset(findBills); sqlite3_clear_bindings(findBills); sqlite3_bind_int(findBills, 1, (int)readingId);
    while (sqlite3_step(findBills) == SQLITE_ROW) {
      const int billId = sqlite3_column_int(findBills, 0);
      const char *billCreated = (const char*)sqlite3_column_text(findBills, 1);
      const char *billUpdated = (const char*)sqlite3_column_text(findBills, 2);
      char billCreatedDate[20], billUpdatedDate[20];
      snprintf(billCreatedDate, sizeof(billCreatedDate), "%s %s", ymd, billCreated && strlen(billCreated) >= 19 ? billCreated + 11 : "00:00:00");
      snprintf(billUpdatedDate, sizeof(billUpdatedDate), "%s %s", ymd, billUpdated && strlen(billUpdated) >= 19 ? billUpdated + 11 : "00:00:00");
      sqlite3_reset(updateBill); sqlite3_clear_bindings(updateBill);
      sqlite3_bind_text(updateBill, 1, ymd, -1, SQLITE_TRANSIENT); sqlite3_bind_text(updateBill, 2, billCreatedDate, -1, SQLITE_TRANSIENT);
      sqlite3_bind_text(updateBill, 3, billUpdatedDate, -1, SQLITE_TRANSIENT); sqlite3_bind_int(updateBill, 4, billId);
      if (sqlite3_step(updateBill) != SQLITE_DONE) { rollback(); goto cleanup; }
      updatedBills++;
    }
  }
  if (sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &error) != SQLITE_OK) { if (error) sqlite3_free(error); rollback(); goto cleanup; }
  completed = true;
  Serial.printf("Date repair complete. Readings updated: %d, Bills updated: %d\n", updatedReadings, updatedBills);
cleanup:
  sqlite3_finalize(findReading); sqlite3_finalize(updateReading); sqlite3_finalize(findBills); sqlite3_finalize(updateBill);
  return completed && updatedReadings > 0;
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
