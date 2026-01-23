#ifndef SYNC_MANAGER_H
#define SYNC_MANAGER_H

#include "../database/customers_database.h"
#include "../database/device_info.h"
#include "../database/readings_database.h"
#include "../database/bill_database.h"
#include "../database/deduction_database.h"
#include "../database/customer_type_database.h"
#include "../database/barangay_database.h"
#include "../configuration/config.h"
#include <ArduinoJson.h>
#include <vector>
#include <SD.h>

// Include organized sync handlers
#include "sync/device_sync.h"
#include "sync/reading_sync.h"
#include "sync/deduction_sync.h"
#include "sync/barangay_sync.h"
#include "sync/customer_type_sync.h"

// Callback for counting customers
static int countCallback(void *data, int argc, char **argv, char **azColName) {
  int* count = (int*)data;
  *count = atoi(argv[0]);
  return 0;
}

// Function to handle all sync protocol commands
bool handleSyncCommands(String raw) {
  // ---- Sync protocol (do NOT uppercase; payload may be mixed-case) ----

  if (raw == "EXPORT_DEVICE_INFO") {
    return handleExportDeviceInfo();
  }

  if (raw == "DROP_DB") {
    return handleDropDatabase();
  }

  if (raw.startsWith("SET_TIME|")) {
    String payload = raw.substring(String("SET_TIME|").length());
    return handleSetTime(payload);
  }

  if (raw.startsWith("SET_LAST_SYNC|")) {
    String payload = raw.substring(String("SET_LAST_SYNC|").length());
    return handleSetLastSync(payload);
  }

  if (raw == "EXPORT_READINGS") {
    return handleExportReadings();
  }

  if (raw == "EXPORT_BILLS") {
    return handleExportBills();
  }

  if (raw == "READINGS_SYNCED") {
    return handleReadingsSynced();
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
  if (raw.startsWith("UPSERT_NEW_CUSTOMER_JSON_CHUNK|")) {
    // UPSERT_NEW_CUSTOMER_JSON_CHUNK|chunkIndex|totalChunks|json_chunk
    String payload = raw.substring(String("UPSERT_NEW_CUSTOMER_JSON_CHUNK|").length());

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
    Serial.println(F("About to deserialize JSON for new customers"));
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

    Serial.print(F("Processing new customer chunk "));
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
        Serial.print(F("Inserted new customer: "));
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
        Serial.print(F("Processed new customer chunk "));
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
        Serial.print(F("Total customers in DB after new sync: "));
        Serial.println(customerCount);
        // Send final success
        Serial.print(F("ACK|UPSERT_NEW_CUSTOMER_JSON|"));
        Serial.println(customers.size());
        Serial.flush();
      }
    } else {
      sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
      Serial.println(F("ERR|UPSERT_FAILED"));
    }
    return true;
  }

  if (raw.startsWith("UPSERT_UPDATED_CUSTOMER_JSON_CHUNK|")) {
    // UPSERT_UPDATED_CUSTOMER_JSON_CHUNK|chunkIndex|totalChunks|json_chunk
    String payload = raw.substring(String("UPSERT_UPDATED_CUSTOMER_JSON_CHUNK|").length());

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
    Serial.println(F("About to deserialize JSON for updated customers"));
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

    Serial.print(F("Processing updated customer chunk "));
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

    // Prepare statement once for all updates in this chunk
    const char* sql = "UPDATE customers SET customer_name = ?, address = ?, previous_reading = ?, status = ?, type_id = ?, deduction_id = ?, brgy_id = ?, updated_at = datetime('now') WHERE account_no = ?;";
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

      sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 2, address.c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_int64(stmt, 3, prev);
      sqlite3_bind_text(stmt, 4, status.c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_int64(stmt, 5, typeId);

      if (deductionId == 0) {
        sqlite3_bind_null(stmt, 6);
      } else {
        sqlite3_bind_int64(stmt, 6, deductionId);
      }

      sqlite3_bind_int64(stmt, 7, brgyId);
      sqlite3_bind_text(stmt, 8, accountNo.c_str(), -1, SQLITE_TRANSIENT);

      rc = sqlite3_step(stmt);
      if (rc != SQLITE_DONE) {
        Serial.print(F("SQLite step error: "));
        Serial.println(sqlite3_errmsg(db));
        allSuccess = false;
        break;
      } else {
        Serial.print(F("Updated customer: "));
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
        Serial.print(F("Processed updated customer chunk "));
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
        Serial.print(F("Total customers in DB after update sync: "));
        Serial.println(customerCount);
        // Send final success
        Serial.print(F("ACK|UPSERT_UPDATED_CUSTOMER_JSON|"));
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
    String payload = raw.substring(String("UPSERT_DEDUCTION|").length());
    return handleUpsertDeduction(payload);
  }

  if (raw.startsWith("UPSERT_BARANGAY|")) {
    String payload = raw.substring(String("UPSERT_BARANGAY|").length());
    return handleUpsertBarangay(payload);
  }

  if (raw.startsWith("UPSERT_CUSTOMER_TYPE|")) {
    String payload = raw.substring(String("UPSERT_CUSTOMER_TYPE|").length());
    return handleUpsertCustomerType(payload);
  }

  if (raw == "RELOAD_SD") {
    return handleReloadSD();
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
    return handleFormatSD();
  }

  if (raw == "RESTART_DEVICE") {
    return handleRestartDevice();
  }

  // If command not recognized, log it for debugging
  Serial.print(F("UNKNOWN_COMMAND: "));
  Serial.println(raw);
  return false;
}

#endif // SYNC_MANAGER_H