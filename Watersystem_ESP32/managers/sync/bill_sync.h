#ifndef BILL_SYNC_H
#define BILL_SYNC_H

#include "../../database/bill_database.h"
#include "../../configuration/config.h"
#include <ArduinoJson.h>
#include <vector>

// Struct to hold bill data temporarily
struct BillDataSync {
  String referenceNumber;
  unsigned long customerId;
  unsigned long readingId;
  String deviceUid;
  String billDate;
  float ratePerM3;
  float charges;
  float penalty;
  float totalDue;
  String dueDate;
  String status;
};

// Callback for counting bills
static int billCountCallback(void *data, int argc, char **argv, char **azColName) {
  int* count = (int*)data;
  *count = atoi(argv[0]);
  return 0;
}

// ===== BILL SYNC OPERATIONS =====

// Handle UPSERT_BILLS_JSON_CHUNK command
bool handleUpsertBillsJsonChunk(String payload) {
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

  // Parse the JSON chunk (it's an array of bills)
  DynamicJsonDocument doc(16384); // 16KB should be enough for 10 bills
  Serial.println(F("About to deserialize JSON for bills"));
  DeserializationError error = deserializeJson(doc, jsonChunk);
  if (error) {
    Serial.println(F("ERR|JSON_PARSE_FAILED"));
    Serial.print(F("Error: "));
    Serial.println(error.c_str());
    return true;
  }
  Serial.println(F("JSON deserialized successfully"));

  JsonArray bills = doc.as<JsonArray>();
  bool allSuccess = true;

  Serial.print(F("Processing bill chunk "));
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
  const char* sql = "INSERT INTO bills (reference_number, customer_id, reading_id, device_uid, bill_date, rate_per_m3, charges, penalty, total_due, due_date, status, created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, datetime('now'), datetime('now'));";
  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.print(F("SQLite prepare error: "));
    Serial.println(sqlite3_errmsg(db));
    sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
    Serial.println(F("ERR|UPSERT_FAILED"));
    return true;
  }

  for (JsonObject bill : bills) {
    String referenceNumber = bill["reference_number"] | "";
    unsigned long customerId = bill["customer_id"] | 0;
    unsigned long readingId = bill["reading_id"] | 0;
    String deviceUid = bill["device_uid"] | "";
    String billDate = bill["bill_date"] | "";
    float ratePerM3 = bill["rate_per_m3"] | 0.0;
    float charges = bill["charges"] | 0.0;
    float penalty = bill["penalty"] | 0.0;
    float totalDue = bill["total_due"] | 0.0;
    String dueDate = bill["due_date"] | "";
    String status = bill["status"] | "pending";

    // Reset and clear bindings for reuse
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);

    sqlite3_bind_text(stmt, 1, referenceNumber.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, customerId);
    sqlite3_bind_int64(stmt, 3, readingId);
    sqlite3_bind_text(stmt, 4, deviceUid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, billDate.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 6, ratePerM3);
    sqlite3_bind_double(stmt, 7, charges);
    sqlite3_bind_double(stmt, 8, penalty);
    sqlite3_bind_double(stmt, 9, totalDue);
    sqlite3_bind_text(stmt, 10, dueDate.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 11, status.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
      Serial.print(F("SQLite step error: "));
      Serial.println(sqlite3_errmsg(db));
      allSuccess = false;
      break;
    } else {
      Serial.print(F("Inserted bill: "));
      Serial.println(referenceNumber);
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
      Serial.print(F("Processed bill chunk "));
      Serial.println(chunkIndex);
    }
    Serial.print(F("ACK|CHUNK|"));
    Serial.println(chunkIndex);
    Serial.printf("Heap free after chunk: %d\n", ESP.getFreeHeap());
    Serial.println(F("Ready for next chunk"));
    Serial.flush();
    if (chunkIndex == totalChunks - 1) {
      // Last chunk, count bills in DB
      int billCount = 0;
      sqlite3_exec(db, "SELECT COUNT(*) FROM bills;", billCountCallback, &billCount, NULL);
      Serial.print(F("Total bills in DB after sync: "));
      Serial.println(billCount);
      // Send final success
      Serial.print(F("ACK|UPSERT_BILLS_JSON|"));
      Serial.println(bills.size());
      Serial.flush();
    }
  } else {
    sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
    Serial.println(F("ERR|UPSERT_FAILED"));
  }
  return true;
}

// Handle EXPORT_BILLS command
bool handleExportBills() {
  Serial.println(F("Exporting bills..."));
  // Get total count
  sqlite3_stmt* countStmt;
  const char* countSql = "SELECT COUNT(*) FROM bills WHERE synced = 0;";
  int rc = sqlite3_prepare_v2(db, countSql, -1, &countStmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.println(F("Failed to prepare count query"));
    return false;
  }
  rc = sqlite3_step(countStmt);
  int totalBills = 0;
  if (rc == SQLITE_ROW) {
    totalBills = sqlite3_column_int(countStmt, 0);
  }
  sqlite3_finalize(countStmt);
  const int CHUNK_SIZE = 150;
  int totalChunks = (totalBills + CHUNK_SIZE - 1) / CHUNK_SIZE;
  Serial.print(F("BEGIN_BILLS_JSON|"));
  Serial.println(totalChunks);

  // Prepare statement for chunked query
  sqlite3_stmt* stmt;
  const char* sql = "SELECT bill_id, reference_number, customer_id, reading_id, device_uid, bill_date, rate_per_m3, charges, penalty, total_due, status, synced, last_sync FROM bills WHERE synced = 0 ORDER BY bill_id LIMIT ? OFFSET ?;";
  int rc2 = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc2 != SQLITE_OK) {
    Serial.println(F("Failed to prepare bills query"));
    return false;
  }

  for (int chunk = 0; chunk < totalChunks; ++chunk) {
    Serial.printf("Heap free before chunk %d: %d\n", chunk, ESP.getFreeHeap());
    int offset = chunk * CHUNK_SIZE;
    sqlite3_bind_int(stmt, 1, CHUNK_SIZE);
    sqlite3_bind_int(stmt, 2, offset);

    DynamicJsonDocument doc(65536);  // Larger doc for 150 items
    JsonArray arr = doc.to<JsonArray>();

    while (sqlite3_step(stmt) == SQLITE_ROW) {
      JsonObject obj = arr.createNestedObject();
      obj["bill_id"] = (int)sqlite3_column_int(stmt, 0);
      obj["reference_number"] = String((const char*)sqlite3_column_text(stmt, 1));
      obj["customer_id"] = (int)sqlite3_column_int(stmt, 2);
      obj["reading_id"] = (int)sqlite3_column_int(stmt, 3);
      obj["device_uid"] = String((const char*)sqlite3_column_text(stmt, 4));
      obj["bill_date"] = String((const char*)sqlite3_column_text(stmt, 5));
      obj["rate_per_m3"] = (float)sqlite3_column_double(stmt, 6);
      obj["charges"] = (float)sqlite3_column_double(stmt, 7);
      obj["penalty"] = (float)sqlite3_column_double(stmt, 8);
      obj["total_due"] = (float)sqlite3_column_double(stmt, 9);
      obj["status"] = String((const char*)sqlite3_column_text(stmt, 10));
      obj["synced"] = (int)sqlite3_column_int(stmt, 11);
      obj["last_sync"] = sqlite3_column_text(stmt, 12) ? String((const char*)sqlite3_column_text(stmt, 12)) : "";
    }

    Serial.print(F("BILLS_CHUNK|"));
    Serial.print(chunk);
    Serial.print(F("|"));
    serializeJson(arr, Serial);
    Serial.println();
    doc.clear();

    // Reset statement for next chunk
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);

    Serial.printf("Heap free after chunk %d: %d\n", chunk, ESP.getFreeHeap());
  }

  sqlite3_finalize(stmt);
  Serial.println(F("END_BILLS_JSON"));
  return true;
}

// Handle BILLS_SYNCED command
bool handleBillsSynced() {
  bool ok = markAllBillsSynced();
  if (ok) {
    setLastSyncEpoch(deviceEpochNow());
    Serial.println(F("ACK|BILLS_SYNCED"));
  } else {
    Serial.println(F("ERR|BILLS_SYNC_FAILED"));
  }
  return true;
}

#endif // BILL_SYNC_H