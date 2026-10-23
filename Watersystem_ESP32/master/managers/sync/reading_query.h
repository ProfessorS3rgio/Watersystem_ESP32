#ifndef READING_QUERY_H
#define READING_QUERY_H

#include "../../database/readings_database.h"
#include <ArduinoJson.h>

// Read-only, exact account lookup for the web inspector. Bound parameters keep
// user-entered account numbers out of SQL text and avoid an all-record export.
bool handleQueryReadingsForAccount(const String &accountNo) {
  if (!db || accountNo.length() == 0 || accountNo.length() > 64) {
    Serial.println(F("ERR|INVALID_READING_QUERY"));
    return false;
  }

  sqlite3_stmt *countStmt = nullptr;
  const char *countSql = "SELECT COUNT(*) FROM readings WHERE customer_account_number = ?;";
  if (sqlite3_prepare_v2(db, countSql, -1, &countStmt, nullptr) != SQLITE_OK) {
    Serial.println(F("ERR|READING_QUERY_FAILED"));
    return false;
  }
  sqlite3_bind_text(countStmt, 1, accountNo.c_str(), -1, SQLITE_TRANSIENT);
  int total = 0;
  if (sqlite3_step(countStmt) == SQLITE_ROW) total = sqlite3_column_int(countStmt, 0);
  sqlite3_finalize(countStmt);

  const int chunkSize = 100;
  const int chunks = (total + chunkSize - 1) / chunkSize;
  Serial.print(F("BEGIN_READINGS_QUERY_JSON|"));
  Serial.println(chunks);

  const char *sql =
    "SELECT reading_id, customer_id, device_uid, previous_reading, current_reading, usage_m3, reading_at, synced, last_sync, customer_account_number, "
    "CAST(strftime('%s', created_at) AS INTEGER) AS created_at_epoch, created_at "
    "FROM readings WHERE customer_account_number = ? ORDER BY reading_id LIMIT ? OFFSET ?;";
  sqlite3_stmt *stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    Serial.println(F("ERR|READING_QUERY_FAILED"));
    return false;
  }

  for (int chunk = 0; chunk < chunks; ++chunk) {
    sqlite3_bind_text(stmt, 1, accountNo.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, chunkSize);
    sqlite3_bind_int(stmt, 3, chunk * chunkSize);
    DynamicJsonDocument doc(32768);
    JsonArray rows = doc.to<JsonArray>();
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      JsonObject row = rows.createNestedObject();
      row["reading_id"] = sqlite3_column_int(stmt, 0);
      row["customer_id"] = sqlite3_column_int(stmt, 1);
      row["device_uid"] = sqlite3_column_text(stmt, 2) ? String((const char *)sqlite3_column_text(stmt, 2)) : "";
      row["previous_reading"] = (unsigned long)sqlite3_column_int64(stmt, 3);
      row["current_reading"] = (unsigned long)sqlite3_column_int64(stmt, 4);
      row["usage_m3"] = (unsigned long)sqlite3_column_int64(stmt, 5);
      const unsigned long readingAt = (unsigned long)sqlite3_column_int64(stmt, 6);
      const unsigned long createdAt = (unsigned long)sqlite3_column_int64(stmt, 10);
      row["reading_at"] = readingAt >= 1577836800UL ? readingAt : createdAt;
      row["synced"] = sqlite3_column_int(stmt, 7);
      row["last_sync"] = sqlite3_column_text(stmt, 8) ? String((const char *)sqlite3_column_text(stmt, 8)) : "";
      row["customer_account_number"] = sqlite3_column_text(stmt, 9) ? String((const char *)sqlite3_column_text(stmt, 9)) : "";
      row["created_at"] = sqlite3_column_text(stmt, 11) ? String((const char *)sqlite3_column_text(stmt, 11)) : "";

      // Human-readable device-originated output for serial cross-checking.
      // The web parser deliberately ignores this diagnostic line; it receives
      // the structured JSON chunk below as the source of inspector data.
      Serial.print(F("QUERY_RESULT|account="));
      Serial.print(accountNo);
      Serial.print(F("|id="));
      Serial.print(sqlite3_column_int(stmt, 0));
      Serial.print(F("|prev="));
      Serial.print((unsigned long)sqlite3_column_int64(stmt, 3));
      Serial.print(F("|current="));
      Serial.print((unsigned long)sqlite3_column_int64(stmt, 4));
      Serial.print(F("|usage="));
      Serial.print((unsigned long)sqlite3_column_int64(stmt, 5));
      Serial.print(F("|reading_at="));
      Serial.print(readingAt >= 1577836800UL ? readingAt : createdAt);
      Serial.print(F("|created_at="));
      Serial.println(sqlite3_column_text(stmt, 11) ? (const char *)sqlite3_column_text(stmt, 11) : "");
    }
    Serial.print(F("READINGS_CHUNK|"));
    Serial.print(chunk);
    Serial.print(F("|"));
    serializeJson(rows, Serial);
    Serial.println();
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
  }
  sqlite3_finalize(stmt);
  Serial.println(F("END_READINGS_QUERY_JSON"));
  return true;
}

#endif // READING_QUERY_H
