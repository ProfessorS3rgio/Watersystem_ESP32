#ifndef DEDUCTION_DATABASE_H
#define DEDUCTION_DATABASE_H

#include <SD.h>
#include "../configuration/config.h"
#include "sync_utils.h"
#include <sqlite3.h>
#include <vector>
#include "database_manager.h"

// ===== DEDUCTION DATA STRUCTURE =====
struct Deduction {
  int deduction_id;
  String name;
  String type;  // "percentage" or "fixed"
  float value;
  String created_at;
  String updated_at;
};

// ===== DEDUCTION DATABASE =====
std::vector<Deduction> deductions;

static int loadDeductionCallback(void *data, int argc, char **argv, char **azColName) {
  Deduction d;
  d.deduction_id = atoi(argv[0]);
  d.name = argv[1];
  d.type = argv[2];
  d.value = atof(argv[3]);
  d.created_at = argv[4];
  d.updated_at = argv[5];
  deductions.push_back(d);
  return 0;
}

void loadDeductionsFromDB() {
  deductions.clear();
  const char *sql = "SELECT deduction_id, name, type, value, created_at, updated_at FROM deductions;";
  sqlite3_exec(db, sql, loadDeductionCallback, NULL, NULL);
}

void initDeductionsDatabase() {
  loadDeductionsFromDB();
}

// ===== UPSERT DEDUCTION FROM SYNC =====
bool upsertDeductionFromSync(unsigned long deductionId, String name, String type, float value, unsigned long createdAt, unsigned long updatedAt) {
  char sql[512];
  sprintf(sql, "INSERT OR REPLACE INTO deductions (deduction_id, name, type, value, created_at, updated_at) VALUES (%lu, '%s', '%s', %f, '%lu', '%lu');",
          deductionId, name.c_str(), type.c_str(), value, createdAt, updatedAt);
  int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
  if (rc == SQLITE_OK) {
    loadDeductionsFromDB(); // Reload vector
    return true;
  }
  return false;
}

// ===== FIND DEDUCTION BY ID =====
int findDeductionById(unsigned long deductionId) {
  for (size_t i = 0; i < deductions.size(); i++) {
    if (deductions[i].deduction_id == (int)deductionId) {
      return i;
    }
  }
  return -1;
}

// ===== GET DEDUCTION AT INDEX =====
Deduction* getDeductionAt(int index) {
  if (index >= 0 && index < (int)deductions.size()) {
    return &deductions[index];
  }
  return nullptr;
}

// ===== GET DEDUCTION COUNT =====
int getDeductionCount() {
  return deductions.size();
}

#endif  // DEDUCTION_DATABASE_H