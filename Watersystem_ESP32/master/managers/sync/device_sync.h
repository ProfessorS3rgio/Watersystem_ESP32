#ifndef DEVICE_SYNC_H
#define DEVICE_SYNC_H

#include "../../database/device_info.h"
#include "../../configuration/config.h"
#include <SD.h>

// ===== DEVICE SYNC OPERATIONS =====

// Handle EXPORT_DEVICE_INFO command
bool handleExportDeviceInfo() {
  Serial.println(F("Exporting device info..."));
  exportDeviceInfoForSync();
  return true;
}

// Handle DROP_DB command
bool handleDropDatabase() {
  Serial.println(F("Dropping database..."));
  if (db) {
    sqlite3_close(db);
    db = nullptr;
  }
  if (SD.remove(DB_PATH)) {
    Serial.println(F("ACK|DROP_DB"));
    // Reinitialize database
    initDatabase();
    initDeviceInfo();
    Serial.println(F("Database reinitialized after drop."));
  } else {
    Serial.println(F("ERR|DROP_DB_FAILED"));
  }
  return true;
}

// Handle SET_TIME command
bool handleSetTime(String payload) {
  payload.trim();
  uint32_t epoch = (uint32_t)payload.toInt();
  if (epoch > 0) {
    setDeviceEpoch(epoch);
    Serial.print(F("ACK|SET_TIME|"));
    Serial.println(epoch);
  } else {
    Serial.println(F("ERR|BAD_TIME"));
  }
  return true;
}

// Handle SET_LAST_SYNC command
bool handleSetLastSync(String payload) {
  payload.trim();
  uint32_t epoch = (uint32_t)payload.toInt();
  if (epoch > 0) {
    setLastSyncEpoch(epoch);
    Serial.print(F("ACK|SET_LAST_SYNC|"));
    Serial.println(epoch);
  } else {
    Serial.println(F("ERR|BAD_LAST_SYNC"));
  }
  return true;
}

// Handle RELOAD_SD command
bool handleReloadSD() {
  Serial.println(F("Reloading SD card and settings..."));
  initSDCard();
  Serial.println(F("SD reloaded successfully"));
  return true;
}

// Handle FORMAT_SD command
bool handleFormatSD() {
  Serial.println(F("Formatting SD card... This will delete all data!"));
  if (formatSDCard()) {
    Serial.println(F("ACK|FORMAT_SD"));
    Serial.println(F("SD card formatted successfully. Reinitializing..."));
    // Close database since file was deleted
    if (db) {
      sqlite3_close(db);
      db = nullptr;
    }
    initSDCard();
    initDatabase();
    initDeviceInfo();
    Serial.println(F("Database reinitialized after format."));
  } else {
    Serial.println(F("ERR|FORMAT_FAILED"));
  }
  return true;
}

// Handle RESTART_DEVICE command
bool handleRestartDevice() {
  Serial.println(F("Restarting device..."));
  Serial.println(F("ACK|RESTART_DEVICE"));
  delay(500);
  ESP.restart();
  return true;
}

#endif // DEVICE_SYNC_H