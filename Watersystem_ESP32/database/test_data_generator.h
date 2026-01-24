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

  if (allCustomers.empty()) {
    Serial.println(F("Loading all customers for test..."));
    loadAllCustomers();
  }

  int customerCount = allCustomers.size();
  if (customerCount == 0) {
    Serial.println(F("No customers found!"));
    return;
  }

  int successCount = 0;

  for (int i = 0; i < count; i++) {
    // Select random customer
    int index = random(0, customerCount);
    Customer* c = &allCustomers[index];

    // Generate random current reading (previous + 1 to 100 m3)
    unsigned long prev = c->previous_reading;
    unsigned long curr = prev + random(1, 101); // 1-100 m3 usage

    // Generate bill (this also handles creating/updating the reading)
    bool billGenerated = generateBillForCustomer(c->account_no, curr);
    if (billGenerated) {
      successCount++;
    }

    // Progress indicator
    if ((i + 1) % 100 == 0) {
      Serial.print(F("Generated "));
      Serial.print(i + 1);
      Serial.println(F("..."));
    }
  }

  Serial.print(F("Successfully generated "));
  Serial.print(successCount);
  Serial.println(F(" test readings and bills."));
}

#endif  // TEST_DATA_GENERATOR_H