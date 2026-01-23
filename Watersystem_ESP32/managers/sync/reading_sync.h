#ifndef READING_SYNC_H
#define READING_SYNC_H

#include "../../database/readings_database.h"
#include "../../database/bill_database.h"

// ===== READING SYNC OPERATIONS =====

// Handle EXPORT_READINGS command
bool handleExportReadings() {
  Serial.println(F("Exporting readings..."));
  exportReadingsForSync();
  return true;
}

// Handle EXPORT_BILLS command
bool handleExportBills() {
  Serial.println(F("Exporting bills..."));
  exportBillsForSync();
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