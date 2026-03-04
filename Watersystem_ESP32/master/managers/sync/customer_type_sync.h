#ifndef CUSTOMER_TYPE_SYNC_H
#define CUSTOMER_TYPE_SYNC_H

#include "../../database/customer_type_database.h"

// ===== CUSTOMER TYPE SYNC OPERATIONS =====

// Handle UPSERT_CUSTOMER_TYPE command
bool handleUpsertCustomerType(String payload) {
  int p1 = payload.indexOf('|');
  int p2 = (p1 >= 0) ? payload.indexOf('|', p1 + 1) : -1;
  int p3 = (p2 >= 0) ? payload.indexOf('|', p2 + 1) : -1;
  int p4 = (p3 >= 0) ? payload.indexOf('|', p3 + 1) : -1;
  int p5 = (p4 >= 0) ? payload.indexOf('|', p4 + 1) : -1;
  int p6 = (p5 >= 0) ? payload.indexOf('|', p5 + 1) : -1;
  int p7 = (p6 >= 0) ? payload.indexOf('|', p6 + 1) : -1;
  if (p1 < 0 || p2 < 0 || p3 < 0 || p4 < 0 || p5 < 0 || p6 < 0 || p7 < 0) {
    Serial.println(F("ERR|BAD_FORMAT"));
    return true;
  }

  String idStr = payload.substring(0, p1);
  String typeName = payload.substring(p1 + 1, p2);
  String rateStr = payload.substring(p2 + 1, p3);
  String minM3Str = payload.substring(p3 + 1, p4);
  String minChargeStr = payload.substring(p4 + 1, p5);
  String penaltyStr = payload.substring(p5 + 1, p6);
  String createdStr = payload.substring(p6 + 1, p7);
  String updatedStr = payload.substring(p7 + 1);

  idStr.trim();
  typeName.trim();
  rateStr.trim();
  minM3Str.trim();
  minChargeStr.trim();
  penaltyStr.trim();
  createdStr.trim();
  updatedStr.trim();

  unsigned long typeId = (unsigned long)idStr.toInt();
  float ratePerM3 = rateStr.toFloat();
  unsigned long minM3 = (unsigned long)minM3Str.toInt();
  float minCharge = minChargeStr.toFloat();
  float penalty = penaltyStr.toFloat();
  unsigned long createdAt = (unsigned long)createdStr.toInt();
  unsigned long updatedAt = (unsigned long)updatedStr.toInt();

  if (upsertCustomerTypeFromSync(typeId, typeName, ratePerM3, minM3, minCharge, penalty, createdAt, updatedAt)) {
    Serial.print(F("ACK|UPSERT|"));
    Serial.println(typeName);
  } else {
    Serial.print(F("ERR|UPSERT_FAILED|CUSTOMER_TYPE|"));
    Serial.println(typeName);
  }
  return true;
}

#endif // CUSTOMER_TYPE_SYNC_H