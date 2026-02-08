#ifndef SETTINGS_DATABASE_H
#define SETTINGS_DATABASE_H

#include <SD.h>
#include "../configuration/config.h"
#include "sync_utils.h"
#include <sqlite3.h>
#include <vector>
#include "database_manager.h"

// ===== SETTINGS DATA STRUCTURE =====
struct Setting {
  int id;
  int bill_due_days;
  int disconnection_days;
  String created_at;
  String updated_at;
};

// ===== SETTINGS DATABASE =====
std::vector<Setting> settings;

static int loadSettingCallback(void *data, int argc, char **argv, char **azColName) {
  Setting s;
  s.id = atoi(argv[0]);
  s.bill_due_days = atoi(argv[1]);
  s.disconnection_days = atoi(argv[2]);
  s.created_at = argv[3];
  s.updated_at = argv[4];
  settings.push_back(s);
  return 0;
}

void loadSettingsFromDB() {
  settings.clear();
  const char *sql = "SELECT id, bill_due_days, disconnection_days, created_at, updated_at FROM settings;";
  sqlite3_exec(db, sql, loadSettingCallback, NULL, NULL);
}

void initSettingsDatabase() {
  loadSettingsFromDB();
}

// ===== UPSERT SETTING FROM SYNC =====
bool upsertSettingFromSync(unsigned long settingId, int billDueDays, int disconnectionDays) {
  char sql[512];
  // Store human-readable RTC-backed timestamps, consistent with other tables/viewers.
  String nowStr = getCurrentDateTimeString();
  sprintf(sql, "INSERT OR REPLACE INTO settings (id, bill_due_days, disconnection_days, created_at, updated_at, synced, last_sync) VALUES (%lu, %d, %d, '%s', '%s', 1, '%s');",
          settingId, billDueDays, disconnectionDays, nowStr.c_str(), nowStr.c_str(), nowStr.c_str());
  int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
  if (rc == SQLITE_OK) {
    loadSettingsFromDB(); // Reload vector
    return true;
  }
  return false;
}

// ===== GET SETTINGS =====
Setting* getSettings() {
  if (settings.size() > 0) {
    return &settings[0];
  }
  return nullptr;
}

#endif  // SETTINGS_DATABASE_H