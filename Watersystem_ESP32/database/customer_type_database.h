#ifndef CUSTOMER_TYPE_DATABASE_H
#define CUSTOMER_TYPE_DATABASE_H

#include <SD.h>
#include "../configuration/config.h"
#include "sync_utils.h"
#include <sqlite3.h>
#include <vector>
#include "database_manager.h"

// ===== CUSTOMER TYPE DATA STRUCTURE =====
struct CustomerType {
  int type_id;
  String type_name;
  float rate_per_m3;
  unsigned long min_m3;
  float min_charge;
  float penalty;
  String created_at;
  String updated_at;
};

// ===== CUSTOMER TYPE DATABASE =====
std::vector<CustomerType> customerTypes;

static int loadCustomerTypeCallback(void *data, int argc, char **argv, char **azColName) {
  CustomerType ct;
  ct.type_id = atoi(argv[0]);
  ct.type_name = argv[1];
  ct.rate_per_m3 = atof(argv[2]);
  ct.min_m3 = strtoul(argv[3], NULL, 10);
  ct.min_charge = atof(argv[4]);
  ct.penalty = atof(argv[5]);
  ct.created_at = argv[6];
  ct.updated_at = argv[7];
  customerTypes.push_back(ct);
  return 0;
}

void loadCustomerTypesFromDB() {
  customerTypes.clear();
  const char *sql = "SELECT type_id, type_name, rate_per_m3, min_m3, min_charge, penalty, created_at, updated_at FROM customer_types;";
  sqlite3_exec(db, sql, loadCustomerTypeCallback, NULL, NULL);
}

void initCustomerTypesDatabase() {
  loadCustomerTypesFromDB();
}

// ===== UPSERT CUSTOMER TYPE FROM SYNC =====
bool upsertCustomerTypeFromSync(unsigned long typeId, String typeName, float ratePerM3, unsigned long minM3, float minCharge, float penalty, unsigned long createdAt, unsigned long updatedAt) {
  char sql[512];
  sprintf(sql, "INSERT OR REPLACE INTO customer_types (type_id, type_name, rate_per_m3, min_m3, min_charge, penalty, created_at, updated_at) VALUES (%lu, '%s', %f, %lu, %f, %f, '%lu', '%lu');",
          typeId, typeName.c_str(), ratePerM3, minM3, minCharge, penalty, createdAt, updatedAt);
  int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
  if (rc == SQLITE_OK) {
    loadCustomerTypesFromDB(); // Reload vector
    return true;
  }
  return false;
}

// ===== FIND CUSTOMER TYPE BY ID =====
int findCustomerTypeById(unsigned long typeId) {
  for (size_t i = 0; i < customerTypes.size(); i++) {
    if (customerTypes[i].type_id == (int)typeId) {
      return i;
    }
  }
  return -1;
}

// ===== GET CUSTOMER TYPE AT INDEX =====
CustomerType* getCustomerTypeAt(int index) {
  if (index >= 0 && index < (int)customerTypes.size()) {
    return &customerTypes[index];
  }
  return nullptr;
}

// ===== GET CUSTOMER TYPE COUNT =====
int getCustomerTypeCount() {
  return customerTypes.size();
}

#endif  // CUSTOMER_TYPE_DATABASE_H