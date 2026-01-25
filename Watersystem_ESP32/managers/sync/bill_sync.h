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
  const int CHUNK_SIZE = 150;
  int totalBills = getTotalBills();
  int totalChunks = (totalBills + CHUNK_SIZE - 1) / CHUNK_SIZE;
  Serial.print(F("BEGIN_BILLS_JSON|"));
  Serial.println(totalChunks);
  int offset = 0;
  for (int chunk = 0; chunk < totalChunks; ++chunk) {
    Serial.printf("Heap free before chunk %d: %d\n", chunk, ESP.getFreeHeap());
    std::vector<Bill> billsChunk = getBillsChunk(offset, CHUNK_SIZE);
    DynamicJsonDocument doc(65536);
    JsonArray arr = doc.to<JsonArray>();
    for (const auto& b : billsChunk) {
      JsonObject obj = arr.createNestedObject();
      obj["bill_id"] = b.bill_id;
      obj["reference_number"] = b.reference_number;
      obj["customer_id"] = b.customer_id;
      obj["reading_id"] = b.reading_id;
      obj["device_uid"] = b.device_uid;
      obj["bill_date"] = b.bill_date;
      obj["rate_per_m3"] = b.rate_per_m3;
      obj["charges"] = b.charges;
      obj["penalty"] = b.penalty;
      obj["total_due"] = b.total_due;
      obj["status"] = b.status;
    }
    Serial.print(F("BILLS_CHUNK|"));
    Serial.print(chunk);
    Serial.print(F("|"));
    serializeJson(arr, Serial);
    Serial.println();
    Serial.printf("Heap free after chunk %d: %d\n", chunk, ESP.getFreeHeap());
    offset += billsChunk.size();
  }
  Serial.println(F("END_BILLS_JSON"));
  return true;
}

#endif // BILL_SYNC_H