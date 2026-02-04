#ifndef BILL_TRANSACTIONS_DATABASE_H
#define BILL_TRANSACTIONS_DATABASE_H

#include "../configuration/config.h"
#include <vector>
#include <sqlite3.h>

// ===== BILL TRANSACTION DATA STRUCTURE =====
struct BillTransaction {
  int bill_transaction_id;
  int bill_id;
  String bill_reference_number;
  String type;  // e.g., "payment", "adjustment"
  String source;  // e.g., "cash", "online"
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
  char sql[1024];
  String nowStr = getCurrentDateTimeString();
  sprintf(sql, "INSERT INTO bill_transactions (bill_id, bill_reference_number, type, source, amount, cash_received, change, transaction_date, payment_method, processed_by_device_uid, notes, created_at, updated_at, synced, last_sync) VALUES (%d, '%s', '%s', '%s', %f, %f, %f, '%s', '%s', '%s', '%s', '%s', '%s', 0, NULL);",
          transaction.bill_id, transaction.bill_reference_number.c_str(), transaction.type.c_str(), transaction.source.c_str(),
          transaction.amount, transaction.cash_received, transaction.change, transaction.transaction_date.c_str(),
          transaction.payment_method.c_str(), transaction.processed_by_device_uid.c_str(), transaction.notes.c_str(),
          nowStr.c_str(), nowStr.c_str());
  int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
  Serial.print(F("Bill transaction save result: "));
  Serial.println(rc == SQLITE_OK ? "OK" : "FAILED");
  return rc == SQLITE_OK;
}

// Function to create a payment transaction
bool recordPaymentTransaction(int billId, String billRef, float amount, float cashReceived, float changeAmount) {
  BillTransaction bt;
  bt.bill_id = billId;
  bt.bill_reference_number = billRef;
  bt.type = "payment";
  bt.source = "cash";
  bt.amount = amount;
  bt.cash_received = cashReceived;
  bt.change = changeAmount;
  bt.transaction_date = getCurrentDateTimeString();
  bt.payment_method = "cash";
  bt.processed_by_device_uid = getDeviceUID();
  bt.notes = "Payment recorded via ESP32 device";
  
  return saveBillTransactionToDB(bt);
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