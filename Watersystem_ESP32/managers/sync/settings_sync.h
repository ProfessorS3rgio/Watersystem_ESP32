#ifndef SETTINGS_SYNC_H
#define SETTINGS_SYNC_H

#include "../../database/settings_database.h"

// ===== SETTINGS SYNC OPERATIONS =====

// Handle UPSERT_SETTINGS command
bool handleUpsertSettings(String payload) {
  int p1 = payload.indexOf('|');
  int p2 = (p1 >= 0) ? payload.indexOf('|', p1 + 1) : -1;
  if (p1 < 0 || p2 < 0) {
    Serial.println(F("ERR|BAD_FORMAT"));
    return true;
  }

  String idStr = payload.substring(0, p1);
  String billDueDaysStr = payload.substring(p1 + 1, p2);
  String disconnectionDaysStr = payload.substring(p2 + 1);

  idStr.trim();
  billDueDaysStr.trim();
  disconnectionDaysStr.trim();

  unsigned long settingId = (unsigned long)idStr.toInt();
  int billDueDays = billDueDaysStr.toInt();
  int disconnectionDays = disconnectionDaysStr.toInt();

  if (upsertSettingFromSync(settingId, billDueDays, disconnectionDays)) {
    Serial.println(F("ACK|UPSERT|Settings"));
  } else {
    Serial.println(F("ERR|UPSERT_FAILED|SETTINGS"));
  }
  return true;
}

#endif // SETTINGS_SYNC_H