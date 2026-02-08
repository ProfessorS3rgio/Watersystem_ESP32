#ifndef READING_SYNC_H
#define READING_SYNC_H

#include "../../database/readings_database.h"
#include "../../database/bill_database.h"
#include <ArduinoJson.h>

// ===== READING SYNC OPERATIONS =====

// Handle EXPORT_READINGS command
bool handleExportReadings() {
  Serial.println(F("Exporting readings..."));
  // Get total count
  sqlite3_stmt* countStmt;
  const char* countSql = "SELECT COUNT(*) FROM readings;";
  int rc = sqlite3_prepare_v2(db, countSql, -1, &countStmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.println(F("Failed to prepare count query"));
    return false;
  }
  rc = sqlite3_step(countStmt);
  int totalReadings = 0;
  if (rc == SQLITE_ROW) {
    totalReadings = sqlite3_column_int(countStmt, 0);
  }
  sqlite3_finalize(countStmt);
  const int CHUNK_SIZE = 150;
  int totalChunks = (totalReadings + CHUNK_SIZE - 1) / CHUNK_SIZE;
  Serial.print(F("BEGIN_READINGS_JSON|"));
  Serial.println(totalChunks);

  // Prepare statement for chunked query
  sqlite3_stmt* stmt;
  const char* sql = "SELECT reading_id, customer_id, device_uid, previous_reading, current_reading, usage_m3, reading_at, synced, last_sync, customer_account_number FROM readings WHERE synced = 0 ORDER BY reading_id LIMIT ? OFFSET ?;";
  int rc2 = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc2 != SQLITE_OK) {
    Serial.println(F("Failed to prepare readings query"));
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
      obj["reading_id"] = (int)sqlite3_column_int(stmt, 0);
      obj["customer_id"] = (int)sqlite3_column_int(stmt, 1);
      obj["device_uid"] = String((const char*)sqlite3_column_text(stmt, 2));
      obj["previous_reading"] = (unsigned long)sqlite3_column_int64(stmt, 3);
      obj["current_reading"] = (unsigned long)sqlite3_column_int64(stmt, 4);
      obj["usage_m3"] = (unsigned long)sqlite3_column_int64(stmt, 5);
      obj["reading_at"] = (unsigned long)sqlite3_column_int64(stmt, 6);
      obj["synced"] = (int)sqlite3_column_int(stmt, 7);
      obj["last_sync"] = sqlite3_column_text(stmt, 8) ? String((const char*)sqlite3_column_text(stmt, 8)) : "";
      obj["customer_account_number"] = String((const char*)sqlite3_column_text(stmt, 9));
    }

    Serial.print(F("READINGS_CHUNK|"));
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
  Serial.println(F("END_READINGS_JSON"));
  
  // Mark all exported readings as synced
  const char* updateSql = "UPDATE readings SET synced = 1, last_sync = datetime('now') WHERE synced = 0;";
  int updateRc = sqlite3_exec(db, updateSql, NULL, NULL, NULL);
  if (updateRc != SQLITE_OK) {
    Serial.print(F("Failed to mark readings as synced: "));
    Serial.println(sqlite3_errmsg(db));
  } else {
    Serial.println(F("Marked exported readings as synced"));
  }
  
  return true;
}

// Handle READINGS_SYNCED command
bool handleReadingsSynced() {
  bool ok = markAllReadingsSynced();
  if (ok) {
    setLastSyncEpoch(deviceEpochNow());
    Serial.println(F("ACK|READINGS_SYNCED"));
  } else {
    Serial.println(F("ERR|READINGS_SYNC_FAILED"));
  }
  return true;
}

#endif // READING_SYNC_H