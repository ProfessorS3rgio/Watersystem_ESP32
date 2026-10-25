#ifndef READING_QUERY_H
#define READING_QUERY_H
#include "../sync_transport.h"
#include "../../database/readings_database.h"
#include "export_json.h"

bool handleQueryReadingsForAccount(const String& accountNo) {
  if (!db || accountNo.length() == 0 || accountNo.length() > 64) { syncOutput().println(F("ERR|INVALID_READING_QUERY")); return false; }
  SyncExportMemory memory(db);
  int total = 0;
  {
    SyncExportStatement count;
    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM readings WHERE customer_account_number = ?;", -1, &count.value, nullptr) != SQLITE_OK) {
      syncOutput().println(F("ERR|READING_QUERY_FAILED")); return false;
    }
    sqlite3_bind_text(count.value, 1, accountNo.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(count.value) != SQLITE_ROW) { syncOutput().println(F("ERR|READING_QUERY_FAILED")); return false; }
    total = sqlite3_column_int(count.value, 0);
  }
  // One row per chunk keeps the existing human diagnostics outside the JSON line.
  syncOutput().print(F("BEGIN_READINGS_QUERY_JSON|")); syncOutput().println(total);
  if (total) {
    SyncExportStatement statement;
    const char* sql = "SELECT reading_id, customer_id, device_uid, previous_reading, current_reading, usage_m3, reading_at, synced, last_sync, customer_account_number, CAST(strftime('%s', created_at) AS INTEGER), created_at FROM readings WHERE customer_account_number = ? ORDER BY reading_id;";
    if (sqlite3_prepare_v2(db, sql, -1, &statement.value, nullptr) != SQLITE_OK) { syncOutput().println(F("ERR|READING_QUERY_FAILED")); return false; }
    sqlite3_bind_text(statement.value, 1, accountNo.c_str(), -1, SQLITE_TRANSIENT);
    for (int chunk = 0; chunk < total; ++chunk) {
      if (!syncWriteExportChunk(syncOutput(), statement.value, "READINGS_CHUNK|", chunk, 1, writeReadingExportRow)) return false;
      const sqlite3_int64 readingAt = sqlite3_column_int64(statement.value, 6);
      syncOutput().print(F("QUERY_RESULT|account=")); syncOutput().print(accountNo);
      syncOutput().print(F("|id=")); syncOutput().print(sqlite3_column_int(statement.value, 0));
      syncOutput().print(F("|prev=")); syncOutput().print((unsigned long)sqlite3_column_int64(statement.value, 3));
      syncOutput().print(F("|current=")); syncOutput().print((unsigned long)sqlite3_column_int64(statement.value, 4));
      syncOutput().print(F("|usage=")); syncOutput().print((unsigned long)sqlite3_column_int64(statement.value, 5));
      syncOutput().print(F("|reading_at=")); syncOutput().print((unsigned long)(readingAt >= 1577836800LL ? readingAt : sqlite3_column_int64(statement.value, 10)));
      syncOutput().print(F("|created_at=")); syncOutput().println(sqlite3_column_text(statement.value, 11) ? (const char*)sqlite3_column_text(statement.value, 11) : "");
    }
    if (!syncExportCursorDone(syncOutput(), statement.value)) return false;
  }
  syncOutput().println(F("END_READINGS_QUERY_JSON"));
  return true;
}
#endif