#ifndef DEDUCTION_DATABASE_H
#define DEDUCTION_DATABASE_H

#include <SD.h>
#include "../configuration/config.h"
#include "sync_utils.h"

// Forward declarations
void printDeductionsList();

// ===== DEDUCTION DATA STRUCTURE =====
struct Deduction {
  unsigned long deduction_id;
  String name;
  String type;  // "percentage" or "fixed"
  float value;
  unsigned long created_at;
  unsigned long updated_at;
};

// ===== DEDUCTION DATABASE =====
const int MAX_DEDUCTIONS = 10;  // Maximum deductions to store
Deduction deductions[MAX_DEDUCTIONS];
int deductionCount = 0;

static const char* DEDUCTIONS_SYNC_FILE = "/WATER_DB/DEDUCTIONS/deductions.psv"; // pipe-separated values

static bool loadDeductionsFromSD() {
  if (!SD.exists(DEDUCTIONS_SYNC_FILE)) {
    return false;
  }

  File f = SD.open(DEDUCTIONS_SYNC_FILE, FILE_READ);
  if (!f) {
    return false;
  }

  deductionCount = 0;
  while (f.available() && deductionCount < MAX_DEDUCTIONS) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    if (line.startsWith("#")) continue;

    int p1 = line.indexOf('|');
    int p2 = (p1 >= 0) ? line.indexOf('|', p1 + 1) : -1;
    int p3 = (p2 >= 0) ? line.indexOf('|', p2 + 1) : -1;
    int p4 = (p3 >= 0) ? line.indexOf('|', p3 + 1) : -1;
    int p5 = (p4 >= 0) ? line.indexOf('|', p4 + 1) : -1;
    if (p1 < 0 || p2 < 0 || p3 < 0 || p4 < 0 || p5 < 0) {
      continue;
    }

    String idStr = line.substring(0, p1);
    String name = line.substring(p1 + 1, p2);
    String type = line.substring(p2 + 1, p3);
    String valueStr = line.substring(p3 + 1, p4);
    String createdStr = line.substring(p4 + 1, p5);
    String updatedStr = line.substring(p5 + 1);

    idStr.trim();
    name.trim();
    type.trim();
    valueStr.trim();
    createdStr.trim();
    updatedStr.trim();

    deductions[deductionCount].deduction_id = (unsigned long)idStr.toInt();
    deductions[deductionCount].name = name;
    deductions[deductionCount].type = type;
    deductions[deductionCount].value = valueStr.toFloat();
    deductions[deductionCount].created_at = (unsigned long)createdStr.toInt();
    deductions[deductionCount].updated_at = (unsigned long)updatedStr.toInt();
    deductionCount++;
  }

  f.close();
  return (deductionCount > 0);
}

static bool saveDeductionsToSD() {
  // Best-effort: truncate by removing first
  if (SD.exists(DEDUCTIONS_SYNC_FILE)) {
    SD.remove(DEDUCTIONS_SYNC_FILE);
  }

  File f = SD.open(DEDUCTIONS_SYNC_FILE, FILE_WRITE);
  if (!f) {
    return false;
  }

  f.println(F("# deduction_id|name|type|value|created_at|updated_at"));
  for (int i = 0; i < deductionCount; i++) {
    f.print(deductions[i].deduction_id);
    f.print('|');
    f.print(sanitizeSyncField(deductions[i].name));
    f.print('|');
    f.print(sanitizeSyncField(deductions[i].type));
    f.print('|');
    f.print(deductions[i].value, 2);
    f.print('|');
    f.print(deductions[i].created_at);
    f.print('|');
    f.println(deductions[i].updated_at);
  }

  f.close();
  return true;
}

// ===== INITIALIZE DATABASE =====
bool addDeduction(String name, String type, float value);
void initDeductionsDatabase() {
#if WS_SERIAL_VERBOSE
  Serial.println(F("Initializing Deductions Database..."));
#endif

  // Prefer SD-backed deduction file
  bool loadedFromSd = false;
  if (SD.cardType() != CARD_NONE) {
    loadedFromSd = loadDeductionsFromSD();
  }

  if (loadedFromSd) {
#if WS_SERIAL_VERBOSE
    Serial.print(F("Loaded "));
    Serial.print(deductionCount);
    Serial.println(F(" deductions from SD card."));
#endif
    return;
  }

  // Start with default deductions if empty
  if (deductionCount == 0) {
    addDeduction("Senior Citizen", "percentage", 20.00);
    addDeduction("SSS", "fixed", 50.00);
#if WS_SERIAL_VERBOSE
    Serial.println(F("Added default deductions."));
#endif
  }

#if WS_SERIAL_VERBOSE
  Serial.println(F("Initialized deduction database."));
#endif
}

// ===== FIND DEDUCTION BY ID =====
int findDeductionById(unsigned long deductionId) {
  for (int i = 0; i < deductionCount; i++) {
    if (deductions[i].deduction_id == deductionId) {
      return i;  // Return index
    }
  }
  return -1;  // Not found
}

// ===== FIND DEDUCTION BY NAME =====
int findDeductionByName(String deductionName) {
  for (int i = 0; i < deductionCount; i++) {
    if (deductions[i].name == deductionName) {
      return i;  // Return index
    }
  }
  return -1;  // Not found
}

// ===== GET DEDUCTION AT INDEX =====
Deduction* getDeductionAt(int index) {
  if (index >= 0 && index < deductionCount) {
    return &deductions[index];
  }
  return nullptr;
}

// ===== ADD NEW DEDUCTION =====
bool addDeduction(String name, String type, float value) {
  if (deductionCount >= MAX_DEDUCTIONS) {
    Serial.println(F("Database is full!"));
    return false;
  }

  // Check if name already exists
  if (findDeductionByName(name) != -1) {
    Serial.println(F("Deduction name already exists!"));
    return false;
  }

  deductions[deductionCount].deduction_id = (unsigned long)(deductionCount + 1);
  deductions[deductionCount].name = name;
  deductions[deductionCount].type = type;
  deductions[deductionCount].value = value;
  deductions[deductionCount].created_at = 0;  // Will be set during sync
  deductions[deductionCount].updated_at = 0;

  deductionCount++;

  if (SD.cardType() != CARD_NONE) {
    (void)saveDeductionsToSD();
  }

  Serial.print(F("Added deduction: "));
  Serial.println(name);
  return true;
}

// ===== UPDATE DEDUCTION =====
bool updateDeduction(String name, String type, float value) {
  int index = findDeductionByName(name);
  if (index == -1) {
    Serial.println(F("Deduction not found!"));
    return false;
  }

  deductions[index].type = type;
  deductions[index].value = value;
  deductions[index].updated_at = 0;  // Will be updated during sync

  if (SD.cardType() != CARD_NONE) {
    (void)saveDeductionsToSD();
  }

  Serial.print(F("Updated deduction: "));
  Serial.println(name);
  return true;
}

// ===== SYNC PROTOCOL (Web Serial) =====
// UPSERT_DEDUCTION|deduction_id|name|type|value|created_at|updated_at
//   -> prints ACK|UPSERT|<name> or ERR|<message>

bool upsertDeductionFromSync(unsigned long deductionId, const String& name, const String& type, float value, unsigned long createdAt, unsigned long updatedAt) {
  String n = name;
  n.trim();
  if (n.length() == 0) return false;

  int idx = findDeductionByName(n);
  if (idx == -1) {
    if (deductionCount >= MAX_DEDUCTIONS) {
      return false;
    }
    idx = deductionCount;
    deductions[idx].deduction_id = deductionId;
    deductionCount++;
  }

  deductions[idx].name = n;
  deductions[idx].type = type;
  deductions[idx].value = value;
  deductions[idx].created_at = createdAt;
  deductions[idx].updated_at = updatedAt;

  if (SD.cardType() != CARD_NONE) {
    (void)saveDeductionsToSD();
  }
  return true;
}

// ===== REMOVE DEDUCTION BY NAME =====
bool removeDeductionByName(const String& name) {
  String n = name;
  n.trim();
  if (n.length() == 0) return false;

  int idx = findDeductionByName(n);
  if (idx == -1) return false;  // Not found

  // Shift all deductions after this one down by one
  for (int i = idx; i < deductionCount - 1; i++) {
    deductions[i] = deductions[i + 1];
  }
  deductionCount--;

  if (SD.cardType() != CARD_NONE) {
    (void)saveDeductionsToSD();
  }
  return true;
}

// ===== CLEAR ALL DEDUCTIONS ON SD =====
void clearDeductionsOnSD() {
  deductionCount = 0;
  if (SD.cardType() != CARD_NONE) {
    SD.remove(DEDUCTIONS_SYNC_FILE);
  }
}

// ===== PRINT ALL DEDUCTIONS =====
void printDeductionsList() {
  Serial.println(F("\n===== DEDUCTIONS DATABASE ====="));
  Serial.println(F("ID | Name          | Type      | Value"));
  Serial.println(F("---|---------------|-----------|------"));
  
  for (int i = 0; i < deductionCount; i++) {
    Serial.print(deductions[i].deduction_id);
    Serial.print(F("  | "));
    Serial.print(deductions[i].name);
    Serial.print(F(" | "));
    Serial.print(deductions[i].type);
    Serial.print(F(" | "));
    Serial.println(deductions[i].value, 2);
  }
  
  Serial.println(F("===============================\n"));
}

// ===== GET DEDUCTION COUNT =====
int getDeductionCount() {
  return deductionCount;
}

#endif  // DEDUCTION_DATABASE_H