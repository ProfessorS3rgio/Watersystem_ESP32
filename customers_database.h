#ifndef CUSTOMERS_DATABASE_H
#define CUSTOMERS_DATABASE_H

#include "config.h"

// Forward declarations
void printCustomersList();

// ===== CUSTOMER DATA STRUCTURE =====
struct Customer {
  String accountNumber;
  String customerName;
  String address;
  unsigned long previousReading;
};

// ===== CUSTOMER DATABASE =====
const int MAX_CUSTOMERS = 50;  // Maximum customers to store
Customer customers[MAX_CUSTOMERS];
int customerCount = 0;

// ===== INITIALIZE DATABASE WITH SAMPLE DATA =====
void initCustomersDatabase() {
  Serial.println(F("Initializing Customers Database..."));
  
  // Customer 1
  customers[0].accountNumber = "001";
  customers[0].customerName = "Theodore Romero";
  customers[0].address = "Purok 2 Makilas";
  customers[0].previousReading = 1250;
  
  // Customer 2
  customers[1].accountNumber = "002";
  customers[1].customerName = "Billy Mamaril";
  customers[1].address = "Purok 2 Makilas";
  customers[1].previousReading = 1220;
  
  // Customer 3
  customers[2].accountNumber = "003";
  customers[2].customerName = "John Doe";
  customers[2].address = "Purok 2 Makilas";
  customers[2].previousReading = 1210;
  
  customerCount = 3;
  
  Serial.print(F("Loaded "));
  Serial.print(customerCount);
  Serial.println(F(" customers from database."));
  printCustomersList();
}

// ===== FIND CUSTOMER BY ACCOUNT NUMBER =====
int findCustomerByAccount(String accountNumber) {
  for (int i = 0; i < customerCount; i++) {
    if (customers[i].accountNumber == accountNumber) {
      return i;  // Return index
    }
  }
  return -1;  // Not found
}

// ===== GET CUSTOMER AT INDEX =====
Customer* getCustomerAt(int index) {
  if (index >= 0 && index < customerCount) {
    return &customers[index];
  }
  return nullptr;
}

// ===== ADD NEW CUSTOMER =====
bool addCustomer(String accountNumber, String customerName, String address, unsigned long previousReading) {
  if (customerCount >= MAX_CUSTOMERS) {
    Serial.println(F("Database is full!"));
    return false;
  }
  
  // Check if account already exists
  if (findCustomerByAccount(accountNumber) != -1) {
    Serial.println(F("Account already exists!"));
    return false;
  }
  
  customers[customerCount].accountNumber = accountNumber;
  customers[customerCount].customerName = customerName;
  customers[customerCount].address = address;
  customers[customerCount].previousReading = previousReading;
  
  customerCount++;
  
  Serial.print(F("Added customer: "));
  Serial.println(customerName);
  return true;
}

// ===== UPDATE CUSTOMER READING =====
bool updateCustomerReading(String accountNumber, unsigned long newReading) {
  int index = findCustomerByAccount(accountNumber);
  if (index == -1) {
    Serial.println(F("Customer not found!"));
    return false;
  }
  
  customers[index].previousReading = newReading;
  Serial.print(F("Updated reading for "));
  Serial.print(customers[index].customerName);
  Serial.print(F(" to "));
  Serial.println(newReading);
  return true;
}

// ===== PRINT ALL CUSTOMERS =====
void printCustomersList() {
  Serial.println(F("\n===== CUSTOMERS DATABASE ====="));
  Serial.println(F("Account | Name              | Address         | Prev Read"));
  Serial.println(F("--------|-------------------|-----------------|----------"));
  
  for (int i = 0; i < customerCount; i++) {
    Serial.print(customers[i].accountNumber);
    Serial.print(F("       | "));
    Serial.print(customers[i].customerName);
    Serial.print(F("      | "));
    Serial.print(customers[i].address);
    Serial.print(F(" | "));
    Serial.println(customers[i].previousReading);
  }
  
  Serial.println(F("===============================\n"));
}

// ===== GET CUSTOMER COUNT =====
int getCustomerCount() {
  return customerCount;
}

#endif  // CUSTOMERS_DATABASE_H
