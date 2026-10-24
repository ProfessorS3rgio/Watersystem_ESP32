#ifndef BILL_TRANSACTION_SYNC_H
#define BILL_TRANSACTION_SYNC_H
#include "../sync_transport.h"

#include "../../database/bill_transactions_database.h"
#include "../../configuration/config.h"
#include <ArduinoJson.h>
#include <vector>

// ===== BILL TRANSACTION SYNC OPERATIONS =====

// Handle EXPORT_BILL_TRANSACTIONS command
bool handleExportBillTransactions() {
  syncOutput().println(F("Exporting bill transactions..."));
  // Get total count
  sqlite3_stmt* countStmt;
  const char* countSql = "SELECT COUNT(*) FROM bill_transactions WHERE synced = 0 AND last_sync IS NULL;";
  int rc = sqlite3_prepare_v2(db, countSql, -1, &countStmt, NULL);
  if (rc != SQLITE_OK) {
    syncOutput().println(F("Failed to prepare count query for bill transactions"));
    return false;
  }
  rc = sqlite3_step(countStmt);
  int totalTransactions = 0;
  if (rc == SQLITE_ROW) {
    totalTransactions = sqlite3_column_int(countStmt, 0);
  }
  sqlite3_finalize(countStmt);
  const int CHUNK_SIZE = syncOverWifi() ? 20 : 300;
  int totalChunks = (totalTransactions + CHUNK_SIZE - 1) / CHUNK_SIZE;
  syncOutput().print(F("BEGIN_BILL_TRANSACTIONS_JSON|"));
  syncOutput().println(totalChunks);

  // Prepare statement for chunked query
  sqlite3_stmt* stmt;
  const char* sql = "SELECT bill_transaction_id, bill_id, bill_reference_number, type, source, amount, cash_received, change, transaction_date, payment_method, processed_by_device_uid, notes, created_at, updated_at, synced, last_sync FROM bill_transactions WHERE synced = 0 AND last_sync IS NULL ORDER BY bill_transaction_id LIMIT ? OFFSET ?;";
  int rc2 = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc2 != SQLITE_OK) {
    syncOutput().println(F("Failed to prepare bill transactions query"));
    return false;
  }

  for (int chunk = 0; chunk < totalChunks; ++chunk) {
    syncOutput().printf("Heap free before chunk %d: %d\n", chunk, ESP.getFreeHeap());
    int offset = chunk * CHUNK_SIZE;
    sqlite3_bind_int(stmt, 1, CHUNK_SIZE);
    sqlite3_bind_int(stmt, 2, offset);

    DynamicJsonDocument doc(syncOverWifi() ? 16384 : 65536);  // Larger doc for 150 items
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

    if (doc.overflowed()) { sqlite3_finalize(stmt); syncOutput().println(F("ERR|JSON_CAPACITY")); return false; }
    syncOutput().print(F("BILL_TRANSACTIONS_CHUNK|"));
    syncOutput().print(chunk);
    syncOutput().print(F("|"));
    serializeJson(arr, syncOutput());
    syncOutput().println();
    doc.clear();

    // Reset statement for next chunk
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);

    syncOutput().printf("Heap free after chunk %d: %d\n", chunk, ESP.getFreeHeap());
  }

  sqlite3_finalize(stmt);
  syncOutput().println(F("END_BILL_TRANSACTIONS_JSON"));
  
  // Export is read-only; wait for the website's committed-import acknowledgment.
  return true;
}

// Handle BILL_TRANSACTIONS_SYNCED command
bool handleBillTransactionsSynced() {
  syncOutput().println(F("Marking bill transactions as synced..."));
  bool ok = markAllBillTransactionsSynced();
  syncOutput().println(ok ? F("ACK|BILL_TRANSACTIONS_SYNCED") : F("ERR|BILL_TRANSACTIONS_SYNC_FAILED"));
  return true;
}

#endif // BILL_TRANSACTION_SYNC_H
