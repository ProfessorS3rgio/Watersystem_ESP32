#ifndef DEVICE_SYNC_H
#define DEVICE_SYNC_H
#include "../sync_transport.h"

#include "../../database/device_info.h"
#include "../../configuration/config.h"
#include <SD.h>

// ===== DEVICE SYNC OPERATIONS =====

// Handle EXPORT_DEVICE_INFO command
bool handleExportDeviceInfo() {
  syncOutput().println(F("Exporting device info..."));
  exportDeviceInfoForSync(syncOutput());
  return true;
}

// Handle DROP_DB command
bool handleDropDatabase() {
  syncOutput().println(F("Dropping database..."));
  if (db) {
    sqlite3_close(db);
    db = nullptr;
  }
  if (SD.remove(DB_PATH)) {
    syncOutput().println(F("ACK|DROP_DB"));
    // Reinitialize database
    initDatabase();
    initDeviceInfo();
    syncOutput().println(F("Database reinitialized after drop."));
  } else {
    syncOutput().println(F("ERR|DROP_DB_FAILED"));
  }
  return true;
}

// Handle SET_TIME command
bool handleSetTime(String payload) {
  payload.trim();
  uint32_t epoch = (uint32_t)payload.toInt();
  if (epoch > 0) {
    setDeviceEpoch(epoch);
    syncOutput().print(F("ACK|SET_TIME|"));
    syncOutput().println(epoch);
  } else {
    syncOutput().println(F("ERR|BAD_TIME"));
  }
  return true;
}

// Handle SET_LAST_SYNC command
bool handleSetLastSync(String payload) {
  payload.trim();
  uint32_t epoch = (uint32_t)payload.toInt();
  if (epoch > 0) {
    setLastSyncEpoch(epoch);
    syncOutput().print(F("ACK|SET_LAST_SYNC|"));
    syncOutput().println(epoch);
  } else {
    syncOutput().println(F("ERR|BAD_LAST_SYNC"));
  }
  return true;
}

// Handle RELOAD_SD command
bool handleReloadSD() {
  syncOutput().println(F("Reloading SD card and settings..."));
  initSDCard();
  syncOutput().println(F("SD reloaded successfully"));
  return true;
}

// Handle FORMAT_SD command
bool handleFormatSD() {
  syncOutput().println(F("Formatting SD card... This will delete all data!"));
  if (formatSDCard()) {
    syncOutput().println(F("ACK|FORMAT_SD"));
    syncOutput().println(F("SD card formatted successfully. Reinitializing..."));
    // Close database since file was deleted
    if (db) {
      sqlite3_close(db);
      db = nullptr;
    }
    initSDCard();
    initDatabase();
    initDeviceInfo();
    syncOutput().println(F("Database reinitialized after format."));
  } else {
    syncOutput().println(F("ERR|FORMAT_FAILED"));
  }
  return true;
}

// Handle RESTART_DEVICE command
bool handleRestartDevice() {
  syncOutput().println(F("Restarting device..."));
  syncOutput().println(F("ACK|RESTART_DEVICE"));
  delay(500);
  ESP.restart();
  return true;
}

#endif // DEVICE_SYNC_H
