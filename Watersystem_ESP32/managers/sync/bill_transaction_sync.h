#ifndef BILL_TRANSACTION_SYNC_H
#define BILL_TRANSACTION_SYNC_H

#include "../../database/bill_transaction_database.h"
#include "../../configuration/config.h"
#include <ArduinoJson.h>
#include <vector>

// ===== BILL TRANSACTION SYNC OPERATIONS =====

// Handle EXPORT_BILL_TRANSACTIONS command
bool handleExportBillTransactions() {
  Serial.println(F("Exporting bill transactions..."));
  const int CHUNK_SIZE = 50;
  int totalTransactions = getTotalBillTransactions();
  int totalChunks = (totalTransactions + CHUNK_SIZE - 1) / CHUNK_SIZE;
  Serial.print(F("BEGIN_BILL_TRANSACTIONS_JSON|"));
  Serial.println(totalChunks);
  int offset = 0;
  for (int chunk = 0; chunk < totalChunks; ++chunk) {
    Serial.printf("Heap free before chunk %d: %d\n", chunk, ESP.getFreeHeap());
    std::vector<BillTransaction> transactionsChunk = getBillTransactionsChunk(offset, CHUNK_SIZE);
    DynamicJsonDocument doc(65536);
    JsonArray arr = doc.to<JsonArray>();
    for (const auto& bt : transactionsChunk) {
      JsonObject obj = arr.createNestedObject();
      obj["bill_transaction_id"] = bt.bill_transaction_id;
      obj["bill_id"] = bt.bill_id;
      obj["bill_reference_number"] = bt.bill_reference_number;
      obj["type"] = bt.type;
      obj["source"] = bt.source;
      obj["amount"] = bt.amount;
      obj["cash_received"] = bt.cash_received;
      obj["change"] = bt.change;
      obj["transaction_date"] = bt.transaction_date;
      obj["payment_method"] = bt.payment_method;
      obj["processed_by_device_uid"] = bt.processed_by_device_uid;
      obj["notes"] = bt.notes;
      obj["created_at"] = bt.created_at;
      obj["updated_at"] = bt.updated_at;
    }
    Serial.print(F("BILL_TRANSACTIONS_CHUNK|"));
    Serial.print(chunk);
    Serial.print(F("|"));
    serializeJson(arr, Serial);
    Serial.println();
    Serial.printf("Heap free after chunk %d: %d\n", chunk, ESP.getFreeHeap());
    offset += transactionsChunk.size();
  }
  Serial.println(F("END_BILL_TRANSACTIONS_JSON"));
  return true;
}

// Handle UPSERT_BILL_TRANSACTIONS_JSON_CHUNK command
bool handleUpsertBillTransactionsJsonChunk(String payload) {
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

  // Parse the JSON chunk (it's an array of bill transactions)
  DynamicJsonDocument doc(16384); // 16KB should be enough for transactions
  Serial.println(F("About to deserialize JSON for bill transactions"));
  DeserializationError error = deserializeJson(doc, jsonChunk);
  if (error) {
    Serial.print(F("ERR|JSON_PARSE_FAIL|"));
    Serial.println(error.c_str());
    return true;
  }

  JsonArray arr = doc.as<JsonArray>();
  int count = arr.size();
  Serial.printf("Processing %d bill transactions in chunk %d/%d\n", count, chunkIndex + 1, totalChunks);

  sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);

  for (JsonObject obj : arr) {
    int bill_transaction_id = obj["bill_transaction_id"];
    int bill_id = obj["bill_id"];
    String bill_reference_number = obj["bill_reference_number"];
    String type = obj["type"];
    String source = obj["source"];
    float amount = obj["amount"];
    float cash_received = obj["cash_received"];
    float change = obj["change"];
    String transaction_date = obj["transaction_date"];
    String payment_method = obj["payment_method"];
    String processed_by_device_uid = obj["processed_by_device_uid"];
    String notes = obj["notes"];
    String created_at = obj["created_at"];
    String updated_at = obj["updated_at"];

    // Upsert logic: INSERT OR REPLACE
    const char* sql = "INSERT OR REPLACE INTO bill_transactions (bill_transaction_id, bill_id, bill_reference_number, type, source, amount, cash_received, change, transaction_date, payment_method, processed_by_device_uid, notes, created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
      sqlite3_bind_int(stmt, 1, bill_transaction_id);
      sqlite3_bind_int(stmt, 2, bill_id);
      sqlite3_bind_text(stmt, 3, bill_reference_number.c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 4, type.c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 5, source.c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_double(stmt, 6, amount);
      sqlite3_bind_double(stmt, 7, cash_received);
      sqlite3_bind_double(stmt, 8, change);
      sqlite3_bind_text(stmt, 9, transaction_date.c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 10, payment_method.c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 11, processed_by_device_uid.c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 12, notes.c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 13, created_at.c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 14, updated_at.c_str(), -1, SQLITE_TRANSIENT);

      int step = sqlite3_step(stmt);
      if (step != SQLITE_DONE) {
        Serial.printf("ERR|INSERT_FAIL|%s\n", sqlite3_errmsg(db));
      }
      sqlite3_finalize(stmt);
    } else {
      Serial.printf("ERR|PREPARE_FAIL|%s\n", sqlite3_errmsg(db));
    }
  }

  sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);

  Serial.printf("ACK|CHUNK_%d_PROCESSED\n", chunkIndex);

  // If this is the last chunk, reload the in-memory data
  if (chunkIndex == totalChunks - 1) {
    loadBillTransactionsFromDB();
    Serial.printf("Reloaded %d bill transactions into memory\n", billTransactions.size());
  }

  return true;
}

#endif // BILL_TRANSACTION_SYNC_H