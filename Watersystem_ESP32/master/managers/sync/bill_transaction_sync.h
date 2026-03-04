#ifndef BILL_TRANSACTION_SYNC_H
#define BILL_TRANSACTION_SYNC_H

#include "../../database/bill_transactions_database.h"
#include "../../configuration/config.h"
#include <ArduinoJson.h>
#include <vector>

// ===== BILL TRANSACTION SYNC OPERATIONS =====

// Handle EXPORT_BILL_TRANSACTIONS command
bool handleExportBillTransactions() {
  Serial.println(F("Exporting bill transactions..."));
  // Get total count
  sqlite3_stmt* countStmt;
  const char* countSql = "SELECT COUNT(*) FROM bill_transactions WHERE synced = 0 AND last_sync IS NULL;";
  int rc = sqlite3_prepare_v2(db, countSql, -1, &countStmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.println(F("Failed to prepare count query for bill transactions"));
    return false;
  }
  rc = sqlite3_step(countStmt);
  int totalTransactions = 0;
  if (rc == SQLITE_ROW) {
    totalTransactions = sqlite3_column_int(countStmt, 0);
  }
  sqlite3_finalize(countStmt);
  const int CHUNK_SIZE = 300;
  int totalChunks = (totalTransactions + CHUNK_SIZE - 1) / CHUNK_SIZE;
  Serial.print(F("BEGIN_BILL_TRANSACTIONS_JSON|"));
  Serial.println(totalChunks);

  // Prepare statement for chunked query
  sqlite3_stmt* stmt;
  const char* sql = "SELECT bill_transaction_id, bill_id, bill_reference_number, type, source, amount, cash_received, change, transaction_date, payment_method, processed_by_device_uid, notes, created_at, updated_at, synced, last_sync FROM bill_transactions WHERE synced = 0 AND last_sync IS NULL ORDER BY bill_transaction_id LIMIT ? OFFSET ?;";
  int rc2 = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc2 != SQLITE_OK) {
    Serial.println(F("Failed to prepare bill transactions query"));
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
      obj["bill_transaction_id"] = (int)sqlite3_column_int(stmt, 0);
      obj["bill_id"] = (int)sqlite3_column_int(stmt, 1);
      obj["bill_reference_number"] = String((const char*)sqlite3_column_text(stmt, 2));
      obj["type"] = String((const char*)sqlite3_column_text(stmt, 3));
      obj["source"] = String((const char*)sqlite3_column_text(stmt, 4));
      obj["amount"] = (float)sqlite3_column_double(stmt, 5);
      obj["cash_received"] = (float)sqlite3_column_double(stmt, 6);
      obj["change"] = (float)sqlite3_column_double(stmt, 7);
      obj["transaction_date"] = String((const char*)sqlite3_column_text(stmt, 8));
      obj["payment_method"] = String((const char*)sqlite3_column_text(stmt, 9));
      obj["processed_by_device_uid"] = String((const char*)sqlite3_column_text(stmt, 10));
      obj["notes"] = String((const char*)sqlite3_column_text(stmt, 11));
      obj["created_at"] = String((const char*)sqlite3_column_text(stmt, 12));
      obj["updated_at"] = String((const char*)sqlite3_column_text(stmt, 13));
      obj["synced"] = (int)sqlite3_column_int(stmt, 14);
      obj["last_sync"] = sqlite3_column_text(stmt, 15) ? String((const char*)sqlite3_column_text(stmt, 15)) : "";
    }

    Serial.print(F("BILL_TRANSACTIONS_CHUNK|"));
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
  Serial.println(F("END_BILL_TRANSACTIONS_JSON"));
  
  // Mark all exported bill transactions as synced (batched)
  if (!markAllBillTransactionsSynced()) {
    Serial.println(F("Failed to mark bill transactions as synced"));
    return false;
  }
  Serial.println(F("Bill transactions marked as synced"));
  return true;
}

// Handle BILL_TRANSACTIONS_SYNCED command
bool handleBillTransactionsSynced() {
  Serial.println(F("Marking bill transactions as synced..."));
  return markAllBillTransactionsSynced();
}

#endif // BILL_TRANSACTION_SYNC_H