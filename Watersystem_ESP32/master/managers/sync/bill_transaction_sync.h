#ifndef BILL_TRANSACTION_SYNC_H
#define BILL_TRANSACTION_SYNC_H
#include "../sync_transport.h"
#include "export_json.h"

#include "../../database/bill_transactions_database.h"
#include "../../configuration/config.h"
#include <ArduinoJson.h>
#include <vector>

// ===== BILL TRANSACTION SYNC OPERATIONS =====

// Handle EXPORT_BILL_TRANSACTIONS command
bool handleExportBillTransactions() {
  syncOutput().println(F("Exporting bill transactions..."));
  if (!db) { syncOutput().println(F("ERR|DATABASE_NOT_READY")); return false; }
  SyncExportMemory memory(db);
  int total = 0;
  {
    SyncExportStatement count;
    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM bill_transactions WHERE synced = 0 AND last_sync IS NULL;", -1, &count.value, nullptr) != SQLITE_OK || sqlite3_step(count.value) != SQLITE_ROW) {
      syncOutput().println(F("ERR|EXPORT_COUNT_FAILED")); return false;
    }
    total = sqlite3_column_int(count.value, 0);
  }
  const int chunkSize = 20;
  const int chunks = (total + chunkSize - 1) / chunkSize;
  syncOutput().print(F("BEGIN_BILL_TRANSACTIONS_JSON|")); syncOutput().println(chunks);
  if (total) {
    SyncExportStatement statement;
    const char* sql = "SELECT bill_transaction_id, bill_id, bill_reference_number, type, source, amount, cash_received, change, transaction_date, payment_method, processed_by_device_uid, notes, created_at, updated_at, synced, last_sync FROM bill_transactions WHERE synced = 0 AND last_sync IS NULL ORDER BY bill_transaction_id;";
    if (sqlite3_prepare_v2(db, sql, -1, &statement.value, nullptr) != SQLITE_OK) { syncOutput().println(F("ERR|EXPORT_QUERY_FAILED")); return false; }
    for (int chunk = 0; chunk < chunks; ++chunk) {
      const int remaining = total - chunk * chunkSize;
      if (!syncWriteExportChunk(syncOutput(), statement.value, "BILL_TRANSACTIONS_CHUNK|", chunk, remaining < chunkSize ? remaining : chunkSize, writeTransactionExportRow)) return false;
    }
    if (!syncExportCursorDone(syncOutput(), statement.value)) return false;
  }
  syncOutput().println(F("END_BILL_TRANSACTIONS_JSON"));
  // Do not mark records synced until the website's committed-import acknowledgment.
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
