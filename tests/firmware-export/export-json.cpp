// Native check for the real firmware serializer. No Arduino or device required.
#include <new>
#include <stdlib.h>
static bool forbidAllocation = false;
void* operator new(size_t size) { if (forbidAllocation) abort(); void* p=malloc(size); if (!p) throw std::bad_alloc(); return p; }
void operator delete(void* p) noexcept { free(p); }
#include "../../Watersystem_ESP32/master/managers/sync/export_json.h"
int main() {
  Print output;
  sqlite3_stmt readings;
  forbidAllocation = true;
  for (int chunk=0;chunk<150;++chunk) {
    output.used=0;
    assert(syncWriteExportChunk(output,&readings,"READINGS_CHUNK|",chunk,20,writeReadingExportRow));
    fwrite(output.buffer,1,output.used,stdout);
  }
  assert(syncExportCursorDone(output,&readings));
  output.used=0;
  writeBillExportRow(&readings,output); output.write('\n');
  writeTransactionExportRow(&readings,output); output.write('\n');
  fwrite(output.buffer,1,output.used,stdout);
  output.used=0;
  assert(!syncWriteExportChunk(output,&readings,"READINGS_CHUNK|",0,1,writeReadingExportRow));
  assert(strstr(output.buffer,"ERR|SQLITE_EXPORT_FAILED"));
  sqlite3_stmt extra;
  assert(!syncExportCursorDone(output,&extra));
  // Exercise the real ACK guard with deterministic SQLite stand-ins.
  for (int scenario=0;scenario<5;++scenario) {
    sqlite3 connection;
    sqlite3_stmt cursor; cursor.limit=2; cursor.owner=&connection;
    SyncExportStamp stamp;
    SyncDigestOutput digest(nullptr,stamp);
    while (sqlite3_step(&cursor)==SQLITE_ROW) { writeReadingExportRow(&cursor,digest); ++stamp.rows; }
    stamp.valid=true;
    if (scenario==1) connection.limit=3; // New reading after review.
    if (scenario==2) connection.edits=1; // Existing reading edited after review.
    if (scenario==3) connection.failUpdate=true;
    if (scenario==4) stamp.valid=false;
    output.used=0;
    const bool accepted=syncAcknowledgeExport(&connection,stamp,"SELECT pending", "readings", "2026-10-26 00:00:00",writeReadingExportRow,output);
    assert(accepted==(scenario==0));
    assert(connection.commits==(scenario==0?1:0));
    assert(connection.updates==(scenario==0?1:0));
    if (scenario==1 || scenario==2) assert(strstr(output.buffer,"EXPORT_CHANGED_REVIEW_AGAIN"));
  }
  forbidAllocation = false;
}
