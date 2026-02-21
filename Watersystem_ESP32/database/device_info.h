#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

#include <SD.h>
#include "../configuration/config.h"
#include "../managers/sdcard_manager.h"
#include <sqlite3.h>

// Device info now stored in SQLite database table 'device_info'
// Table: (brgy_id INTEGER, device_mac TEXT UNIQUE, device_uid TEXT, firmware_version TEXT, device_name TEXT, collector TEXT, print_count INTEGER, customer_count INTEGER, last_sync TEXT, created_at TEXT, updated_at TEXT)

static const char* DEVICE_TYPE_VALUE = "ESP32 Water System";
static const char* FIRMWARE_VERSION_VALUE = "v1.0.0";
static const char* COLLECTOR_NAME_VALUE = "Aurelio Macasling";
static const unsigned long DEVICE_ID_VALUE = 2;  // Makilas barangay device
static const unsigned long BRGY_ID_VALUE = 2;    // Makilas barangay

// Device UID - unique identifier (MAC address)
static String getDeviceUID() {
#if defined(ARDUINO_ARCH_ESP32)
  uint64_t mac = ESP.getEfuseMac();
  char uid[17];
  sprintf(uid, "%012llX", mac);
  return String(uid);
#else
  return "UNKNOWN";
#endif
}

// Global variables for cached values
static uint32_t g_lastSyncEpoch = 0;
static uint32_t g_printCount = 0;

// Helper functions for DB operations
static void setDeviceInfoValue(const char* key, const String& value) {
  if (!db) {
    Serial.println(F("DB not open"));
    return;
  }
  char sql[256];
  String mac = getDeviceUID();
  String nowStr = getCurrentDateTimeString();
  if (strcmp(key, "print_count") == 0) {
    sprintf(sql, "UPDATE device_info SET print_count = %d, updated_at = '%s' WHERE device_mac = '%s';", value.toInt(), nowStr.c_str(), mac.c_str());
  } else if (strcmp(key, "last_sync_epoch") == 0) {
    sprintf(sql, "UPDATE device_info SET last_sync = '%s', updated_at = '%s' WHERE device_mac = '%s';", value.c_str(), nowStr.c_str(), mac.c_str());
  } else {
    // For other keys, do nothing or handle if needed
    return;
  }
  Serial.print(F("Executing SQL: "));
  Serial.println(sql);
  int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
  if (rc != SQLITE_OK) {
    Serial.printf("SQL error: %s\n", sqlite3_errmsg(db));
  } else {
    Serial.println(F("SQL executed successfully"));
  }
}

static String getDeviceInfoValue(const char* key) {
  if (!db) return "";
  if (strcmp(key, "device_id") == 0) return String(DEVICE_ID_VALUE);
  if (strcmp(key, "brgy_id") == 0) return String(BRGY_ID_VALUE);
  if (strcmp(key, "collector") == 0) return COLLECTOR_NAME_VALUE;
  char sql[128];
  const char* column = "";
  if (strcmp(key, "device_mac") == 0) column = "device_mac";
  else if (strcmp(key, "device_uid") == 0) column = "device_uid";
  else if (strcmp(key, "firmware_version") == 0) column = "firmware_version";
  else if (strcmp(key, "device_name") == 0) column = "device_name";
  else if (strcmp(key, "print_count") == 0) column = "print_count";
  else if (strcmp(key, "customer_count") == 0) column = "customer_count";
  else if (strcmp(key, "last_sync_epoch") == 0) column = "last_sync";
  else return "";
  
  sprintf(sql, "SELECT %s FROM device_info WHERE device_mac = '%s';", column, getDeviceUID().c_str());
  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) return "";
  String value = "";
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    if (sqlite3_column_type(stmt, 0) == SQLITE_INTEGER) {
      value = String(sqlite3_column_int(stmt, 0));
    } else {
      value = String((const char*)sqlite3_column_text(stmt, 0));
    }
  }
  sqlite3_finalize(stmt);
  return value;
}

static void loadDeviceInfoFromDB() {
  if (!db) return;
  g_lastSyncEpoch = getDeviceInfoValue("last_sync_epoch").toInt();
  g_printCount = getDeviceInfoValue("print_count").toInt();
  // Initialize defaults if not set
  if (g_lastSyncEpoch == 0) {
    setDeviceInfoValue("last_sync_epoch", "0");
  }
  // Do not set print_count to 0 here, as it should persist
}

static void initDeviceInfo() {
  loadDeviceInfoFromDB();
}

static void incrementPrintCount() {
  Serial.print(F("Incrementing print count from "));
  Serial.print(g_printCount);
  g_printCount++;
  Serial.print(F(" to "));
  Serial.println(g_printCount);
  setDeviceInfoValue("print_count", String(g_printCount));
}

static void setLastSyncEpoch(uint32_t epoch) {
  g_lastSyncEpoch = epoch;
  setDeviceInfoValue("last_sync_epoch", String(epoch));
}

static uint32_t getLastSyncEpoch() {
  return g_lastSyncEpoch;
}

static uint32_t countPendingReadings() {
  if (!db) return 0;

  const char *sql = "SELECT COUNT(*) FROM readings WHERE updated_at = created_at;";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) return 0;

  uint32_t count = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    count = sqlite3_column_int(stmt, 0);
  }

  sqlite3_finalize(stmt);
  return count;
}

static uint32_t countCustomers() {
  if (!db) return 0;

  const char *sql = "SELECT COUNT(*) FROM customers;";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) return 0;

  uint32_t count = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    count = sqlite3_column_int(stmt, 0);
  }

  sqlite3_finalize(stmt);
  return count;
}

static bool deviceInfoSdReady() {
  return (SD.cardType() != CARD_NONE);
}

static uint64_t sdTotalBytesSafe() {
#if defined(ARDUINO_ARCH_ESP32)
  if (!deviceInfoSdReady()) return 0;
  deselectTftSelectSd();
  return (uint64_t)SD.totalBytes();
#else
  return 0;
#endif
}

static uint64_t sdUsedBytesSafe() {
#if defined(ARDUINO_ARCH_ESP32)
  if (!deviceInfoSdReady()) return 0;
  deselectTftSelectSd();
  return (uint64_t)SD.usedBytes();
#else
  return 0;
#endif
}

static void exportDeviceInfoForSync() {
  uint32_t startMs = millis();
  Serial.println(F("BEGIN_DEVICE_INFO"));

  Serial.print(F("INFO|device_type|"));
  Serial.println(DEVICE_TYPE_VALUE);

  Serial.print(F("INFO|firmware_version|"));
  Serial.println(FIRMWARE_VERSION_VALUE);

  Serial.print(F("INFO|device_id|"));
  Serial.println(DEVICE_ID_VALUE);

  Serial.print(F("INFO|brgy_id|"));
  Serial.println(BRGY_ID_VALUE);

  Serial.print(F("INFO|device_uid|"));
  Serial.println(getDeviceUID());

  Serial.print(F("INFO|device_mac|"));
  Serial.println(getDeviceUID());

  Serial.print(F("INFO|collector|"));
  Serial.println(COLLECTOR_NAME_VALUE);

  Serial.print(F("INFO|last_sync_epoch|"));
  Serial.println((unsigned long)g_lastSyncEpoch);

  Serial.print(F("INFO|print_count|"));
  Serial.println((unsigned long)g_printCount);

  Serial.print(F("INFO|customer_count|"));
  Serial.println((unsigned long)countCustomers());

  Serial.print(F("INFO|pending_readings|"));
  Serial.println((unsigned long)countPendingReadings());

  // SD card status
  Serial.print(F("INFO|sd_present|"));
  Serial.println(deviceInfoSdReady() ? 1 : 0);

  uint64_t totalBytes = sdTotalBytesSafe();
  uint64_t usedBytes = sdUsedBytesSafe();
  uint64_t freeBytes = (totalBytes > usedBytes) ? (totalBytes - usedBytes) : 0;

  Serial.print(F("INFO|sd_total_bytes|"));
  Serial.println((unsigned long long)totalBytes);
  Serial.print(F("INFO|sd_used_bytes|"));
  Serial.println((unsigned long long)usedBytes);
  Serial.print(F("INFO|sd_free_bytes|"));
  Serial.println((unsigned long long)freeBytes);

  // ESP32 CPU / heap status
#if defined(ARDUINO_ARCH_ESP32)
  Serial.print(F("INFO|cpu_freq_mhz|"));
  Serial.println((unsigned long)getCpuFrequencyMhz());

  Serial.print(F("INFO|heap_free_bytes|"));
  Serial.println((unsigned long)ESP.getFreeHeap());

  Serial.print(F("INFO|heap_min_free_bytes|"));
  Serial.println((unsigned long)ESP.getMinFreeHeap());

  Serial.print(F("INFO|heap_max_alloc_bytes|"));
  Serial.println((unsigned long)ESP.getMaxAllocHeap());
#endif

  Serial.println(F("END_DEVICE_INFO"));

  uint32_t elapsedMs = millis() - startMs;
  Serial.print(F("Done. ("));
  Serial.print(elapsedMs);
  Serial.println(F(" ms)"));
}

#endif  // DEVICE_INFO_H
