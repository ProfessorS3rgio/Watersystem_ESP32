#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

#include <SD.h>
#include "../configuration/config.h"
#include "../managers/sdcard_manager.h"

// Device info stored in PSV file on SD card: /WATER_DB/device_info.psv
// Format: key|value per line

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

static void ensureDeviceInfoDir() {
  if (!SD.exists(DB_ROOT)) {
    SD.mkdir(DB_ROOT);
  }
}


// Helper function to initialize device info file with defaults
static void initializeDeviceInfoFile() {
  if (!ensureSdMounted()) return;
  
  ensureDeviceInfoDir();
  
  // Try to open as READ first to check if it exists
  File existing = SD.open(DEVICE_INFO_FILE, FILE_READ);
  if (existing) {
    Serial.println(F("[DEVICE_INFO] File already exists, skipping creation"));
    existing.close();
    return;
  }

  Serial.println(F("[DEVICE_INFO] Creating new device info file..."));
  File file = SD.open(DEVICE_INFO_FILE, FILE_WRITE);
  if (!file) {
    Serial.println(F("[DEVICE_INFO] ERROR: Could not create file!"));
    return;
  }

  file.println("device_id|" + String(DEVICE_ID_VALUE));
  file.println("brgy_id|" + String(BRGY_ID_VALUE));
  file.println("device_mac|" + getDeviceUID());
  file.println("device_uid|" + getDeviceUID());
  file.println("firmware_version|" + String(FIRMWARE_VERSION_VALUE));
  file.println("device_name|ESP32 Water Device");
  file.println("print_count|0");
  file.println("customer_count|0");
  file.println("last_sync_epoch|0");

  file.close();
  Serial.println(F("[DEVICE_INFO] File created successfully"));
}

// Helper function to read value for a key from file
static String readDeviceInfoValue(const char* key) {
  if (!ensureSdMounted()) return "";
  
  File file = SD.open(DEVICE_INFO_FILE, FILE_READ);
  if (!file) return "";

  String value = "";
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    int sep = line.indexOf('|');
    if (sep == -1) continue;
    String k = line.substring(0, sep);
    if (k == key) {
      value = line.substring(sep + 1);
      break;
    }
  }
  file.close();
  return value;
}

// Helper function to write value for a key to file
static void writeDeviceInfoValue(const char* key, const String& value) {
  if (!ensureSdMounted()) return;

  File file = SD.open(DEVICE_INFO_FILE, FILE_READ);
  if (!file) return;

  String content = "";
  bool found = false;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    int sep = line.indexOf('|');
    if (sep != -1) {
      String k = line.substring(0, sep);
      if (k == key) {
        content += k + "|" + value + "\n";
        found = true;
      } else {
        content += line + "\n";
      }
    } else {
      content += line + "\n";
    }
  }
  file.close();

  if (!found) {
    content += String(key) + "|" + value + "\n";
  }

  file = SD.open(DEVICE_INFO_FILE, FILE_WRITE);
  if (file) {
    file.print(content);
    file.close();
  }
}

// Helper functions for DB operations (now file-based)
static void setDeviceInfoValue(const char* key, const String& value) {
  writeDeviceInfoValue(key, value);
}

static String getDeviceInfoValue(const char* key) {
  return readDeviceInfoValue(key);
}

static void loadDeviceInfoFromFile() {
  g_lastSyncEpoch = getDeviceInfoValue("last_sync_epoch").toInt();
  g_printCount = getDeviceInfoValue("print_count").toInt();
  // Values are initialized in file, so no need to set defaults here
}

static void initDeviceInfo() {
  initializeDeviceInfoFile();
  loadDeviceInfoFromFile();
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
