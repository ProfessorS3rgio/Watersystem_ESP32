#ifndef CUSTOMER_TYPE_DATABASE_H
#define CUSTOMER_TYPE_DATABASE_H

#include <SD.h>
#include "../configuration/config.h"
#include "sync_utils.h"

// Forward declarations
void printCustomerTypesList();

// ===== CUSTOMER TYPE DATA STRUCTURE =====
struct CustomerType {
  unsigned long type_id;
  String type_name;
  float rate_per_m3;
  unsigned long min_m3;
  float min_charge;
  unsigned long created_at;
  unsigned long updated_at;
};

// ===== CUSTOMER TYPE DATABASE =====
const int MAX_CUSTOMER_TYPES = 10;  // Maximum customer types to store
CustomerType customerTypes[MAX_CUSTOMER_TYPES];
int customerTypeCount = 0;

static const char* CUSTOMER_TYPES_SYNC_FILE = "/WATER_DB/CUSTOMER_TYPES/customer_types.psv"; // pipe-separated values

static bool loadCustomerTypesFromSD() {
  if (!SD.exists(CUSTOMER_TYPES_SYNC_FILE)) {
    return false;
  }

  File f = SD.open(CUSTOMER_TYPES_SYNC_FILE, FILE_READ);
  if (!f) {
    return false;
  }

  customerTypeCount = 0;
  while (f.available() && customerTypeCount < MAX_CUSTOMER_TYPES) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    if (line.startsWith("#")) continue;

    int p1 = line.indexOf('|');
    int p2 = (p1 >= 0) ? line.indexOf('|', p1 + 1) : -1;
    int p3 = (p2 >= 0) ? line.indexOf('|', p2 + 1) : -1;
    int p4 = (p3 >= 0) ? line.indexOf('|', p3 + 1) : -1;
    int p5 = (p4 >= 0) ? line.indexOf('|', p4 + 1) : -1;
    int p6 = (p5 >= 0) ? line.indexOf('|', p5 + 1) : -1;
    if (p1 < 0 || p2 < 0 || p3 < 0 || p4 < 0 || p5 < 0 || p6 < 0) {
      continue;
    }

    String idStr = line.substring(0, p1);
    String name = line.substring(p1 + 1, p2);
    String rateStr = line.substring(p2 + 1, p3);
    String minM3Str = line.substring(p3 + 1, p4);
    String minChargeStr = line.substring(p4 + 1, p5);
    String createdStr = line.substring(p5 + 1, p6);
    String updatedStr = line.substring(p6 + 1);

    idStr.trim();
    name.trim();
    rateStr.trim();
    minM3Str.trim();
    minChargeStr.trim();
    createdStr.trim();
    updatedStr.trim();

    customerTypes[customerTypeCount].type_id = (unsigned long)idStr.toInt();
    customerTypes[customerTypeCount].type_name = name;
    customerTypes[customerTypeCount].rate_per_m3 = rateStr.toFloat();
    customerTypes[customerTypeCount].min_m3 = (unsigned long)minM3Str.toInt();
    customerTypes[customerTypeCount].min_charge = minChargeStr.toFloat();
    customerTypes[customerTypeCount].created_at = (unsigned long)createdStr.toInt();
    customerTypes[customerTypeCount].updated_at = (unsigned long)updatedStr.toInt();
    customerTypeCount++;
  }

  f.close();
  return (customerTypeCount > 0);
}

static bool saveCustomerTypesToSD() {
  // Best-effort: truncate by removing first
  if (SD.exists(CUSTOMER_TYPES_SYNC_FILE)) {
    SD.remove(CUSTOMER_TYPES_SYNC_FILE);
  }

  File f = SD.open(CUSTOMER_TYPES_SYNC_FILE, FILE_WRITE);
  if (!f) {
    return false;
  }

  f.println(F("# type_id|type_name|rate_per_m3|min_m3|min_charge|created_at|updated_at"));
  for (int i = 0; i < customerTypeCount; i++) {
    f.print(customerTypes[i].type_id);
    f.print('|');
    f.print(sanitizeSyncField(customerTypes[i].type_name));
    f.print('|');
    f.print(customerTypes[i].rate_per_m3, 2);
    f.print('|');
    f.print(customerTypes[i].min_m3);
    f.print('|');
    f.print(customerTypes[i].min_charge, 2);
    f.print('|');
    f.print(customerTypes[i].created_at);
    f.print('|');
    f.println(customerTypes[i].updated_at);
  }

  f.close();
  return true;
}

// ===== INITIALIZE DATABASE =====
void initCustomerTypesDatabase() {
#if WS_SERIAL_VERBOSE
  Serial.println(F("Initializing Customer Types Database..."));
#endif

  // Prefer SD-backed customer types file
  bool loadedFromSd = false;
  if (SD.cardType() != CARD_NONE) {
    loadedFromSd = loadCustomerTypesFromSD();
  }

  if (loadedFromSd) {
#if WS_SERIAL_VERBOSE
    Serial.print(F("Loaded "));
    Serial.print(customerTypeCount);
    Serial.println(F(" customer types from SD card."));
#endif
    return;
  }

  // Start with empty database - customer types will be synced from web
  customerTypeCount = 0;

#if WS_SERIAL_VERBOSE
  Serial.println(F("Initialized empty customer types database."));
#endif
}

// ===== FIND CUSTOMER TYPE BY ID =====
int findCustomerTypeById(unsigned long typeId) {
  for (int i = 0; i < customerTypeCount; i++) {
    if (customerTypes[i].type_id == typeId) {
      return i;  // Return index
    }
  }
  return -1;  // Not found
}

// ===== FIND CUSTOMER TYPE BY NAME =====
int findCustomerTypeByName(String typeName) {
  for (int i = 0; i < customerTypeCount; i++) {
    if (customerTypes[i].type_name == typeName) {
      return i;  // Return index
    }
  }
  return -1;  // Not found
}

// ===== GET CUSTOMER TYPE AT INDEX =====
CustomerType* getCustomerTypeAt(int index) {
  if (index >= 0 && index < customerTypeCount) {
    return &customerTypes[index];
  }
  return nullptr;
}

// ===== ADD NEW CUSTOMER TYPE =====
bool addCustomerType(String typeName, float ratePerM3, unsigned long minM3 = 0, float minCharge = 0.00) {
  if (customerTypeCount >= MAX_CUSTOMER_TYPES) {
    Serial.println(F("Database is full!"));
    return false;
  }

  // Check if name already exists
  if (findCustomerTypeByName(typeName) != -1) {
    Serial.println(F("Customer type name already exists!"));
    return false;
  }

  customerTypes[customerTypeCount].type_id = (unsigned long)(customerTypeCount + 1);
  customerTypes[customerTypeCount].type_name = typeName;
  customerTypes[customerTypeCount].rate_per_m3 = ratePerM3;
  customerTypes[customerTypeCount].min_m3 = minM3;
  customerTypes[customerTypeCount].min_charge = minCharge;
  customerTypes[customerTypeCount].created_at = 0;  // Will be set during sync
  customerTypes[customerTypeCount].updated_at = 0;

  customerTypeCount++;

  if (SD.cardType() != CARD_NONE) {
    (void)saveCustomerTypesToSD();
  }

  Serial.print(F("Added customer type: "));
  Serial.println(typeName);
  return true;
}

// ===== UPDATE CUSTOMER TYPE =====
bool updateCustomerType(String typeName, float ratePerM3, unsigned long minM3, float minCharge) {
  int index = findCustomerTypeByName(typeName);
  if (index == -1) {
    Serial.println(F("Customer type not found!"));
    return false;
  }

  customerTypes[index].rate_per_m3 = ratePerM3;
  customerTypes[index].min_m3 = minM3;
  customerTypes[index].min_charge = minCharge;
  customerTypes[index].updated_at = 0;  // Will be updated during sync

  if (SD.cardType() != CARD_NONE) {
    (void)saveCustomerTypesToSD();
  }

  Serial.print(F("Updated customer type: "));
  Serial.println(typeName);
  return true;
}

// ===== SYNC PROTOCOL (Web Serial) =====
// EXPORT_CUSTOMER_TYPES
//   -> prints BEGIN_CUSTOMER_TYPES, then TYPE|type_id|type_name|rate_per_m3|min_m3|min_charge|created_at|updated_at lines, then END_CUSTOMER_TYPES
// UPSERT_CUSTOMER_TYPE|type_id|type_name|rate_per_m3|min_m3|min_charge|created_at|updated_at
//   -> prints ACK|UPSERT|<type_name> or ERR|<message>

void exportCustomerTypesForSync() {
  uint32_t startMs = millis();
  Serial.println(F("BEGIN_CUSTOMER_TYPES"));
  for (int i = 0; i < customerTypeCount; i++) {
    Serial.print(F("TYPE|"));
    Serial.print(customerTypes[i].type_id);
    Serial.print('|');
    Serial.print(sanitizeSyncField(customerTypes[i].type_name));
    Serial.print('|');
    Serial.print(customerTypes[i].rate_per_m3, 2);
    Serial.print('|');
    Serial.print(customerTypes[i].min_m3);
    Serial.print('|');
    Serial.print(customerTypes[i].min_charge, 2);
    Serial.print('|');
    Serial.print(customerTypes[i].created_at);
    Serial.print('|');
    Serial.println(customerTypes[i].updated_at);
  }
  Serial.println(F("END_CUSTOMER_TYPES"));

  uint32_t elapsedMs = millis() - startMs;
  Serial.print(F("Done. ("));
  Serial.print(elapsedMs);
  Serial.println(F(" ms)"));
}

bool upsertCustomerTypeFromSync(unsigned long typeId, const String& typeName, float ratePerM3, unsigned long minM3, float minCharge, unsigned long createdAt, unsigned long updatedAt) {
  String n = typeName;
  n.trim();
  if (n.length() == 0) return false;

  int idx = findCustomerTypeByName(n);
  if (idx == -1) {
    if (customerTypeCount >= MAX_CUSTOMER_TYPES) {
      return false;
    }
    idx = customerTypeCount;
    customerTypes[idx].type_id = typeId;
    customerTypeCount++;
  }

  customerTypes[idx].type_name = n;
  customerTypes[idx].rate_per_m3 = ratePerM3;
  customerTypes[idx].min_m3 = minM3;
  customerTypes[idx].min_charge = minCharge;
  customerTypes[idx].created_at = createdAt;
  customerTypes[idx].updated_at = updatedAt;

  if (SD.cardType() != CARD_NONE) {
    (void)saveCustomerTypesToSD();
  }
  return true;
}

// ===== REMOVE CUSTOMER TYPE BY NAME =====
bool removeCustomerTypeByName(const String& typeName) {
  String n = typeName;
  n.trim();
  if (n.length() == 0) return false;

  int idx = findCustomerTypeByName(n);
  if (idx == -1) return false;  // Not found

  // Shift all customer types after this one down by one
  for (int i = idx; i < customerTypeCount - 1; i++) {
    customerTypes[i] = customerTypes[i + 1];
  }
  customerTypeCount--;

  if (SD.cardType() != CARD_NONE) {
    (void)saveCustomerTypesToSD();
  }
  return true;
}

// ===== CLEAR ALL CUSTOMER TYPES ON SD =====
void clearCustomerTypesOnSD() {
  customerTypeCount = 0;
  if (SD.cardType() != CARD_NONE) {
    SD.remove(CUSTOMER_TYPES_SYNC_FILE);
  }
}

// ===== PRINT ALL CUSTOMER TYPES =====
void printCustomerTypesList() {
  Serial.println(F("\n===== CUSTOMER TYPES DATABASE ====="));
  Serial.println(F("ID | Name          | Rate/m3 | Min m3 | Min Charge"));
  Serial.println(F("---|---------------|---------|--------|-----------"));
  
  for (int i = 0; i < customerTypeCount; i++) {
    Serial.print(customerTypes[i].type_id);
    Serial.print(F("  | "));
    Serial.print(customerTypes[i].type_name);
    Serial.print(F(" | "));
    Serial.print(customerTypes[i].rate_per_m3, 2);
    Serial.print(F(" | "));
    Serial.print(customerTypes[i].min_m3);
    Serial.print(F(" | "));
    Serial.println(customerTypes[i].min_charge, 2);
  }
  
  Serial.println(F("====================================\n"));
}

// ===== GET CUSTOMER TYPE COUNT =====
int getCustomerTypeCount() {
  return customerTypeCount;
}

#endif  // CUSTOMER_TYPE_DATABASE_H