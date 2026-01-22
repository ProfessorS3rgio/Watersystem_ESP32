#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

#include <SD.h>
#include "../configuration/config.h"
#include "../managers/sdcard_manager.h"
#include <sqlite3.h>

// Device info now stored in SQLite database table 'device_info'
// Table: (key TEXT PRIMARY KEY, value TEXT, created_at TEXT, updated_at TEXT)

static const char* DEVICE_TYPE_VALUE = "ESP32 Water System";
static const char* FIRMWARE_VERSION_VALUE = "v1.0.0";
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
  if (!db) return;
  char sql[256];
  sprintf(sql, "INSERT OR REPLACE INTO device_info (key, value, created_at, updated_at) VALUES ('%s', '%s', datetime('now'), datetime('now'));", key, value.c_str());
  sqlite3_exec(db, sql, NULL, NULL, NULL);
}

static String getDeviceInfoValue(const char* key) {
  if (!db) return "";
  char sql[128];
  sprintf(sql, "SELECT value FROM device_info WHERE key = '%s';", key);
  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) return "";
  String value = "";
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    value = String((const char*)sqlite3_column_text(stmt, 0));
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
  if (g_printCount == 0) {
    setDeviceInfoValue("print_count", "0");
  }
}

static void initDeviceInfo() {
  loadDeviceInfoFromDB();
}

static void incrementPrintCount() {
  g_printCount++;
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

  Serial.print(F("INFO|last_sync_epoch|"));
  Serial.println((unsigned long)g_lastSyncEpoch);

  Serial.print(F("INFO|print_count|"));
  Serial.println((unsigned long)g_printCount);

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
