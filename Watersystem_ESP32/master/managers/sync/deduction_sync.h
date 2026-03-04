#ifndef DEDUCTION_SYNC_H
#define DEDUCTION_SYNC_H

#include "../../database/deduction_database.h"

// ===== DEDUCTION SYNC OPERATIONS =====

// Handle UPSERT_DEDUCTION command
bool handleUpsertDeduction(String payload) {
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
  String name = payload.substring(p1 + 1, p2);
  String type = payload.substring(p2 + 1, p3);
  String valueStr = payload.substring(p3 + 1, p4);
  String createdStr = payload.substring(p4 + 1, p5);
  String updatedStr = payload.substring(p5 + 1);

  idStr.trim();
  name.trim();
  type.trim();
  valueStr.trim();
  createdStr.trim();
  updatedStr.trim();

  unsigned long deductionId = (unsigned long)idStr.toInt();
  float value = valueStr.toFloat();
  unsigned long createdAt = (unsigned long)createdStr.toInt();
  unsigned long updatedAt = (unsigned long)updatedStr.toInt();

  if (upsertDeductionFromSync(deductionId, name, type, value, createdAt, updatedAt)) {
    Serial.print(F("ACK|UPSERT|"));
    Serial.println(name);
  } else {
    Serial.print(F("ERR|UPSERT_FAILED|DEDUCTION|"));
    Serial.println(name);
  }
  return true;
}

#endif // DEDUCTION_SYNC_H