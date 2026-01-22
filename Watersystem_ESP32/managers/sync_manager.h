#ifndef SYNC_MANAGER_H
#define SYNC_MANAGER_H

#include "../database/customers_database.h"
#include "../database/device_info.h"
#include "../database/readings_database.h"
#include "../database/deduction_database.h"
#include "../database/customer_type_database.h"
#include "../configuration/config.h"
#include <ArduinoJson.h>
#include <vector>

// Callback for counting customers
static int countCallback(void *data, int argc, char **argv, char **azColName) {
  int* count = (int*)data;
  *count = atoi(argv[0]);
  return 0;
}

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

  if (raw.startsWith("UPSERT_CUSTOMERS_JSON|")) {
    // UPSERT_CUSTOMERS_JSON|<json_array>
    String payload = raw.substring(String("UPSERT_CUSTOMERS_JSON|").length());

    DynamicJsonDocument doc(131072); // 128KB for large customer lists
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.println(F("ERR|JSON_PARSE_FAILED"));
      return true;
    }

    JsonArray customers = doc.as<JsonArray>();
    bool allSuccess = true;
    for (JsonObject customer : customers) {
      String accountNo = customer["account_no"] | "";
      String name = customer["customer_name"] | "";
      String address = customer["address"] | "";
      unsigned long prev = customer["previous_reading"] | 0;
      String status = customer["status"] | "active";
      unsigned long typeId = customer["type_id"] | 1;
      unsigned long deductionId = customer["deduction_id"] | 0;
      unsigned long brgyId = customer["brgy_id"] | 1;

      if (!upsertCustomerFromSync(accountNo, name, address, prev, status, typeId, deductionId, brgyId)) {
        allSuccess = false;
        break;
      }
    }

    if (allSuccess) {
      Serial.print(F("ACK|UPSERT_CUSTOMERS_JSON|"));
      Serial.println(customers.size());
    } else {
      Serial.println(F("ERR|UPSERT_CUSTOMERS_JSON_FAILED"));
    }
    return true;
  }

  if (raw.startsWith("UPSERT_CUSTOMERS_JSON_CHUNK|")) {
    // UPSERT_CUSTOMERS_JSON_CHUNK|chunkIndex|totalChunks|json_chunk
    String payload = raw.substring(String("UPSERT_CUSTOMERS_JSON_CHUNK|").length());

    int p1 = payload.indexOf('|');
    int p2 = (p1 >= 0) ? payload.indexOf('|', p1 + 1) : -1;
    if (p1 < 0 || p2 < 0) {
      Serial.println(F("ERR|BAD_CHUNK_FORMAT"));
      return true;
    }

    int chunkIndex = payload.substring(0, p1).toInt();
    int totalChunks = payload.substring(p1 + 1, p2).toInt();
    String jsonChunk = payload.substring(p2 + 1);
    jsonChunk.replace("\\|", "|");

    // Parse the JSON chunk (it's an array of customers)
    DynamicJsonDocument doc(16384); // 16KB should be enough for 10 customers
    Serial.println(F("About to deserialize JSON"));
    DeserializationError error = deserializeJson(doc, jsonChunk);
    if (error) {
      Serial.println(F("ERR|JSON_PARSE_FAILED"));
      Serial.print(F("Error: "));
      Serial.println(error.c_str());
      return true;
    }
    Serial.println(F("JSON deserialized successfully"));

    JsonArray customers = doc.as<JsonArray>();
    bool allSuccess = true;

    Serial.print(F("Processing chunk "));
    Serial.println(chunkIndex);

    // Set synchronous to NORMAL for speed during sync (durability is still good)
    sqlite3_exec(db, "PRAGMA synchronous = NORMAL;", NULL, NULL, NULL);

    Serial.printf("Heap free before chunk: %d\n", ESP.getFreeHeap());

    // Begin transaction for this chunk
    int rc_begin = sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
    if (rc_begin != SQLITE_OK) {
      Serial.print(F("BEGIN failed for chunk "));
      Serial.print(chunkIndex);
      Serial.print(F(": "));
      Serial.println(sqlite3_errmsg(db));
      Serial.println(F("ERR|UPSERT_FAILED"));
      return true;
    }

    // Prepare statement once for all inserts in this chunk
    const char* sql = "INSERT INTO customers (account_no, customer_name, address, previous_reading, status, type_id, deduction_id, brgy_id, created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, datetime('now'), datetime('now'));";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
      Serial.print(F("SQLite prepare error: "));
      Serial.println(sqlite3_errmsg(db));
      sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
      Serial.println(F("ERR|UPSERT_FAILED"));
      return true;
    }

    for (JsonObject customer : customers) {
      String accountNo = customer["account_no"] | "";
      String name = customer["customer_name"] | "";
      String address = customer["address"] | "";
      unsigned long prev = customer["previous_reading"] | 0;
      String status = customer["status"] | "active";
      unsigned long typeId = customer["type_id"] | 1;
      unsigned long deductionId = customer["deduction_id"] | 0;
      unsigned long brgyId = customer["brgy_id"] | 1;

      // Reset and clear bindings for reuse
      sqlite3_reset(stmt);
      sqlite3_clear_bindings(stmt);

      sqlite3_bind_text(stmt, 1, accountNo.c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 3, address.c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_int64(stmt, 4, prev);
      sqlite3_bind_text(stmt, 5, status.c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_int64(stmt, 6, typeId);

      if (deductionId == 0) {
        sqlite3_bind_null(stmt, 7);
      } else {
        sqlite3_bind_int64(stmt, 7, deductionId);
      }

      sqlite3_bind_int64(stmt, 8, brgyId);

      rc = sqlite3_step(stmt);
      if (rc != SQLITE_DONE) {
        Serial.print(F("SQLite step error: "));
        Serial.println(sqlite3_errmsg(db));
        allSuccess = false;
        break;
      } else {
        Serial.print(F("Inserted customer: "));
        Serial.println(accountNo);
      }
    }

    sqlite3_finalize(stmt);

    if (allSuccess) {
      int rc_commit = sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
      if (rc_commit != SQLITE_OK) {
        Serial.print(F("COMMIT failed for chunk "));
        Serial.print(chunkIndex);
        Serial.print(F(": "));
        Serial.println(sqlite3_errmsg(db));
        Serial.println(F("ERR|UPSERT_FAILED"));
        return true;
      } else {
        Serial.print(F("Processed chunk "));
        Serial.println(chunkIndex);
      }
      Serial.print(F("ACK|CHUNK|"));
      Serial.println(chunkIndex);
      Serial.printf("Heap free after chunk: %d\n", ESP.getFreeHeap());
      Serial.println(F("Ready for next chunk"));
      Serial.flush();
      if (chunkIndex == totalChunks - 1) {
        // Last chunk, count customers in DB
        int customerCount = 0;
        sqlite3_exec(db, "SELECT COUNT(*) FROM customers;", countCallback, &customerCount, NULL);
        Serial.print(F("Total customers in DB after sync: "));
        Serial.println(customerCount);
        // Send final success
        Serial.print(F("ACK|UPSERT_CUSTOMERS_JSON|"));
        Serial.println(customers.size());
        Serial.flush();
      }
    } else {
      sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
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
      Serial.print(F("ERR|UPSERT_FAILED|DEDUCTION|"));
      Serial.println(name);
    }
    return true;
  }

  if (raw.startsWith("UPSERT_CUSTOMER_TYPE|")) {
    // UPSERT_CUSTOMER_TYPE|type_id|type_name|rate_per_m3|min_m3|min_charge|penalty|created_at|updated_at
    String payload = raw.substring(String("UPSERT_CUSTOMER_TYPE|").length());

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

  // If command not recognized, log it for debugging
  Serial.print(F("UNKNOWN_COMMAND: "));
  Serial.println(raw);
  return false;
}

#endif // SYNC_MANAGER_H