#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

#include <SD.h>
#include "../configuration/config.h"
#include "../managers/sdcard_manager.h"

// Stored at SD so the website can show persistent device info.
// Format (key=value lines):
// device_type=...
// firmware_version=...
// last_sync_epoch=...
// print_count=...

static const char* DEVICE_INFO_FILE = "/WATER_DB/SETTINGS/device_info.txt";

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

static uint32_t g_lastSyncEpoch = 0;
static uint32_t g_printCount = 0;

// Forward declarations (needed because this file is a header and order matters in C++)
static bool deviceInfoSdReady();

static void ensureDeviceInfoDirs() {
  if (!deviceInfoSdReady()) return;
  deselectTftSelectSd();

  // Ensure the folder structure exists so SD.open(FILE_WRITE) succeeds.
  if (!SD.exists("/WATER_DB")) {
    SD.mkdir("/WATER_DB");
  }
  if (!SD.exists("/WATER_DB/SETTINGS")) {
    SD.mkdir("/WATER_DB/SETTINGS");
  }
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

static void writeDeviceInfoToSD() {
  if (!deviceInfoSdReady()) return;

  deselectTftSelectSd();

  ensureDeviceInfoDirs();

  // Best-effort overwrite by remove
  if (SD.exists(DEVICE_INFO_FILE)) {
    SD.remove(DEVICE_INFO_FILE);
  }

  File f = SD.open(DEVICE_INFO_FILE, FILE_WRITE);
  if (!f) return;

  f.print(F("device_type="));
  f.println(DEVICE_TYPE_VALUE);
  f.print(F("firmware_version="));
  f.println(FIRMWARE_VERSION_VALUE);
  f.print(F("device_id="));
  f.println(DEVICE_ID_VALUE);
  f.print(F("brgy_id="));
  f.println(BRGY_ID_VALUE);
  f.print(F("device_uid="));
  f.println(getDeviceUID());
  f.print(F("last_sync_epoch="));
  f.println((unsigned long)g_lastSyncEpoch);
  f.print(F("print_count="));
  f.println((unsigned long)g_printCount);

  f.close();
}

static void loadDeviceInfoFromSD() {
  if (!deviceInfoSdReady()) return;

  deselectTftSelectSd();

  ensureDeviceInfoDirs();
  if (!SD.exists(DEVICE_INFO_FILE)) {
    writeDeviceInfoToSD();
    return;
  }

  File f = SD.open(DEVICE_INFO_FILE, FILE_READ);
  if (!f) return;

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    int eq = line.indexOf('=');
    if (eq < 0) continue;

    String key = line.substring(0, eq);
    String val = line.substring(eq + 1);
    key.trim();
    val.trim();

    if (key == "last_sync_epoch") {
      g_lastSyncEpoch = (uint32_t)val.toInt();
    } else if (key == "print_count") {
      g_printCount = (uint32_t)val.toInt();
    }
  }

  f.close();
}

static void initDeviceInfo() {
  loadDeviceInfoFromSD();
}

static void incrementPrintCount() {
  g_printCount++;
  writeDeviceInfoToSD();
}

static void setLastSyncEpoch(uint32_t epoch) {
  g_lastSyncEpoch = epoch;
  writeDeviceInfoToSD();
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
