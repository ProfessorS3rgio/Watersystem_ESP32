#ifndef BILL_TRANSACTION_DATABASE_H
#define BILL_TRANSACTION_DATABASE_H

#include "../configuration/config.h"
#include "database_manager.h"
#include <vector>
#include <ArduinoJson.h>

// ===== BILL TRANSACTION STRUCTURE =====
struct BillTransaction {
  int bill_transaction_id;
  int bill_id;
  String bill_reference_number;
  String type;
  String source;
  float amount;
  float cash_received;
  float change;
  String transaction_date;
  String payment_method;
  String processed_by_device_uid;
  String notes;
  String created_at;
  String updated_at;
};

// ===== BILL TRANSACTION DATABASE =====
std::vector<BillTransaction> billTransactions;

static int loadBillTransactionCallback(void *data, int argc, char **argv, char **azColName) {
  BillTransaction bt;
  bt.bill_transaction_id = atoi(argv[0]);
  bt.bill_id = atoi(argv[1]);
  bt.bill_reference_number = argv[2];
  bt.type = argv[3];
  bt.source = argv[4];
  bt.amount = atof(argv[5]);
  bt.cash_received = atof(argv[6]);
  bt.change = atof(argv[7]);
  bt.transaction_date = argv[8];
  bt.payment_method = argv[9];
  bt.processed_by_device_uid = argv[10];
  bt.notes = argv[11] ? argv[11] : "";
  bt.created_at = argv[12];
  bt.updated_at = argv[13];
  billTransactions.push_back(bt);
  return 0;
}

void loadBillTransactionsFromDB() {
  billTransactions.clear();
  const char *sql = "SELECT bill_transaction_id, bill_id, bill_reference_number, type, source, amount, cash_received, change, transaction_date, payment_method, processed_by_device_uid, notes, created_at, updated_at FROM bill_transactions;";
  sqlite3_exec(db, sql, loadBillTransactionCallback, NULL, NULL);
}

std::vector<BillTransaction> getBillTransactionsChunk(int offset, int limit) {
  std::vector<BillTransaction> chunk;
  chunk.reserve(limit);
  char sql[256];
  sprintf(sql, "SELECT bill_transaction_id, bill_id, bill_reference_number, type, source, amount, cash_received, change, transaction_date, payment_method, processed_by_device_uid, notes, created_at, updated_at FROM bill_transactions ORDER BY bill_transaction_id LIMIT %d OFFSET %d;", limit, offset);
  sqlite3_exec(db, sql, [](void *data, int argc, char **argv, char **azColName) -> int {
    std::vector<BillTransaction>* chunk = static_cast<std::vector<BillTransaction>*>(data);
    BillTransaction bt;
    bt.bill_transaction_id = atoi(argv[0]);
    bt.bill_id = atoi(argv[1]);
    bt.bill_reference_number = argv[2];
    bt.type = argv[3];
    bt.source = argv[4];
    bt.amount = atof(argv[5]);
    bt.cash_received = atof(argv[6]);
    bt.change = atof(argv[7]);
    bt.transaction_date = argv[8];
    bt.payment_method = argv[9];
    bt.processed_by_device_uid = argv[10];
    bt.notes = argv[11] ? argv[11] : "";
    bt.created_at = argv[12];
    bt.updated_at = argv[13];
    chunk->push_back(bt);
    return 0;
  }, &chunk, NULL);
  return chunk;
}

int getTotalBillTransactions() {
  int count = 0;
  const char *sql = "SELECT COUNT(*) FROM bill_transactions;";
  sqlite3_exec(db, sql, [](void *data, int argc, char **argv, char **azColName) -> int {
    int* count = (int*)data;
    *count = atoi(argv[0]);
    return 0;
  }, &count, NULL);
  return count;
}

void initBillTransactionDatabase() {
  // Note: We don't load all bill transactions into memory at startup
  // to save heap space. They are queried from DB as needed for sync.
  Serial.println(F("Bill transaction database initialized (lazy loading)."));
}

#endif // BILL_TRANSACTION_DATABASE_H