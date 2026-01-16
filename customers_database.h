#ifndef CUSTOMERS_DATABASE_H
#define CUSTOMERS_DATABASE_H

#include <SD.h>
#include "config.h"

// Forward declarations
void printCustomersList();

// ===== CUSTOMER DATA STRUCTURE =====
struct Customer {
  // Match Laravel schema fields:
  // account_no, customer_name, address, previous_reading, is_active, timestamps
  unsigned long id;
  String account_no;
  String customer_name;
  String address;
  unsigned long previous_reading;
  bool is_active;
  unsigned long created_at;  // epoch seconds if available, else 0
  unsigned long updated_at;  // epoch seconds if available, else 0
};

// ===== CUSTOMER DATABASE =====
const int MAX_CUSTOMERS = 50;  // Maximum customers to store
Customer customers[MAX_CUSTOMERS];
int customerCount = 0;

static const char* CUSTOMERS_SYNC_FILE = "/WATER_DB/CUSTOMERS/customers.psv"; // pipe-separated values

static String sanitizeSyncField(const String& s) {
  String out = s;
  out.replace("\r", " ");
  out.replace("\n", " ");
  out.replace("|", " ");
  return out;
}

static bool parseBoolToken(const String& s) {
  String v = s;
  v.trim();
  v.toLowerCase();
  return (v == "1" || v == "true" || v == "y" || v == "yes");
}

static bool loadCustomersFromSD() {
  if (!SD.exists(CUSTOMERS_SYNC_FILE)) {
    return false;
  }

  File f = SD.open(CUSTOMERS_SYNC_FILE, FILE_READ);
  if (!f) {
    return false;
  }

  customerCount = 0;
  while (f.available() && customerCount < MAX_CUSTOMERS) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    if (line.startsWith("#")) continue;

    int p1 = line.indexOf('|');
    int p2 = (p1 >= 0) ? line.indexOf('|', p1 + 1) : -1;
    int p3 = (p2 >= 0) ? line.indexOf('|', p2 + 1) : -1;
    int p4 = (p3 >= 0) ? line.indexOf('|', p3 + 1) : -1;
    if (p1 < 0 || p2 < 0 || p3 < 0 || p4 < 0) {
      continue;
    }

    String accountNo = line.substring(0, p1);
    String name = line.substring(p1 + 1, p2);
    String address = line.substring(p2 + 1, p3);
    String prev = line.substring(p3 + 1, p4);
    String active = line.substring(p4 + 1);

    accountNo.trim();
    name.trim();
    address.trim();
    prev.trim();
    active.trim();
    if (accountNo.length() == 0) continue;

    customers[customerCount].id = (unsigned long)(customerCount + 1);
    customers[customerCount].account_no = accountNo;
    customers[customerCount].customer_name = name;
    customers[customerCount].address = address;
    customers[customerCount].previous_reading = (unsigned long)prev.toInt();
    customers[customerCount].is_active = parseBoolToken(active);
    customers[customerCount].created_at = 0;
    customers[customerCount].updated_at = 0;
    customerCount++;
  }

  f.close();
  return (customerCount > 0);
}

static bool saveCustomersToSD() {
  // Best-effort: truncate by removing first
  if (SD.exists(CUSTOMERS_SYNC_FILE)) {
    SD.remove(CUSTOMERS_SYNC_FILE);
  }

  File f = SD.open(CUSTOMERS_SYNC_FILE, FILE_WRITE);
  if (!f) {
    return false;
  }

  f.println(F("# account_no|customer_name|address|previous_reading|is_active"));
  for (int i = 0; i < customerCount; i++) {
    f.print(sanitizeSyncField(customers[i].account_no));
    f.print('|');
    f.print(sanitizeSyncField(customers[i].customer_name));
    f.print('|');
    f.print(sanitizeSyncField(customers[i].address));
    f.print('|');
    f.print(customers[i].previous_reading);
    f.print('|');
    f.println(customers[i].is_active ? '1' : '0');
  }

  f.close();
  return true;
}

// ===== INITIALIZE DATABASE =====
void initCustomersDatabase() {
#if WS_SERIAL_VERBOSE
  Serial.println(F("Initializing Customers Database..."));
#endif

  // Prefer SD-backed customer file (so the device really uses SD as its database).
  bool loadedFromSd = false;
  if (SD.cardType() != CARD_NONE) {
    loadedFromSd = loadCustomersFromSD();
  }

  if (loadedFromSd) {
#if WS_SERIAL_VERBOSE
    Serial.print(F("Loaded "));
    Serial.print(customerCount);
    Serial.println(F(" customers from SD card."));
#endif
    return;
  }
  
  // Start with empty database - customers will be synced from web
  customerCount = 0;
  
#if WS_SERIAL_VERBOSE
  Serial.println(F("Initialized empty customer database."));
#endif
}

// ===== FIND CUSTOMER BY ACCOUNT NUMBER =====
int findCustomerByAccount(String accountNumber) {
  for (int i = 0; i < customerCount; i++) {
    if (customers[i].account_no == accountNumber) {
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
  
  customers[customerCount].account_no = accountNumber;
  customers[customerCount].customer_name = customerName;
  customers[customerCount].address = address;
  customers[customerCount].previous_reading = previousReading;
  customers[customerCount].is_active = true;
  customers[customerCount].created_at = 0;
  customers[customerCount].updated_at = 0;
  customers[customerCount].id = (unsigned long)(customerCount + 1);
  
  customerCount++;

  if (SD.cardType() != CARD_NONE) {
    (void)saveCustomersToSD();
  }
  
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
  
  customers[index].previous_reading = newReading;

  if (SD.cardType() != CARD_NONE) {
    (void)saveCustomersToSD();
  }
  Serial.print(F("Updated reading for "));
  Serial.print(customers[index].customer_name);
  Serial.print(F(" to "));
  Serial.println(newReading);
  return true;
}

// ===== SYNC PROTOCOL (Web Serial) =====
// EXPORT_CUSTOMERS
//   -> prints BEGIN_CUSTOMERS, then CUST|account_no|customer_name|address|previous_reading|is_active lines, then END_CUSTOMERS
// UPSERT_CUSTOMER|account_no|customer_name|address|previous_reading|is_active
//   -> prints ACK|UPSERT|<account_no> or ERR|<message>

static void exportCustomersForSync() {
  uint32_t startMs = millis();
  Serial.println(F("BEGIN_CUSTOMERS"));
  for (int i = 0; i < customerCount; i++) {
    Serial.print(F("CUST|"));
    Serial.print(sanitizeSyncField(customers[i].account_no));
    Serial.print('|');
    Serial.print(sanitizeSyncField(customers[i].customer_name));
    Serial.print('|');
    Serial.print(sanitizeSyncField(customers[i].address));
    Serial.print('|');
    Serial.print(customers[i].previous_reading);
    Serial.print('|');
    Serial.println(customers[i].is_active ? '1' : '0');
  }
  Serial.println(F("END_CUSTOMERS"));

  uint32_t elapsedMs = millis() - startMs;
  Serial.print(F("Done. ("));
  Serial.print(elapsedMs);
  Serial.println(F(" ms)"));
  Serial.print(F("Total Customers exported: "));
  Serial.println(customerCount);
}

static bool upsertCustomerFromSync(const String& accountNo, const String& name, const String& address, unsigned long prev, bool active) {
  String acct = accountNo;
  acct.trim();
  if (acct.length() == 0) return false;

  int idx = findCustomerByAccount(acct);
  if (idx == -1) {
    if (customerCount >= MAX_CUSTOMERS) {
      return false;
    }
    idx = customerCount;
    customers[idx].id = (unsigned long)(idx + 1);
    customers[idx].account_no = acct;
    customerCount++;
  }

  customers[idx].customer_name = name;
  customers[idx].address = address;
  customers[idx].previous_reading = prev;
  customers[idx].is_active = active;

  if (SD.cardType() != CARD_NONE) {
    (void)saveCustomersToSD();
  }
  return true;
}

// ===== REMOVE CUSTOMER BY ACCOUNT =====
static bool removeCustomerByAccount(const String& accountNo) {
  String acct = accountNo;
  acct.trim();
  if (acct.length() == 0) return false;

  int idx = findCustomerByAccount(acct);
  if (idx == -1) return false;  // Not found

  // Shift all customers after this one down by one
  for (int i = idx; i < customerCount - 1; i++) {
    customers[i] = customers[i + 1];
  }
  customerCount--;

  if (SD.cardType() != CARD_NONE) {
    (void)saveCustomersToSD();
  }
  return true;
}

// ===== CLEAR ALL CUSTOMERS ON SD =====
void clearCustomersOnSD() {
  customerCount = 0;
  if (SD.cardType() != CARD_NONE) {
    SD.remove(CUSTOMERS_SYNC_FILE);
  }
}

// ===== PRINT ALL CUSTOMERS =====
void printCustomersList() {
  Serial.println(F("\n===== CUSTOMERS DATABASE ====="));
  Serial.println(F("Account | Name              | Address         | Prev Read | Active"));
  Serial.println(F("--------|-------------------|-----------------|----------|-------"));
  
  for (int i = 0; i < customerCount; i++) {
    Serial.print(customers[i].account_no);
    Serial.print(F("       | "));
    Serial.print(customers[i].customer_name);
    Serial.print(F("      | "));
    Serial.print(customers[i].address);
    Serial.print(F(" | "));
    Serial.print(customers[i].previous_reading);
    Serial.print(F(" | "));
    Serial.println(customers[i].is_active ? F("Y") : F("N"));
  }
  
  Serial.println(F("===============================\n"));
}

// ===== GET CUSTOMER COUNT =====
int getCustomerCount() {
  return customerCount;
}

#endif  // CUSTOMERS_DATABASE_H
