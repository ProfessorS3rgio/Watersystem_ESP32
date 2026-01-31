#ifndef SYNC_MANAGER_H
#define SYNC_MANAGER_H

#include "../database/customers_database.h"
#include "../database/device_info.h"
#include "../database/readings_database.h"
#include "../database/bill_database.h"
#include "../database/deduction_database.h"
#include "../database/customer_type_database.h"
#include "../database/barangay_database.h"
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

// Function to handle all sync protocol commands
bool handleSyncCommands(String raw) {
  // ---- Sync protocol (do NOT uppercase; payload may be mixed-case) ----

  if (raw == "EXPORT_DEVICE_INFO") {
    return handleExportDeviceInfo();
  }

  if (raw == "DROP_DB") {
    return handleDropDatabase();
  }

  if (raw.startsWith("SET_TIME|")) {
    String payload = raw.substring(String("SET_TIME|").length());
    return handleSetTime(payload);
  }

  if (raw.startsWith("SET_LAST_SYNC|")) {
    String payload = raw.substring(String("SET_LAST_SYNC|").length());
    return handleSetLastSync(payload);
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

  if (raw == "READINGS_SYNCED") {
    return handleReadingsSynced();
  }

  if (raw.startsWith("UPSERT_CUSTOMERS_JSON|")) {
    String payload = raw.substring(String("UPSERT_CUSTOMERS_JSON|").length());
    return handleUpsertCustomersJson(payload);
  }

  if (raw.startsWith("UPSERT_CUSTOMERS_JSON_CHUNK|")) {
    String payload = raw.substring(String("UPSERT_CUSTOMERS_JSON_CHUNK|").length());
    return handleUpsertCustomersJsonChunk(payload);
  }
  if (raw.startsWith("UPSERT_NEW_CUSTOMER_JSON_CHUNK|")) {
    String payload = raw.substring(String("UPSERT_NEW_CUSTOMER_JSON_CHUNK|").length());
    return handleUpsertNewCustomerJsonChunk(payload);
  }

  if (raw.startsWith("UPSERT_UPDATED_CUSTOMER_JSON_CHUNK|")) {
    String payload = raw.substring(String("UPSERT_UPDATED_CUSTOMER_JSON_CHUNK|").length());
    return handleUpsertUpdatedCustomerJsonChunk(payload);
  }

  if (raw.startsWith("UPSERT_DEDUCTION|")) {
    String payload = raw.substring(String("UPSERT_DEDUCTION|").length());
    return handleUpsertDeduction(payload);
  }

  if (raw.startsWith("UPSERT_BARANGAY|")) {
    String payload = raw.substring(String("UPSERT_BARANGAY|").length());
    return handleUpsertBarangay(payload);
  }

  if (raw.startsWith("UPSERT_CUSTOMER_TYPE|")) {
    String payload = raw.substring(String("UPSERT_CUSTOMER_TYPE|").length());
    return handleUpsertCustomerType(payload);
  }

  if (raw.startsWith("UPSERT_BILLS_JSON_CHUNK|")) {
    String payload = raw.substring(String("UPSERT_BILLS_JSON_CHUNK|").length());
    return handleUpsertBillsJsonChunk(payload);
  }

  if (raw.startsWith("UPSERT_BILL_TRANSACTIONS_JSON_CHUNK|")) {
    String payload = raw.substring(String("UPSERT_BILL_TRANSACTIONS_JSON_CHUNK|").length());
    return handleUpsertBillTransactionsJsonChunk(payload);
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