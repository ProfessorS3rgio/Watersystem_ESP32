#ifndef READING_SYNC_H
#define READING_SYNC_H

#include "../../database/readings_database.h"
#include "../../database/bill_database.h"
#include <ArduinoJson.h>

// ===== READING SYNC OPERATIONS =====

// Handle EXPORT_READINGS command
bool handleExportReadings() {
  Serial.println(F("Exporting readings..."));
  // Load readings if not loaded
  if (readings.empty()) {
    loadReadingsFromDB();
  }
  const int CHUNK_SIZE = 10;
  int totalReadings = readings.size();
  int totalChunks = (totalReadings + CHUNK_SIZE - 1) / CHUNK_SIZE;
  Serial.print(F("BEGIN_READINGS_JSON|"));
  Serial.println(totalChunks);
  for (int chunk = 0; chunk < totalChunks; ++chunk) {
    Serial.printf("Heap free before chunk %d: %d\n", chunk, ESP.getFreeHeap());
    int start = chunk * CHUNK_SIZE;
    int end = min(start + CHUNK_SIZE, totalReadings);
    DynamicJsonDocument doc(16384);
    JsonArray arr = doc.to<JsonArray>();
    for (int i = start; i < end; ++i) {
      const auto& r = readings[i];
      JsonObject obj = arr.createNestedObject();
      obj["reading_id"] = r.reading_id;
      obj["customer_id"] = r.customer_id;
      obj["device_uid"] = r.device_uid;
      obj["previous_reading"] = r.previous_reading;
      obj["current_reading"] = r.current_reading;
      obj["usage_m3"] = r.usage_m3;
      obj["reading_at"] = r.reading_at.toInt();
    }
    Serial.print(F("READINGS_CHUNK|"));
    Serial.print(chunk);
    Serial.print(F("|"));
    serializeJson(arr, Serial);
    Serial.println();
    doc.clear();
    Serial.printf("Heap free after chunk %d: %d\n", chunk, ESP.getFreeHeap());
  }
  Serial.println(F("END_READINGS_JSON"));
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