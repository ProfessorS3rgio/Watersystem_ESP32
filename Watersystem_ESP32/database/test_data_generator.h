#ifndef TEST_DATA_GENERATOR_H
#define TEST_DATA_GENERATOR_H

#include "../configuration/config.h"
#include "customers_database.h"
#include "readings_database.h"
#include "bill_database.h"
#include <Arduino.h>

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

  int successCount = 0;

  for (int i = 0; i < count; i++) {
    // Select random customer account
    String account = getRandomAccountNo();
    if (account == "") continue;

    // Get previous reading
    unsigned long prev = getPreviousReadingForAccount(account);

    // Generate random current reading (previous + 1 to 100 m3)
    unsigned long curr = prev + random(1, 101); // 1-100 m3 usage

    // Generate bill (this also handles creating/updating the reading)
    bool billGenerated = generateBillForCustomer(account, curr);
    if (billGenerated) {
      successCount++;
      Serial.print(F("Heap after insert "));
      Serial.print(i + 1);
      Serial.print(F(": "));
      Serial.println(ESP.getFreeHeap());
    }

    // Progress indicator
    if ((i + 1) % 100 == 0) {
      Serial.print(F("Generated "));
      Serial.print(i + 1);
      Serial.println(F("..."));
    }

    // Yield to prevent watchdog reset
    YIELD_WDT();
  }

  Serial.print(F("Successfully generated "));
  Serial.print(successCount);
  Serial.println(F(" test readings and bills."));
}

#endif  // TEST_DATA_GENERATOR_H