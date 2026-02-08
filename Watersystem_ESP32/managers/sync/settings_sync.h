#ifndef SETTINGS_SYNC_H
#define SETTINGS_SYNC_H

#include "../../database/settings_database.h"

// ===== SETTINGS SYNC OPERATIONS =====

// Handle UPSERT_SETTINGS command
bool handleUpsertSettings(String payload) {
  int p1 = payload.indexOf('|');
  int p2 = (p1 >= 0) ? payload.indexOf('|', p1 + 1) : -1;
  int p3 = (p2 >= 0) ? payload.indexOf('|', p2 + 1) : -1;
  int p4 = (p3 >= 0) ? payload.indexOf('|', p3 + 1) : -1;
  int p5 = (p4 >= 0) ? payload.indexOf('|', p4 + 1) : -1;
  if (p1 < 0 || p2 < 0 || p3 < 0 || p4 < 0 || p5 < 0) {
    Serial.println(F("ERR|BAD_FORMAT"));
    return true;
  }

  String idStr = payload.substring(0, p1);
  String billDueDaysStr = payload.substring(p1 + 1, p2);
  String disconnectionDaysStr = payload.substring(p2 + 1, p3);
  String createdStr = payload.substring(p3 + 1, p4);
  String updatedStr = payload.substring(p4 + 1);

  idStr.trim();
  billDueDaysStr.trim();
  disconnectionDaysStr.trim();
  createdStr.trim();
  updatedStr.trim();

  unsigned long settingId = (unsigned long)idStr.toInt();
  int billDueDays = billDueDaysStr.toInt();
  int disconnectionDays = disconnectionDaysStr.toInt();
  unsigned long createdAt = (unsigned long)createdStr.toInt();
  unsigned long updatedAt = (unsigned long)updatedStr.toInt();

  if (upsertSettingFromSync(settingId, billDueDays, disconnectionDays, createdAt, updatedAt)) {
    Serial.println(F("ACK|UPSERT|Settings"));
  } else {
    Serial.println(F("ERR|UPSERT_FAILED|SETTINGS"));
  }
  return true;
}

#endif // SETTINGS_SYNC_H