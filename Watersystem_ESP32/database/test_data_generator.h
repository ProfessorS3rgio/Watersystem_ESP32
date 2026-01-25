#ifndef TEST_DATA_GENERATOR_H
#define TEST_DATA_GENERATOR_H

#include "../configuration/config.h"
#include "customers_database.h"
#include "readings_database.h"
#include "bill_database.h"
#include <Arduino.h>
#include <vector>

// ===== TEST DATA GENERATOR =====

// Helper to count rows in a table
static int getTableCount(const char* tableName) {
  char q[64];
  sqlite3_stmt* cstmt;
  sprintf(q, "SELECT COUNT(*) FROM %s;", tableName);
  int rc = sqlite3_prepare_v2(db, q, -1, &cstmt, NULL);
  if (rc != SQLITE_OK) return -1;
  rc = sqlite3_step(cstmt);
  int cnt = 0;
  if (rc == SQLITE_ROW) cnt = sqlite3_column_int(cstmt, 0);
  sqlite3_finalize(cstmt);
  return cnt;
}

// Function to generate test bill transactions
void generateTestBillTransactions(int billCount) {
  if (billCount == 0) return;

  Serial.print(F("Generating transactions for "));
  Serial.print(billCount);
  Serial.println(F(" bills..."));

  // Get some bills to create transactions for
  sqlite3_stmt* stmt;
  const char* sql = "SELECT bill_id, reference_number, total_due FROM bills ORDER BY bill_id DESC LIMIT ?;";
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.println(F("Failed to prepare bill query"));
    return;
  }
  sqlite3_bind_int(stmt, 1, billCount);

  sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);

  // Prepare check statement once
  sqlite3_stmt* checkStmt;
  const char* checkSql = "SELECT COUNT(*) FROM bill_transactions WHERE bill_id = ?;";
  int rc_check = sqlite3_prepare_v2(db, checkSql, -1, &checkStmt, NULL);
  if (rc_check != SQLITE_OK) {
    Serial.print(F("Failed to prepare check statement: "));
    Serial.println(sqlite3_errmsg(db));
    sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
    return;
  }

  int transCount = 0;
  while (sqlite3_step(stmt) == SQLITE_ROW && transCount < billCount) {
    int billId = sqlite3_column_int(stmt, 0);
    String refNum = String((const char*)sqlite3_column_text(stmt, 1));
    float totalDue = sqlite3_column_double(stmt, 2);

    // Check if transaction already exists for this bill
    bool exists = false;
    sqlite3_reset(checkStmt);
    sqlite3_bind_int(checkStmt, 1, billId);
    if (sqlite3_step(checkStmt) == SQLITE_ROW) {
      exists = sqlite3_column_int(checkStmt, 0) > 0;
    }
    if (exists) continue;  // Skip if transaction already exists

    // Create a payment transaction
    const char* transSql = "INSERT INTO bill_transactions (bill_id, bill_reference_number, type, source, amount, cash_received, change, transaction_date, payment_method, processed_by_device_uid, notes, created_at, updated_at) VALUES (?, ?, 'payment', 'Device', ?, ?, 0, datetime('now'), 'cash', ?, 'Test payment', datetime('now'), datetime('now'));";
    sqlite3_stmt* transStmt;
    int rc2 = sqlite3_prepare_v2(db, transSql, -1, &transStmt, NULL);
    if (rc2 == SQLITE_OK) {
      sqlite3_bind_int(transStmt, 1, billId);
      sqlite3_bind_text(transStmt, 2, refNum.c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_double(transStmt, 3, totalDue);
      sqlite3_bind_double(transStmt, 4, totalDue);
      sqlite3_bind_text(transStmt, 5, getDeviceUID().c_str(), -1, SQLITE_TRANSIENT);
      int rc_step = sqlite3_step(transStmt);
      if (rc_step == SQLITE_DONE) {
        transCount++;
        // Progress indicator
        if (transCount % 100 == 0) {
          Serial.print(F("Generated "));
          Serial.print(transCount);
          Serial.println(F(" transactions..."));
        }
      } else {
        Serial.print(F("Transaction insert failed: "));
        Serial.println(sqlite3_errmsg(db));
      }
      sqlite3_finalize(transStmt);
    } else {
      Serial.print(F("Transaction prepare failed: "));
      Serial.println(sqlite3_errmsg(db));
    }

    // Yield to prevent watchdog reset
    YIELD_WDT();
  }

  sqlite3_finalize(stmt);
  sqlite3_finalize(checkStmt);
  sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);

  Serial.print(F("Generated "));
  Serial.print(transCount);
  Serial.println(F(" test transactions."));
}

// Function to generate random test readings and bills
void generateTestReadingsAndBills(int count) {
  Serial.print(F("Generating "));
  Serial.print(count);
  Serial.println(F(" test readings and bills..."));

  // Check current counts
  int currentReadings = getTableCount("readings");
  int currentBills = getTableCount("bills");
  int currentTransactions = getTableCount("bill_transactions");

  Serial.print(F("Current: readings=")); Serial.print(currentReadings);
  Serial.print(F(", bills=")); Serial.print(currentBills);
  Serial.print(F(", transactions=")); Serial.println(currentTransactions);

  if (currentReadings >= 1000) {
    Serial.println(F("Readings already at 1000, skipping..."));
    return;
  }
  if (currentBills >= 1000) {
    Serial.println(F("Bills already at 1000, skipping..."));
    return;
  }
  if (currentTransactions >= 1000) {
    Serial.println(F("Transactions already at 1000, skipping..."));
    return;
  }

  int maxToGenerate = min(count, 1000 - max({currentReadings, currentBills, currentTransactions}));
  if (maxToGenerate <= 0) {
    Serial.println(F("Cannot generate more, at limit."));
    return;
  }

  int customerCount = getCustomerCount();
  if (customerCount == 0) {
    Serial.println(F("No customers found!"));
    return;
  }

  int beforeReadings = getTableCount("readings");
  int beforeBills = getTableCount("bills");
  Serial.print(F("Before: readings=")); Serial.print(beforeReadings);
  Serial.print(F(", bills=")); Serial.println(beforeBills);

  // Pre-load account numbers for customers with no readings (only insert new data)
  std::vector<String> accounts;
  const char* sql = "SELECT c.account_no FROM customers c LEFT JOIN readings r ON c.customer_id = r.customer_id WHERE r.customer_id IS NULL;";
  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    Serial.println(F("Failed to load accounts with no readings"));
    return;
  }
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    accounts.push_back(String((const char*)sqlite3_column_text(stmt, 0)));
  }
  sqlite3_finalize(stmt);

  if (accounts.size() == 0) {
    Serial.println(F("All customers already have readings!"));
    return;
  }

  int numToGenerate = min(maxToGenerate, (int)accounts.size());
  Serial.print(F("Found "));
  Serial.print(accounts.size());
  Serial.print(F(" customers with no readings. Generating "));
  Serial.print(numToGenerate);
  Serial.println(F(" new bills..."));

  // Begin transaction for faster bulk inserts
  // Enable bulk mode to avoid growing in-memory bill list
  g_bulkInsertMode = true;
  sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);

  int successCount = 0;
  const int batchSize = 50;  // Commit every 50 inserts to avoid rollback on interruption

  for (int i = 0; i < numToGenerate; i++) {
    String account = accounts[i];

    // Get previous reading (should be 0 for new customers)
    unsigned long prev = getPreviousReadingForAccount(account);

    // Generate random current reading (previous + 1 to 100 m3)
    unsigned long curr = prev + random(1, 101); // 1-100 m3 usage

    // Generate bill (this will create new reading since customer has no readings)
    bool billGenerated = generateBillForCustomer(account, curr);
    if (billGenerated) {
      successCount++;
      // Print heap less frequently to reduce Serial allocations
      if ((i + 1) % 50 == 0 || i == numToGenerate - 1) {
        Serial.print(F("Heap after insert "));
        Serial.print(i + 1);
        Serial.print(F(": "));
        Serial.println(ESP.getFreeHeap());
      }
    }

    // Progress indicator
    if ((i + 1) % 100 == 0) {
      Serial.print(F("Generated "));
      Serial.print(i + 1);
      Serial.println(F("..."));
    }

    // Yield to prevent watchdog reset
    YIELD_WDT();

    // Commit in batches to save progress
    if ((i + 1) % batchSize == 0 || i == numToGenerate - 1) {
      sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
      sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);  // Start new transaction for next batch
    }
  }

  // Final commit if not already done
  sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
  // Disable bulk mode and release preloaded account list
  g_bulkInsertMode = false;
  
  // Release preloaded account list to free heap immediately
  accounts.clear();
  accounts.shrink_to_fit();

  Serial.print(F("Successfully generated "));
  Serial.print(successCount);
  Serial.println(F(" test readings and bills."));

  // Now generate some bill transactions for the new bills
  generateTestBillTransactions(successCount);

  // Verify counts after commit
  int afterReadings = getTableCount("readings");
  int afterBills = getTableCount("bills");
  int afterTransactions = getTableCount("bill_transactions");
  Serial.print(F("After: readings=")); Serial.print(afterReadings);
  Serial.print(F(", bills=")); Serial.print(afterBills);
  Serial.print(F(", transactions=")); Serial.println(afterTransactions);
  Serial.print(F("Inserted readings: ")); Serial.println(afterReadings - currentReadings);
  Serial.print(F("Inserted bills: ")); Serial.println(afterBills - currentBills);
  Serial.print(F("Inserted transactions: ")); Serial.println(afterTransactions - currentTransactions);
}

#endif  // TEST_DATA_GENERATOR_H