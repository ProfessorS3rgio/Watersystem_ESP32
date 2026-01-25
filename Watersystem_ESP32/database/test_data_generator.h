#ifndef TEST_DATA_GENERATOR_H
#define TEST_DATA_GENERATOR_H

#include "../configuration/config.h"
#include "customers_database.h"
#include "readings_database.h"
#include "bill_database.h"
#include <Arduino.h>
#include <vector>

// ===== TEST DATA GENERATOR =====

// Function to generate random test readings and bills
void generateTestReadingsAndBills(int count) {
  Serial.print(F("Generating "));
  Serial.print(count);
  Serial.println(F(" test readings and bills..."));

  int customerCount = getCustomerCount();
  if (customerCount == 0) {
    Serial.println(F("No customers found!"));
    return;
  }

  // Helper to count rows in a table
  auto getTableCount = [&](const char* tableName) -> int {
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
  };

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

  int numToGenerate = min(count, (int)accounts.size());
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

  // Verify counts after commit
  int afterReadings = getTableCount("readings");
  int afterBills = getTableCount("bills");
  Serial.print(F("After: readings=")); Serial.print(afterReadings);
  Serial.print(F(", bills=")); Serial.println(afterBills);
  Serial.print(F("Inserted readings: ")); Serial.println(afterReadings - beforeReadings);
  Serial.print(F("Inserted bills: ")); Serial.println(afterBills - beforeBills);
}

#endif  // TEST_DATA_GENERATOR_H