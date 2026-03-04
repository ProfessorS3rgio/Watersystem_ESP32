#ifndef BARANGAY_DATABASE_H
#define BARANGAY_DATABASE_H

#include <SD.h>
#include "../configuration/config.h"
#include "sync_utils.h"
#include <sqlite3.h>
#include <vector>
#include "database_manager.h"

// ===== BARANGAY DATA STRUCTURE =====
struct Barangay {
  int brgy_id;
  String barangay;
  String prefix;
  int next_number;
  String updated_at;
};

// ===== BARANGAY DATABASE =====
std::vector<Barangay> barangays;

static int loadBarangayCallback(void *data, int argc, char **argv, char **azColName) {
  Barangay b;
  b.brgy_id = atoi(argv[0]);
  b.barangay = argv[1];
  b.prefix = argv[2];
  b.next_number = atoi(argv[3]);
  b.updated_at = argv[4];
  barangays.push_back(b);
  return 0;
}

void loadBarangaysFromDB() {
  barangays.clear();
  const char *sql = "SELECT brgy_id, barangay, prefix, next_number, updated_at FROM barangay_sequence;";
  sqlite3_exec(db, sql, loadBarangayCallback, NULL, NULL);
}

void initBarangaysDatabase() {
  loadBarangaysFromDB();
}

// ===== UPSERT BARANGAY FROM SYNC =====
bool upsertBarangayFromSync(unsigned long brgyId, String barangay, String prefix, unsigned long nextNumber, unsigned long createdAt, unsigned long updatedAt) {
  char sql[512];
  sprintf(sql, "INSERT OR REPLACE INTO barangay_sequence (brgy_id, barangay, prefix, next_number, updated_at) VALUES (%lu, '%s', '%s', %lu, '%lu');",
          brgyId, barangay.c_str(), prefix.c_str(), nextNumber, updatedAt);

  int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
  if (rc == SQLITE_OK) {
    loadBarangaysFromDB(); // Reload vector
    return true;
  }
  return false;
}

// ===== FIND BARANGAY BY ID =====
int findBarangayById(unsigned long brgyId) {
  for (size_t i = 0; i < barangays.size(); i++) {
    if (barangays[i].brgy_id == (int)brgyId) {
      return i;
    }
  }
  return -1;
}

// ===== GET BARANGAY AT INDEX =====
Barangay* getBarangayAt(int index) {
  if (index >= 0 && index < (int)barangays.size()) {
    return &barangays[index];
  }
  return nullptr;
}

// ===== GET BARANGAY COUNT =====
int getBarangayCount() {
  return barangays.size();
}

// ===== GET BARANGAY PREFIX FOR CURRENT DEVICE =====
String getCurrentBarangayPrefix() {
  for (const auto& b : barangays) {
    if (b.brgy_id == BRGY_ID_VALUE) {
      return b.prefix;
    }
  }
  return ""; // Not found
}

#endif  // BARANGAY_DATABASE_H