#ifndef BILL_TRANSACTIONS_DATABASE_H
#define BILL_TRANSACTIONS_DATABASE_H

#include "../configuration/config.h"
#include <Arduino.h>
#include <vector>
#include <sqlite3.h>
#include "device_info.h"      // getDeviceUID(), getCurrentDateTimeString()

// ===== RECEIPT DATA (used by receipt_printer + payment flow) =====
struct ReceiptData {
  String receiptNumber;
  String paymentDateTime;

  String customerName;
  String accountNo;
  String customerType;
  String address;
  String collector;

  unsigned long prevReading;
  unsigned long currReading;
  unsigned long usage;

  float rate;
  float subtotal;
  float deductions;
  float penalty;
  float total;

  float amountPaid;
  float change;

  String billRefNumber;
};

extern ReceiptData currentReceipt;

static int getBillIdByReferenceNumber(const String& billRef) {
  const char* sql = "SELECT bill_id FROM bills WHERE reference_number = ? LIMIT 1;";
  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.print(F("Failed to prepare getBillIdByReferenceNumber: "));
    Serial.println(sqlite3_errmsg(db));
    return 0;
  }

  sqlite3_bind_text(stmt, 1, billRef.c_str(), -1, SQLITE_TRANSIENT);

  int billId = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    billId = sqlite3_column_int(stmt, 0);
  }
  sqlite3_finalize(stmt);
  return billId;
}

static bool markBillPaidById(int billId) {
  const char* sql = "UPDATE bills SET status = 'Paid', updated_at = ? WHERE bill_id = ?;";
  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.print(F("Failed to prepare markBillPaidById: "));
    Serial.println(sqlite3_errmsg(db));
    return false;
  }

  String nowStr = getCurrentDateTimeString();
  sqlite3_bind_text(stmt, 1, nowStr.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 2, billId);

  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return rc == SQLITE_DONE;
}

// ===== BILL TRANSACTION DATA STRUCTURE =====
struct BillTransaction {
  int bill_transaction_id;
  int bill_id;
  String bill_reference_number;
  String type;  // e.g., "payment", "adjustment"
  String source;  // "Office" | "Device"
  float amount;
  float cash_received;
  float change;
  String transaction_date;
  String payment_method;
  String processed_by_device_uid;
  String notes;
  String created_at;
  String updated_at;
  bool synced;
  String last_sync;
};

// ===== BILL TRANSACTIONS DATABASE =====
std::vector<BillTransaction> billTransactions;

static int loadBillTransactionCallback(void *data, int argc, char **argv, char **azColName) {
  BillTransaction bt;
  bt.bill_transaction_id = atoi(argv[0]);
  bt.bill_id = atoi(argv[1]);
  bt.bill_reference_number = argv[2] ? argv[2] : "";
  bt.type = argv[3] ? argv[3] : "";
  bt.source = argv[4] ? argv[4] : "";
  bt.amount = atof(argv[5]);
  bt.cash_received = atof(argv[6]);
  bt.change = atof(argv[7]);
  bt.transaction_date = argv[8] ? argv[8] : "";
  bt.payment_method = argv[9] ? argv[9] : "";
  bt.processed_by_device_uid = argv[10] ? argv[10] : "";
  bt.notes = argv[11] ? argv[11] : "";
  bt.created_at = argv[12] ? argv[12] : "";
  bt.updated_at = argv[13] ? argv[13] : "";
  bt.synced = atoi(argv[14]);
  bt.last_sync = argv[15] ? argv[15] : "";
  billTransactions.push_back(bt);
  return 0;
}

void loadBillTransactionsFromDB() {
  billTransactions.clear();
  const char *sql = "SELECT bill_transaction_id, bill_id, bill_reference_number, type, source, amount, cash_received, change, transaction_date, payment_method, processed_by_device_uid, notes, created_at, updated_at, synced, last_sync FROM bill_transactions ORDER BY bill_transaction_id;";
  sqlite3_exec(db, sql, loadBillTransactionCallback, NULL, NULL);
}

bool saveBillTransactionToDB(BillTransaction transaction) {
  const char* sql =
    "INSERT INTO bill_transactions ("
    "bill_id, bill_reference_number, type, source, amount, cash_received, change, "
    "transaction_date, payment_method, processed_by_device_uid, notes, created_at, updated_at, synced, last_sync"
    ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 0, NULL);";

  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.print(F("Failed to prepare saveBillTransactionToDB: "));
    Serial.println(sqlite3_errmsg(db));
    return false;
  }

  String nowStr = getCurrentDateTimeString();
  if (transaction.transaction_date.length() == 0) {
    transaction.transaction_date = nowStr;
  }

  // This code runs on the ESP32 device, so the transaction source is always Device.
  transaction.source = "Device";

  sqlite3_bind_int(stmt, 1, transaction.bill_id);
  sqlite3_bind_text(stmt, 2, transaction.bill_reference_number.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, transaction.type.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 4, transaction.source.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_double(stmt, 5, transaction.amount);
  sqlite3_bind_double(stmt, 6, transaction.cash_received);
  sqlite3_bind_double(stmt, 7, transaction.change);
  sqlite3_bind_text(stmt, 8, transaction.transaction_date.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 9, transaction.payment_method.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 10, transaction.processed_by_device_uid.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 11, transaction.notes.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 12, nowStr.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 13, nowStr.c_str(), -1, SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  Serial.print(F("Bill transaction save result: "));
  Serial.println(rc == SQLITE_DONE ? "OK" : "FAILED");
  return rc == SQLITE_DONE;
}

// Function to create a payment transaction
bool recordPaymentTransaction(int billId, String billRef, float amount, float cashReceived, float changeAmount) {
  BillTransaction bt;
  bt.bill_id = billId;
  bt.bill_reference_number = billRef;
  bt.type = "payment";
  bt.source = "Device";
  bt.amount = amount;
  bt.cash_received = cashReceived;
  bt.change = changeAmount;
  bt.transaction_date = getCurrentDateTimeString();
  bt.payment_method = "cash";
  bt.processed_by_device_uid = getDeviceUID();
  bt.notes = "Payment recorded via ESP32 device";
  
  bool ok = saveBillTransactionToDB(bt);
  if (ok) {
    (void)markBillPaidById(billId);
  }
  return ok;
}

// Convenience: record a cash payment by bill reference number (looks up bill_id internally)
bool recordPaymentTransactionByBillRef(const String& billRef, float amountDue, float cashReceived, float changeAmount) {
  int billId = getBillIdByReferenceNumber(billRef);
  if (billId <= 0) {
    Serial.print(F("Failed to find bill_id for ref: "));
    Serial.println(billRef);
    return false;
  }
  return recordPaymentTransaction(billId, billRef, amountDue, cashReceived, changeAmount);
}

// ===== MARK ALL BILL TRANSACTIONS SYNCED =====
bool markAllBillTransactionsSynced() {
  String nowStr = getCurrentDateTimeString();
  char sql[128];
  sprintf(sql, "UPDATE bill_transactions SET synced = 1, last_sync = '%s', updated_at = '%s';", nowStr.c_str(), nowStr.c_str());
  int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
  return rc == SQLITE_OK;
}

#endif // BILL_TRANSACTIONS_DATABASE_H