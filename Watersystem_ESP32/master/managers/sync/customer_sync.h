#ifndef CUSTOMER_SYNC_H
#define CUSTOMER_SYNC_H

#include "../../database/customers_database.h"
#include "../../configuration/config.h"
#include <ArduinoJson.h>
#include <vector>

// compile-time flag to reduce serial output during bulk syncs
#ifndef SYNC_DEBUG
#define SYNC_DEBUG 0
#endif

// Struct to hold customer data temporarily
struct CustomerData {
  String accountNo;
  String name;
  String address;
  unsigned long prevReading;
  String status;
  unsigned long typeId;
  unsigned long deductionId;
  unsigned long brgyId;
};

// Callback for counting customers
static int countCallback(void *data, int argc, char **argv, char **azColName) {
  int* count = (int*)data;
  *count = atoi(argv[0]);
  return 0;
}

// ===== CUSTOMER SYNC OPERATIONS =====

// Handle UPSERT_CUSTOMERS_JSON command
bool handleUpsertCustomersJson(String payload) {
  DynamicJsonDocument doc(65536); // 64KB for customer lists
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.println(F("ERR|JSON_PARSE_FAILED"));
    return true;
  }

  JsonArray customers = doc.as<JsonArray>();
  bool allSuccess = true;
  for (JsonObject customer : customers) {
    String accountNo = customer["account_no"] | "";
    String name = customer["customer_name"] | "";
    String address = customer["address"] | "";
    unsigned long prev = customer["previous_reading"] | 0;
    String status = customer["status"] | "active";
    unsigned long typeId = customer["type_id"] | 1;
    unsigned long deductionId = customer["deduction_id"] | 0;
    unsigned long brgyId = customer["brgy_id"] | 1;

    if (!upsertCustomerFromSync(accountNo, name, address, prev, status, typeId, deductionId, brgyId)) {
      allSuccess = false;
      break;
    }
  }

  if (allSuccess) {
    Serial.print(F("ACK|UPSERT_CUSTOMERS_JSON|"));
    Serial.println(customers.size());
  } else {
    Serial.println(F("ERR|UPSERT_CUSTOMERS_JSON_FAILED"));
  }
  return true;
}

// Handle UPSERT_CUSTOMERS_JSON_CHUNK command
bool handleUpsertCustomersJsonChunk(String payload) {
  int p1 = payload.indexOf('|');
  int p2 = (p1 >= 0) ? payload.indexOf('|', p1 + 1) : -1;
  if (p1 < 0 || p2 < 0) {
    Serial.println(F("ERR|BAD_CHUNK_FORMAT"));
    return true;
  }

  int chunkIndex = payload.substring(0, p1).toInt();
  int totalChunks = payload.substring(p1 + 1, p2).toInt();
  String jsonChunk = payload.substring(p2 + 1);
  jsonChunk.replace("\\|", "|");

  // Parse the JSON chunk (it's an array of customers)
  DynamicJsonDocument doc(16384); // 16KB should be enough for 10 customers
  Serial.println(F("About to deserialize JSON"));
  DeserializationError error = deserializeJson(doc, jsonChunk);
  if (error) {
    Serial.println(F("ERR|JSON_PARSE_FAILED"));
    Serial.print(F("Error: "));
    Serial.println(error.c_str());
    return true;
  }
  Serial.println(F("JSON deserialized successfully"));

  JsonArray customers = doc.as<JsonArray>();
  bool allSuccess = true;

  Serial.print(F("Processing chunk "));
  Serial.println(chunkIndex);

  // Set synchronous to NORMAL for speed during sync (durability is still good)
  sqlite3_exec(db, "PRAGMA synchronous = NORMAL;", NULL, NULL, NULL);

  Serial.printf("Heap free before chunk: %d\n", ESP.getFreeHeap());

  // Begin transaction for this chunk
  int rc_begin = sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
  if (rc_begin != SQLITE_OK) {
    Serial.print(F("BEGIN failed for chunk "));
    Serial.print(chunkIndex);
    Serial.print(F(": "));
    Serial.println(sqlite3_errmsg(db));
    Serial.println(F("ERR|UPSERT_FAILED"));
    return true;
  }

  for (JsonObject customer : customers) {
    String accountNo = customer["account_no"] | "";
    String name = customer["customer_name"] | "";
    String address = customer["address"] | "";
    unsigned long prev = customer["previous_reading"] | 0;
    String status = customer["status"] | "active";
    unsigned long typeId = customer["type_id"] | 1;
    unsigned long deductionId = customer["deduction_id"] | 0;
    unsigned long brgyId = customer["brgy_id"] | 1;

    if (!upsertCustomerFromSync(accountNo, name, address, prev, status, typeId, deductionId, brgyId)) {
      allSuccess = false;
      break;
    }
  }

  if (allSuccess) {
    int rc_commit = sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
    if (rc_commit != SQLITE_OK) {
      Serial.print(F("COMMIT failed for chunk "));
      Serial.print(chunkIndex);
      Serial.print(F(": "));
      Serial.println(sqlite3_errmsg(db));
      Serial.println(F("ERR|UPSERT_FAILED"));
      return true;
    } else {
      Serial.print(F("Processed chunk "));
      Serial.println(chunkIndex);
    }
    Serial.print(F("ACK|CHUNK|"));
    Serial.println(chunkIndex);
    Serial.printf("Heap free after chunk: %d\n", ESP.getFreeHeap());
    Serial.println(F("Ready for next chunk"));
    Serial.flush();
    if (chunkIndex == totalChunks - 1) {
      // Last chunk, count customers in DB
      int customerCount = 0;
      sqlite3_exec(db, "SELECT COUNT(*) FROM customers;", countCallback, &customerCount, NULL);
      Serial.print(F("Total customers in DB after sync: "));
      Serial.println(customerCount);
      // Send final success
      Serial.print(F("ACK|UPSERT_CUSTOMERS_JSON|"));
      Serial.println(customers.size());
      Serial.flush();
    }
  } else {
    sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
    Serial.println(F("ERR|UPSERT_FAILED"));
  }
  return true;
}

// simple base64 decoder used for syncing customer chunks
static String base64_decode(const String& in) {
  static const char* b64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  int len = in.length();
  String out;
  out.reserve((len * 3) / 4);
  int val = 0, valb = -8;

  for (int i = 0; i < len; i++) {
    char c = in[i];
    if (c == '\r' || c == '\n' || isspace(c)) continue;
    if (c == '=') break;
    const char* p = strchr(b64_chars, c);
    if (!p) continue;
    val = (val << 6) + (p - b64_chars);
    valb += 6;
    if (valb >= 0) {
      out += char((val >> valb) & 0xFF);
      valb -= 8;
    }
  }
  return out;
}

static String base64_decode_from_offset(const String& in, int startIndex) {
  static const char* b64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  const int len = in.length();
  if (startIndex < 0 || startIndex >= len) {
    return String();
  }

  String out;
  out.reserve(((len - startIndex) * 3) / 4);
  int val = 0;
  int valb = -8;

  for (int i = startIndex; i < len; i++) {
    char c = in[i];
    if (c == '\r' || c == '\n' || isspace(c)) continue;
    if (c == '=') break;
    const char* p = strchr(b64_chars, c);
    if (!p) continue;
    val = (val << 6) + (p - b64_chars);
    valb += 6;
    if (valb >= 0) {
      out += char((val >> valb) & 0xFF);
      valb -= 8;
    }
  }

  return out;
}

// Handle UPSERT_NEW_CUSTOMER_JSON_CHUNK command
bool handleUpsertNewCustomerJsonChunk(String payload) {
  int p1 = payload.indexOf('|');
  int p2 = (p1 >= 0) ? payload.indexOf('|', p1 + 1) : -1;
  if (p1 < 0 || p2 < 0) {
    Serial.print(F("ERR|BAD_CHUNK_FORMAT payload='"));
    {
      int previewLen = (payload.length() < 100) ? payload.length() : 100;
      Serial.print(payload.substring(0, previewLen));
    }
    Serial.println(F("'"));
    Serial.print(F("payload_len="));
    Serial.println(payload.length());
    return true;
  }

  int chunkIndex = payload.substring(0, p1).toInt();
  int totalChunks = payload.substring(p1 + 1, p2).toInt();
  const int encodedStart = p2 + 1;
  const int encodedLen = payload.length() - encodedStart;

  if (encodedLen <= 0) {
    Serial.println(F("ERR|EMPTY_BASE64_CHUNK"));
    return true;
  }

  // decode from base64 (matches new web protocol) without copying the encoded payload first
  String jsonChunk = base64_decode_from_offset(payload, encodedStart);

  // diagnostic logging to help debug empty/short payloads
  Serial.print(F("Received chunk "));
  Serial.print(chunkIndex);
  Serial.print(F("/"));
  Serial.print(totalChunks);
  Serial.print(F(" encoded="));
  Serial.print(encodedLen);
  Serial.print(F(" length="));
  Serial.println(jsonChunk.length());
#if SYNC_DEBUG
  Serial.print(F("Payload preview: '"));
  {
    int lp = (jsonChunk.length() < 50) ? jsonChunk.length() : 50;
    Serial.print(jsonChunk.substring(0, lp));
  }
  Serial.println(F("'"));
#endif
  if (jsonChunk.length() < 10) {
    Serial.print(F("Chunk data preview: '"));
    Serial.print(jsonChunk);
    Serial.println(F("'"));
  }

  if (jsonChunk.length() == 0) {
    Serial.println(F("ERR|EMPTY_JSON_CHUNK"));
    return true;
  }

  // Parse the JSON chunk (it's an array of customers)
  DynamicJsonDocument doc(65536); // 64KB for up to 150 customers
  Serial.println(F("About to deserialize JSON for new customers"));
  DeserializationError error = deserializeJson(doc, jsonChunk);
  if (error) {
    Serial.println(F("ERR|JSON_PARSE_FAILED"));
    Serial.print(F("Error: "));
    Serial.println(error.c_str());
    return true;
  }
  Serial.println(F("JSON deserialized successfully"));

  JsonArray customers = doc.as<JsonArray>();
  bool allSuccess = true;

  Serial.print(F("Processing new customer chunk "));
  Serial.println(chunkIndex);
#if SYNC_DEBUG
  // print each customer in debug mode
#endif

  // Set synchronous to NORMAL for speed during sync (durability is still good)
  sqlite3_exec(db, "PRAGMA synchronous = NORMAL;", NULL, NULL, NULL);

  Serial.printf("Heap free before chunk: %d\n", ESP.getFreeHeap());

  // Begin transaction for this chunk
  int rc_begin = sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
  if (rc_begin != SQLITE_OK) {
    Serial.print(F("BEGIN failed for chunk "));
    Serial.print(chunkIndex);
    Serial.print(F(": "));
    Serial.println(sqlite3_errmsg(db));
    Serial.println(F("ERR|UPSERT_FAILED"));
    return true;
  }

  for (JsonObject customer : customers) {
    String accountNo = customer["account_no"] | "";
    String name = customer["customer_name"] | "";
    String address = customer["address"] | "";
    unsigned long prev = customer["previous_reading"] | 0;
    String status = customer["status"] | "active";
    unsigned long typeId = customer["type_id"] | 1;
    unsigned long deductionId = customer["deduction_id"] | 0;
    unsigned long brgyId = customer["brgy_id"] | 1;

    if (!upsertCustomerFromSync(accountNo, name, address, prev, status, typeId, deductionId, brgyId)) {
      allSuccess = false;
      break;
    }
  }

  if (allSuccess) {
    int rc_commit = sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
    if (rc_commit != SQLITE_OK) {
      Serial.print(F("COMMIT failed for chunk "));
      Serial.print(chunkIndex);
      Serial.print(F(": "));
      Serial.println(sqlite3_errmsg(db));
      Serial.println(F("ERR|UPSERT_FAILED"));
      return true;
    } else {
#if SYNC_DEBUG
      Serial.print(F("Processed new customer chunk "));
      Serial.println(chunkIndex);
#endif
    }
    Serial.print(F("ACK|CHUNK|"));
    Serial.println(chunkIndex);
    Serial.printf("Heap free after chunk: %d\n", ESP.getFreeHeap());
    Serial.println(F("Ready for next chunk"));
    Serial.flush();
    if (chunkIndex == totalChunks - 1) {
      // Last chunk, count customers in DB
      int customerCount = 0;
      sqlite3_exec(db, "SELECT COUNT(*) FROM customers;", countCallback, &customerCount, NULL);
      Serial.print(F("Total customers in DB after new sync: "));
      Serial.println(customerCount);
      // Send final success
      Serial.print(F("ACK|UPSERT_NEW_CUSTOMER_JSON|"));
      Serial.println(customers.size());
      Serial.flush();
    }
  } else {
    sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
    Serial.println(F("ERR|UPSERT_FAILED"));
  }
  return true;
}

// Handle UPSERT_UPDATED_CUSTOMER_JSON_CHUNK command
bool handleUpsertUpdatedCustomerJsonChunk(String payload) {
  int p1 = payload.indexOf('|');
  int p2 = (p1 >= 0) ? payload.indexOf('|', p1 + 1) : -1;
  if (p1 < 0 || p2 < 0) {
    Serial.print(F("ERR|BAD_CHUNK_FORMAT|payload='"));
    Serial.print(payload);
    Serial.println(F("'"));
    Serial.print(F("payload_len="));
    Serial.println(payload.length());
    return true;
  }

  int chunkIndex = payload.substring(0, p1).toInt();
  int totalChunks = payload.substring(p1 + 1, p2).toInt();
  const int encodedStart = p2 + 1;
  const int encodedLen = payload.length() - encodedStart;

  if (encodedLen <= 0) {
    Serial.println(F("ERR|EMPTY_BASE64_CHUNK"));
    return true;
  }

  // decode base64 without copying the encoded payload first
  String jsonChunk = base64_decode_from_offset(payload, encodedStart);
  Serial.print(F("Received updated chunk "));
  Serial.print(chunkIndex);
  Serial.print(F("/"));
  Serial.print(totalChunks);
  Serial.print(F(" encoded="));
  Serial.print(encodedLen);
  Serial.print(F(" length="));
  Serial.println(jsonChunk.length());
  if (jsonChunk.length() < 10) {
    Serial.print(F("Chunk data preview: '"));
    Serial.print(jsonChunk);
    Serial.println(F("'"));
  }
  if (jsonChunk.length() == 0) {
    Serial.println(F("ERR|EMPTY_JSON_CHUNK"));
    return true;
  }

  // Parse the JSON chunk (it's an array of customers)
  DynamicJsonDocument doc(65536); // 64KB for up to 150 customers
  Serial.println(F("About to deserialize JSON for updated customers"));
  DeserializationError error = deserializeJson(doc, jsonChunk);
  if (error) {
    Serial.println(F("ERR|JSON_PARSE_FAILED"));
    Serial.print(F("Error: "));
    Serial.println(error.c_str());
    return true;
  }
  Serial.println(F("JSON deserialized successfully"));

  JsonArray customers = doc.as<JsonArray>();
  bool allSuccess = true;

  Serial.print(F("Processing updated customer chunk "));
  Serial.println(chunkIndex);

  // Set synchronous to NORMAL for speed during sync (durability is still good)
  sqlite3_exec(db, "PRAGMA synchronous = NORMAL;", NULL, NULL, NULL);

  Serial.printf("Heap free before chunk: %d\n", ESP.getFreeHeap());

  // Begin transaction for this chunk
  int rc_begin = sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
  if (rc_begin != SQLITE_OK) {
    Serial.print(F("BEGIN failed for chunk "));
    Serial.print(chunkIndex);
    Serial.print(F(": "));
    Serial.println(sqlite3_errmsg(db));
    Serial.println(F("ERR|UPSERT_FAILED"));
    return true;
  }

  for (JsonObject customer : customers) {
    String accountNo = customer["account_no"] | "";
    String name = customer["customer_name"] | "";
    String address = customer["address"] | "";
    unsigned long prev = customer["previous_reading"] | 0;
    String status = customer["status"] | "active";
    unsigned long typeId = customer["type_id"] | 1;
    unsigned long deductionId = customer["deduction_id"] | 0;
    unsigned long brgyId = customer["brgy_id"] | 1;

    if (!upsertCustomerFromSync(accountNo, name, address, prev, status, typeId, deductionId, brgyId)) {
      allSuccess = false;
      break;
    }
  }

  if (allSuccess) {
    int rc_commit = sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
    if (rc_commit != SQLITE_OK) {
      Serial.print(F("COMMIT failed for chunk "));
      Serial.print(chunkIndex);
      Serial.print(F(": "));
      Serial.println(sqlite3_errmsg(db));
      Serial.println(F("ERR|UPSERT_FAILED"));
      return true;
    } else {
      Serial.print(F("Processed updated customer chunk "));
      Serial.println(chunkIndex);
    }
    Serial.print(F("ACK|CHUNK|"));
    Serial.println(chunkIndex);
    Serial.printf("Heap free after chunk: %d\n", ESP.getFreeHeap());
    Serial.println(F("Ready for next chunk"));
    Serial.flush();
    if (chunkIndex == totalChunks - 1) {
      // Last chunk, count customers in DB
      int customerCount = 0;
      sqlite3_exec(db, "SELECT COUNT(*) FROM customers;", countCallback, &customerCount, NULL);
      Serial.print(F("Total customers in DB after update sync: "));
      Serial.println(customerCount);
      // Send final success
      Serial.print(F("ACK|UPSERT_UPDATED_CUSTOMER_JSON|"));
      Serial.println(customers.size());
      Serial.flush();
    }
  } else {
    sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
    Serial.println(F("ERR|UPSERT_FAILED"));
  }
  return true;
}

#endif // CUSTOMER_SYNC_H