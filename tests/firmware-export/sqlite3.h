#pragma once
#include <string.h>
typedef long long sqlite3_int64;
struct sqlite3;
struct sqlite3_stmt { int row = 0; int limit = 3000; sqlite3* owner = nullptr; };
struct sqlite3 { int limit = 2; int edits = 0; int updates = 0; int commits = 0; int rollbacks = 0; bool failUpdate = false; };
#define SQLITE_OK 0
#define SQLITE_ROW 100
#define SQLITE_DONE 101
inline int sqlite3_step(sqlite3_stmt* stmt) { return ++stmt->row <= stmt->limit ? SQLITE_ROW : SQLITE_DONE; }
inline int sqlite3_finalize(sqlite3_stmt*) { return 0; }
inline int sqlite3_db_release_memory(sqlite3*) { return 0; }
inline int sqlite3_prepare_v2(sqlite3* db, const char*, int, sqlite3_stmt** output, const char**) {
  static sqlite3_stmt cursor;
  cursor.row = 0; cursor.limit = db->limit; cursor.owner = db; *output = &cursor;
  return SQLITE_OK;
}
inline int sqlite3_exec(sqlite3* db, const char* sql, void*, void*, void*) {
  if (strstr(sql,"UPDATE ")) { if (db->failUpdate) return 1; ++db->updates; }
  if (strstr(sql,"COMMIT")) ++db->commits;
  if (strstr(sql,"ROLLBACK")) ++db->rollbacks;
  return SQLITE_OK;
}
inline sqlite3_int64 sqlite3_column_int64(sqlite3_stmt* stmt, int col) { return col == 0 ? stmt->row : col == 6 ? 0 : col == 10 ? 1792934229LL : 1234567890123LL + (stmt->owner && col == 4 ? stmt->owner->edits : 0); }
inline double sqlite3_column_double(sqlite3_stmt*, int) { return 1234.56; }
inline const unsigned char* sqlite3_column_text(sqlite3_stmt*, int col) {
  static const char text[] = "Doña | \\\"\n\r\t\001\000end";
  return col == 8 ? nullptr : reinterpret_cast<const unsigned char*>(text);
}
inline int sqlite3_column_bytes(sqlite3_stmt*, int col) {
  return col == 8 ? 0 : sizeof("Doña | \\\"\n\r\t\001\000end") - 1;
}
