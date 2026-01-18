#ifndef SYNC_MANAGER_H
#define SYNC_MANAGER_H

#include "../database/customers_database.h"
#include "../database/device_info.h"
#include "../database/readings_database.h"
#include "../database/deduction_database.h"
#include "../database/customer_type_database.h"
#include "../configuration/config.h"

// Function to handle all sync protocol commands
bool handleSyncCommands(String raw) {
  // ---- Sync protocol (do NOT uppercase; payload may be mixed-case) ----
  if (raw == "EXPORT_CUSTOMERS") {
    Serial.println(F("Exporting customers..."));
    exportCustomersForSync();
    return true;
  }

  if (raw == "EXPORT_DEVICE_INFO") {
    Serial.println(F("Exporting device info..."));
    exportDeviceInfoForSync();
    return true;
  }

  if (raw.startsWith("SET_TIME|")) {
    // SET_TIME|<epoch_seconds>
    String payload = raw.substring(String("SET_TIME|").length());
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

  if (raw.startsWith("SET_LAST_SYNC|")) {
    // SET_LAST_SYNC|<epoch_seconds>
    String payload = raw.substring(String("SET_LAST_SYNC|").length());
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

  if (raw == "EXPORT_READINGS") {
    Serial.println(F("Exporting readings..."));
    exportReadingsForSync();
    return true;
  }

  if (raw == "READINGS_SYNCED") {
    bool ok = markAllReadingsSynced();
    if (ok) {
      setLastSyncEpoch(deviceEpochNow());
      Serial.println(F("ACK|READINGS_SYNCED"));
    } else {
      Serial.println(F("ERR|READINGS_SYNC_FAILED"));
    }
    return true;
  }

  if (raw.startsWith("UPSERT_CUSTOMER|")) {
    // UPSERT_CUSTOMER|account_no|customer_name|address|previous_reading|status|type_id|deduction_id|brgy_id
    String payload = raw.substring(String("UPSERT_CUSTOMER|").length());

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

    String accountNo = payload.substring(0, p1);
    String name = payload.substring(p1 + 1, p2);
    String address = payload.substring(p2 + 1, p3);
    String prevStr = payload.substring(p3 + 1, p4);
    String status = payload.substring(p4 + 1, p5);
    String typeIdStr = payload.substring(p5 + 1, p6);
    String deductionIdStr = payload.substring(p6 + 1, p7);
    String brgyIdStr = payload.substring(p7 + 1);

    accountNo.trim();
    name.trim();
    address.trim();
    prevStr.trim();
    status.trim();
    typeIdStr.trim();
    deductionIdStr.trim();
    brgyIdStr.trim();

    unsigned long prev = (unsigned long)prevStr.toInt();
    unsigned long typeId = (unsigned long)typeIdStr.toInt();
    unsigned long deductionId = (unsigned long)deductionIdStr.toInt();
    unsigned long brgyId = (unsigned long)brgyIdStr.toInt();

    if (upsertCustomerFromSync(accountNo, name, address, prev, status, typeId, deductionId, brgyId)) {
      Serial.print(F("ACK|UPSERT|"));
      Serial.println(accountNo);
    } else {
      Serial.println(F("ERR|UPSERT_FAILED"));
    }
    return true;
  }

  if (raw.startsWith("UPSERT_DEDUCTION|")) {
    // UPSERT_DEDUCTION|deduction_id|name|type|value|created_at|updated_at
    String payload = raw.substring(String("UPSERT_DEDUCTION|").length());

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
      Serial.println(F("ERR|UPSERT_FAILED"));
    }
    return true;
  }

  if (raw.startsWith("UPSERT_CUSTOMER_TYPE|")) {
    // UPSERT_CUSTOMER_TYPE|type_id|type_name|rate_per_m3|min_m3|min_charge|created_at|updated_at
    String payload = raw.substring(String("UPSERT_CUSTOMER_TYPE|").length());

    int p1 = payload.indexOf('|');
    int p2 = (p1 >= 0) ? payload.indexOf('|', p1 + 1) : -1;
    int p3 = (p2 >= 0) ? payload.indexOf('|', p2 + 1) : -1;
    int p4 = (p3 >= 0) ? payload.indexOf('|', p3 + 1) : -1;
    int p5 = (p4 >= 0) ? payload.indexOf('|', p4 + 1) : -1;
    int p6 = (p5 >= 0) ? payload.indexOf('|', p5 + 1) : -1;
    if (p1 < 0 || p2 < 0 || p3 < 0 || p4 < 0 || p5 < 0 || p6 < 0) {
      Serial.println(F("ERR|BAD_FORMAT"));
      return true;
    }

    String idStr = payload.substring(0, p1);
    String typeName = payload.substring(p1 + 1, p2);
    String rateStr = payload.substring(p2 + 1, p3);
    String minM3Str = payload.substring(p3 + 1, p4);
    String minChargeStr = payload.substring(p4 + 1, p5);
    String createdStr = payload.substring(p5 + 1, p6);
    String updatedStr = payload.substring(p6 + 1);

    idStr.trim();
    typeName.trim();
    rateStr.trim();
    minM3Str.trim();
    minChargeStr.trim();
    createdStr.trim();
    updatedStr.trim();

    unsigned long typeId = (unsigned long)idStr.toInt();
    float ratePerM3 = rateStr.toFloat();
    unsigned long minM3 = (unsigned long)minM3Str.toInt();
    float minCharge = minChargeStr.toFloat();
    unsigned long createdAt = (unsigned long)createdStr.toInt();
    unsigned long updatedAt = (unsigned long)updatedStr.toInt();

    if (upsertCustomerTypeFromSync(typeId, typeName, ratePerM3, minM3, minCharge, createdAt, updatedAt)) {
      Serial.print(F("ACK|UPSERT|"));
      Serial.println(typeName);
    } else {
      Serial.println(F("ERR|UPSERT_FAILED"));
    }
    return true;
  }

  if (raw == "RELOAD_SD") {
    Serial.println(F("Reloading SD card and settings..."));
    initSDCard();
    Serial.println(F("SD reloaded successfully"));
    return true;
  }

  if (raw.startsWith("REMOVE_CUSTOMER|")) {
    // REMOVE_CUSTOMER|<account_no>
    String payload = raw.substring(String("REMOVE_CUSTOMER|").length());
    payload.trim();
    if (payload.length() > 0) {
      if (removeCustomerByAccount(payload)) {
        Serial.print(F("ACK|REMOVE_CUSTOMER|"));
        Serial.println(payload);
      } else {
        Serial.println(F("ERR|CUSTOMER_NOT_FOUND"));
      }
    } else {
      Serial.println(F("ERR|BAD_ACCOUNT_NO"));
    }
    return true;
  }

  if (raw == "FORMAT_SD") {
    Serial.println(F("Formatting SD card... This will delete all data!"));
    if (formatSDCard()) {
      Serial.println(F("ACK|FORMAT_SD"));
      Serial.println(F("SD card formatted successfully. Reinitializing..."));
      initSDCard();
    } else {
      Serial.println(F("ERR|FORMAT_FAILED"));
    }
    return true;
  }

  if (raw == "RESTART_DEVICE") {
    Serial.println(F("Restarting device..."));
    Serial.println(F("ACK|RESTART_DEVICE"));
    delay(500);
    ESP.restart();
    return true;
  }

  return false;
}

#endif // SYNC_MANAGER_H