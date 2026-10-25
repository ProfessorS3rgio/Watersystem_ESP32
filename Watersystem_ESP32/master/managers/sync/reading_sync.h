#ifndef READING_SYNC_H
#define READING_SYNC_H
#include "../sync_transport.h"
#include "../../database/readings_database.h"
#include "../../database/bill_database.h"
#include "export_json.h"

// Inspection and normal exports are read-only; only READINGS_SYNCED writes state.
bool handleExportReadings(bool includeSynced = false) {
  syncOutput().println(F("Exporting readings..."));
  if (!db) { syncOutput().println(F("ERR|DATABASE_NOT_READY")); return false; }
  SyncExportMemory memory(db);
  int total = 0;
  {
    SyncExportStatement count;
    const char* sql = includeSynced ? "SELECT COUNT(*) FROM readings;" : "SELECT COUNT(*) FROM readings WHERE synced = 0 AND last_sync IS NULL;";
    if (sqlite3_prepare_v2(db, sql, -1, &count.value, nullptr) != SQLITE_OK || sqlite3_step(count.value) != SQLITE_ROW) {
      syncOutput().println(F("ERR|READINGS_COUNT_FAILED")); return false;
    }
    total = sqlite3_column_int(count.value, 0);
  }
  const int chunkSize = 20;
  const int chunks = (total + chunkSize - 1) / chunkSize;
  syncOutput().print(F("BEGIN_READINGS_JSON|")); syncOutput().println(chunks);
  if (total) {
    SyncExportStatement statement;
    const char* sql = includeSynced
      ? "SELECT reading_id, customer_id, device_uid, previous_reading, current_reading, usage_m3, reading_at, synced, last_sync, customer_account_number, CAST(strftime('%s', created_at) AS INTEGER), created_at FROM readings ORDER BY reading_id;"
      : "SELECT reading_id, customer_id, device_uid, previous_reading, current_reading, usage_m3, reading_at, synced, last_sync, customer_account_number, CAST(strftime('%s', created_at) AS INTEGER), created_at FROM readings WHERE synced = 0 AND last_sync IS NULL ORDER BY reading_id;";
    if (sqlite3_prepare_v2(db, sql, -1, &statement.value, nullptr) != SQLITE_OK) { syncOutput().println(F("ERR|READINGS_QUERY_FAILED")); return false; }
    for (int chunk = 0; chunk < chunks; ++chunk) {
      const int remaining = total - chunk * chunkSize;
      if (!syncWriteExportChunk(syncOutput(), statement.value, "READINGS_CHUNK|", chunk, remaining < chunkSize ? remaining : chunkSize, writeReadingExportRow)) return false;
    }
    if (!syncExportCursorDone(syncOutput(), statement.value)) return false;
  }
  syncOutput().println(F("END_READINGS_JSON"));
  return true;
}

bool handleReadingsSynced() {
  bool ok = markAllReadingsSynced();
  if (ok) { setLastSyncEpoch(deviceEpochNow()); syncOutput().println(F("ACK|READINGS_SYNCED")); }
  else syncOutput().println(F("ERR|READINGS_SYNC_FAILED"));
  return true;
}
#endif