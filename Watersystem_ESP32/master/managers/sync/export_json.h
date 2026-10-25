#ifndef DMBC_EXPORT_JSON_H
#define DMBC_EXPORT_JSON_H
#include <Arduino.h>
#include <sqlite3.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

// SQLite owns each row until the next step. No JsonDocument or String copies.
static void syncJsonString(Print& output, const char* value, size_t length) {
  static const char hex[] = "0123456789abcdef";
  output.write('"');
  for (size_t i = 0; value && i < length; ++i) {
    const uint8_t byte = static_cast<uint8_t>(value[i]);
    if (byte == '"' || byte == '\\') { output.write('\\'); output.write(byte); }
    else if (byte < 0x20) {
      const char escaped[] = { '\\', 'u', '0', '0', hex[byte >> 4], hex[byte & 15] };
      output.write(reinterpret_cast<const uint8_t*>(escaped), sizeof(escaped));
    } else output.write(byte);
  }
  output.write('"');
}

class SyncJsonObject {
  Print& output;
  bool first = true;
  void key(const char* name) {
    if (!first) output.write(',');
    first = false; syncJsonString(output, name, strlen(name)); output.write(':');
  }
public:
  explicit SyncJsonObject(Print& target) : output(target) { output.write('{'); }
  ~SyncJsonObject() { output.write('}'); }
  SyncJsonObject(const SyncJsonObject&) = delete;
  SyncJsonObject& operator=(const SyncJsonObject&) = delete;
  void integer(const char* name, sqlite3_int64 value) {
    key(name); char number[24];
    snprintf(number, sizeof(number), "%lld", static_cast<long long>(value)); output.print(number);
  }
  void number(const char* name, double value) {
    key(name);
    if (!isfinite(value)) { output.print("null"); return; }
    char number[32]; snprintf(number, sizeof(number), "%.15g", value); output.print(number);
  }
  void text(const char* name, sqlite3_stmt* statement, int column) {
    key(name);
    const char* value = reinterpret_cast<const char*>(sqlite3_column_text(statement, column));
    syncJsonString(output, value, value ? static_cast<size_t>(sqlite3_column_bytes(statement, column)) : 0);
  }
};

struct SyncExportStatement {
  sqlite3_stmt* value = nullptr;
  ~SyncExportStatement() { if (value) sqlite3_finalize(value); }
};
struct SyncExportMemory {
  sqlite3* connection;
  explicit SyncExportMemory(sqlite3* value) : connection(value) { if (connection) sqlite3_db_release_memory(connection); }
  ~SyncExportMemory() { if (connection) sqlite3_db_release_memory(connection); }
};

static void writeReadingExportRow(sqlite3_stmt* stmt, Print& output) {
  SyncJsonObject row(output);
  row.integer("reading_id", sqlite3_column_int64(stmt, 0)); row.integer("customer_id", sqlite3_column_int64(stmt, 1));
  row.text("device_uid", stmt, 2);
  row.integer("previous_reading", sqlite3_column_int64(stmt, 3)); row.integer("current_reading", sqlite3_column_int64(stmt, 4)); row.integer("usage_m3", sqlite3_column_int64(stmt, 5));
  const sqlite3_int64 readingAt = sqlite3_column_int64(stmt, 6);
  row.integer("reading_at", readingAt >= 1577836800LL ? readingAt : sqlite3_column_int64(stmt, 10));
  row.integer("synced", sqlite3_column_int64(stmt, 7)); row.text("last_sync", stmt, 8);
  row.text("customer_account_number", stmt, 9); row.text("created_at", stmt, 11);
}
static void writeBillExportRow(sqlite3_stmt* stmt, Print& output) {
  SyncJsonObject row(output);
  row.integer("bill_id", sqlite3_column_int64(stmt, 0)); row.text("reference_number", stmt, 1);
  row.integer("customer_id", sqlite3_column_int64(stmt, 2)); row.integer("reading_id", sqlite3_column_int64(stmt, 3));
  row.text("device_uid", stmt, 4); row.text("bill_date", stmt, 5);
  row.number("rate_per_m3", sqlite3_column_double(stmt, 6)); row.number("charges", sqlite3_column_double(stmt, 7));
  row.number("penalty", sqlite3_column_double(stmt, 8)); row.number("total_due", sqlite3_column_double(stmt, 9));
  row.text("status", stmt, 10); row.integer("synced", sqlite3_column_int64(stmt, 11));
  row.text("last_sync", stmt, 12); row.text("customer_account_number", stmt, 13);
}
static void writeTransactionExportRow(sqlite3_stmt* stmt, Print& output) {
  SyncJsonObject row(output);
  row.integer("bill_transaction_id", sqlite3_column_int64(stmt, 0)); row.integer("bill_id", sqlite3_column_int64(stmt, 1));
  row.text("bill_reference_number", stmt, 2); row.text("type", stmt, 3); row.text("source", stmt, 4);
  row.number("amount", sqlite3_column_double(stmt, 5)); row.number("cash_received", sqlite3_column_double(stmt, 6)); row.number("change", sqlite3_column_double(stmt, 7));
  row.text("transaction_date", stmt, 8); row.text("payment_method", stmt, 9); row.text("processed_by_device_uid", stmt, 10);
  row.text("notes", stmt, 11); row.text("created_at", stmt, 12); row.text("updated_at", stmt, 13);
  row.integer("synced", sqlite3_column_int64(stmt, 14)); row.text("last_sync", stmt, 15);
}

typedef void (*SyncExportRowWriter)(sqlite3_stmt*, Print&);
struct SyncExportStamp { uint64_t hash = 14695981039346656037ULL; uint32_t rows = 0; bool valid = false; };
static SyncExportStamp g_readingExportStamp, g_billExportStamp, g_transactionExportStamp;
class SyncDigestOutput : public Print {
  Print* target;
  SyncExportStamp& stamp;
public:
  using Print::write;
  SyncDigestOutput(Print* output, SyncExportStamp& value) : target(output), stamp(value) {}
  size_t write(uint8_t value) override {
    stamp.hash = (stamp.hash ^ value) * 1099511628211ULL;
    return target ? target->write(value) : 1;
  }
};
static bool syncWriteExportChunk(Print& output, sqlite3_stmt* stmt, const char* prefix, int index, int rows, SyncExportRowWriter writer, SyncExportStamp* stamp = nullptr) {
  output.print(prefix); output.print(index); output.print("|[");
  for (int i = 0; i < rows; ++i) {
    if (sqlite3_step(stmt) != SQLITE_ROW) { output.println(']'); output.println("ERR|SQLITE_EXPORT_FAILED"); return false; }
    if (i) output.write(',');
    if (stamp) { SyncDigestOutput digest(&output, *stamp); writer(stmt, digest); ++stamp->rows; }
    else writer(stmt, output);
    delay(0);
  }
  output.println(']'); return true;
}
static bool syncExportCursorDone(Print& output, sqlite3_stmt* stmt) {
  if (sqlite3_step(stmt) == SQLITE_DONE) return true;
  output.println("ERR|EXPORT_COUNT_CHANGED"); return false;
}
// Compare the last successful pending export under a SQLite write lock.
// A new or edited record leaves the entire pending group untouched.
static bool syncAcknowledgeExport(sqlite3* connection, SyncExportStamp& expected, const char* selectSql, const char* table, const char* timestamp, SyncExportRowWriter writer, Print& output) {
  if (!connection || !expected.valid) { output.println("ERR|EXPORT_REQUIRED"); return false; }
  SyncExportMemory memory(connection);
  if (sqlite3_exec(connection, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr) != SQLITE_OK) { output.println("ERR|ACK_BUSY"); return false; }
  SyncExportStamp current;
  bool valid = true;
  {
    SyncExportStatement statement;
    if (sqlite3_prepare_v2(connection, selectSql, -1, &statement.value, nullptr) != SQLITE_OK) valid = false;
    else {
      SyncDigestOutput digest(nullptr, current);
      int rc;
      while ((rc = sqlite3_step(statement.value)) == SQLITE_ROW) { writer(statement.value, digest); ++current.rows; delay(0); }
      valid = rc == SQLITE_DONE && current.rows == expected.rows && current.hash == expected.hash;
    }
  }
  if (!valid) {
    sqlite3_exec(connection, "ROLLBACK;", nullptr, nullptr, nullptr);
    expected.valid = false; output.println("ERR|EXPORT_CHANGED_REVIEW_AGAIN"); return false;
  }
  char sql[256];
  snprintf(sql, sizeof(sql), "UPDATE %s SET synced=1,last_sync='%s',updated_at='%s' WHERE synced=0 AND last_sync IS NULL;", table, timestamp, timestamp);
  if (sqlite3_exec(connection, sql, nullptr, nullptr, nullptr) != SQLITE_OK || sqlite3_exec(connection, "COMMIT;", nullptr, nullptr, nullptr) != SQLITE_OK) {
    sqlite3_exec(connection, "ROLLBACK;", nullptr, nullptr, nullptr); output.println("ERR|ACK_SAVE_FAILED"); return false;
  }
  expected.valid = false;
  return true;
}
#endif
