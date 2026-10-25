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
  forbidAllocation = false;
}
