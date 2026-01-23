#ifndef BARANGAY_SYNC_H
#define BARANGAY_SYNC_H

#include "../../database/barangay_database.h"

// ===== BARANGAY SYNC OPERATIONS =====

// Handle UPSERT_BARANGAY command
bool handleUpsertBarangay(String payload) {
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
  String barangay = payload.substring(p1 + 1, p2);
  String prefix = payload.substring(p2 + 1, p3);
  String nextNumberStr = payload.substring(p3 + 1, p4);
  String createdStr = payload.substring(p4 + 1, p5);
  String updatedStr = payload.substring(p5 + 1);

  idStr.trim();
  barangay.trim();
  prefix.trim();
  nextNumberStr.trim();
  createdStr.trim();
  updatedStr.trim();

  unsigned long brgyId = (unsigned long)idStr.toInt();
  unsigned long nextNumber = (unsigned long)nextNumberStr.toInt();
  unsigned long createdAt = (unsigned long)createdStr.toInt();
  unsigned long updatedAt = (unsigned long)updatedStr.toInt();

  if (upsertBarangayFromSync(brgyId, barangay, prefix, nextNumber, createdAt, updatedAt)) {
    Serial.print(F("ACK|UPSERT|"));
    Serial.println(barangay);
  } else {
    Serial.print(F("ERR|UPSERT_FAILED|BARANGAY|"));
    Serial.println(barangay);
  }
  return true;
}

#endif // BARANGAY_SYNC_H