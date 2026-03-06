#ifndef SYNC_MANAGER_H
#define SYNC_MANAGER_H

#include "../database/customers_database.h"
#include "../database/device_info.h"
#include "../database/readings_database.h"
#include "../database/bill_database.h"
#include "../database/deduction_database.h"
#include "../database/customer_type_database.h"
#include "../database/barangay_database.h"
#include "../database/settings_database.h"
#include "../configuration/config.h"
#include <ArduinoJson.h>
#include <vector>
#include <SD.h>

// Include organized sync handlers
#include "sync/device_sync.h"
#include "sync/reading_sync.h"
#include "sync/bill_sync.h"
#include "sync/bill_transaction_sync.h"
#include "sync/deduction_sync.h"
#include "sync/barangay_sync.h"
#include "sync/customer_type_sync.h"
#include "sync/customer_sync.h"
#include "sync/settings_sync.h"

// Function to handle all sync protocol commands
bool handleSyncCommands(String raw) {
  // Debug: show incoming line and length so we can troubleshoot malformed
  // commands (e.g. BAD_CHUNK_FORMAT).  This will appear in the serial log on
  // the host.
  Serial.print(F("RX_SYNC [len="));
  Serial.print(raw.length());
  Serial.print(F("] : "));
  if (raw.length() > 256) {
    Serial.print(raw.substring(0, 256));
    Serial.println(F("...[truncated]"));
  } else {
    Serial.println(raw);
  }

  // ---- Sync protocol (do NOT uppercase; payload may be mixed-case) ----

  if (raw == "EXPORT_DEVICE_INFO") {
    return handleExportDeviceInfo();
  }

  if (raw == "DROP_DB") {
    return handleDropDatabase();
  }

  if (raw.startsWith("SET_TIME|")) {
    raw.remove(0, String("SET_TIME|").length());
    return handleSetTime(raw);
  }

  if (raw.startsWith("SET_LAST_SYNC|")) {
    raw.remove(0, String("SET_LAST_SYNC|").length());
    return handleSetLastSync(raw);
  }

  if (raw == "EXPORT_READINGS") {
    return handleExportReadings();
  }

  if (raw == "EXPORT_BILLS") {
    return handleExportBills();
  }

  if (raw == "EXPORT_BILL_TRANSACTIONS") {
    return handleExportBillTransactions();
  }

  if (raw == "BILLS_SYNCED") {
    return handleBillsSynced();
  }

  if (raw == "BILL_TRANSACTIONS_SYNCED") {
    return handleBillTransactionsSynced();
  }

  if (raw == "READINGS_SYNCED") {
    return handleReadingsSynced();
  }

  if (raw.startsWith("UPSERT_CUSTOMERS_JSON|")) {
    raw.remove(0, String("UPSERT_CUSTOMERS_JSON|").length());
    return handleUpsertCustomersJson(raw);
  }

  if (raw.startsWith("UPSERT_CUSTOMERS_JSON_CHUNK|")) {
    raw.remove(0, String("UPSERT_CUSTOMERS_JSON_CHUNK|").length());
    return handleUpsertCustomersJsonChunk(raw);
  }
  if (raw.startsWith("UPSERT_NEW_CUSTOMER_JSON_CHUNK|")) {
    raw.remove(0, String("UPSERT_NEW_CUSTOMER_JSON_CHUNK|").length());
    return handleUpsertNewCustomerJsonChunk(raw);
  }

  if (raw.startsWith("UPSERT_UPDATED_CUSTOMER_JSON_CHUNK|")) {
    raw.remove(0, String("UPSERT_UPDATED_CUSTOMER_JSON_CHUNK|").length());
    return handleUpsertUpdatedCustomerJsonChunk(raw);
  }

  if (raw.startsWith("UPSERT_DEDUCTION|")) {
    raw.remove(0, String("UPSERT_DEDUCTION|").length());
    return handleUpsertDeduction(raw);
  }

  if (raw.startsWith("UPSERT_BARANGAY|")) {
    raw.remove(0, String("UPSERT_BARANGAY|").length());
    return handleUpsertBarangay(raw);
  }

  if (raw.startsWith("UPSERT_CUSTOMER_TYPE|")) {
    raw.remove(0, String("UPSERT_CUSTOMER_TYPE|").length());
    return handleUpsertCustomerType(raw);
  }

  if (raw.startsWith("UPSERT_SETTINGS|")) {
    raw.remove(0, String("UPSERT_SETTINGS|").length());
    return handleUpsertSettings(raw);
  }

  if (raw.startsWith("UPSERT_BILLS_JSON_CHUNK|")) {
    raw.remove(0, String("UPSERT_BILLS_JSON_CHUNK|").length());
    return handleUpsertBillsJsonChunk(raw);
  }

  if (raw == "RELOAD_SD") {
    return handleReloadSD();
  }

  if (raw == "FORMAT_SD") {
    return handleFormatSD();
  }

  if (raw == "RESTART_DEVICE") {
    return handleRestartDevice();
  }

  // If command not recognized, log it for debugging
  Serial.print(F("UNKNOWN_COMMAND: "));
  Serial.println(raw);
  return false;
}

#endif // SYNC_MANAGER_H