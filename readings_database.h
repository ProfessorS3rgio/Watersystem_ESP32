#ifndef READINGS_DATABASE_H
#define READINGS_DATABASE_H

#include <SD.h>
#include <time.h>
#include "config.h"
#include "sdcard_manager.h"
#include "customers_database.h"

// SD-backed, append-only readings log.
// File format (pipe-separated):
// customer_id|device_id|previous_reading|current_reading|usage_m3|reading_at_epoch|synced

static const char* READINGS_SYNC_FILE = DB_READINGS "/readings.psv";
static const char* TIME_OFFSET_FILE = DB_SETTINGS "/time_offset.txt";

static long g_timeOffsetSeconds = 0; // epochNow ~= millis()/1000 + offset

// Device ID for this ESP32 device (Makilas barangay)
static const unsigned long DEVICE_ID = 2;

static bool isSdReadyForReadings() {
  return (SD.cardType() != CARD_NONE);
}

static uint32_t deviceEpochNow() {
  return (uint32_t)((long)(millis() / 1000) + g_timeOffsetSeconds);
}

static bool loadDeviceTimeOffsetFromSD() {
  if (!isSdReadyForReadings()) return false;

  deselectTftSelectSd();

  if (!SD.exists(TIME_OFFSET_FILE)) {
    return false;
  }

  File f = SD.open(TIME_OFFSET_FILE, FILE_READ);
  if (!f) return false;

  String s = f.readStringUntil('\n');
  f.close();
  s.trim();
  if (s.length() == 0) return false;

  g_timeOffsetSeconds = (long)s.toInt();
  return true;
}

static bool saveDeviceTimeOffsetToSD() {
  if (!isSdReadyForReadings()) return false;

  deselectTftSelectSd();

  if (SD.exists(TIME_OFFSET_FILE)) {
    SD.remove(TIME_OFFSET_FILE);
  }

  File f = SD.open(TIME_OFFSET_FILE, FILE_WRITE);
  if (!f) return false;

  f.println(g_timeOffsetSeconds);
  f.close();
  return true;
}

static void setDeviceEpoch(uint32_t epochSeconds) {
  g_timeOffsetSeconds = (long)epochSeconds - (long)(millis() / 1000);
  (void)saveDeviceTimeOffsetToSD();
}

static bool ensureReadingsFileExists() {
  if (!isSdReadyForReadings()) return false;

  deselectTftSelectSd();

  if (SD.exists(READINGS_SYNC_FILE)) {
    return true;
  }

  File f = SD.open(READINGS_SYNC_FILE, FILE_WRITE);
  if (!f) return false;

  f.println(F("# customer_id|device_id|previous_reading|current_reading|usage_m3|reading_at_epoch|synced"));
  f.close();
  return true;
}

bool appendReadingToSD(unsigned long customerId, unsigned long prev, unsigned long curr, unsigned long usage, uint32_t readingAtEpoch) {
  if (!isSdReadyForReadings()) return false;

  (void)ensureReadingsFileExists();

  deselectTftSelectSd();

  File f = SD.open(READINGS_SYNC_FILE, FILE_WRITE);
  if (!f) return false;

  f.print(customerId);
  f.print('|');
  f.print(DEVICE_ID);
  f.print('|');
  f.print(prev);
  f.print('|');
  f.print(curr);
  f.print('|');
  f.print(usage);
  f.print('|');
  f.print(readingAtEpoch);
  f.print('|');
  f.println('0'); // not yet synced

  f.close();
  return true;
}

static bool epochToYearMonth(uint32_t epoch, int& yearOut, int& monthOut) {
  time_t t = (time_t)epoch;
  struct tm tmv;
#if defined(ARDUINO_ARCH_ESP32)
  if (gmtime_r(&t, &tmv) == nullptr) return false;
#else
  struct tm* ptm = gmtime(&t);
  if (ptm == nullptr) return false;
  tmv = *ptm;
#endif

  yearOut = tmv.tm_year + 1900;
  monthOut = tmv.tm_mon + 1;
  return (yearOut >= 1970 && monthOut >= 1 && monthOut <= 12);
}

static bool hasReadingForCustomerInYearMonth(unsigned long customerId, int year, int month) {
  if (!isSdReadyForReadings()) return false;
  if (!SD.exists(READINGS_SYNC_FILE)) return false;

  deselectTftSelectSd();
  File f = SD.open(READINGS_SYNC_FILE, FILE_READ);
  if (!f) return false;

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    if (line.startsWith("#")) continue;

    // customer_id|device_id|prev|curr|usage|epoch|synced
    int p1 = line.indexOf('|');
    int p2 = (p1 >= 0) ? line.indexOf('|', p1 + 1) : -1;
    int p3 = (p2 >= 0) ? line.indexOf('|', p2 + 1) : -1;
    int p4 = (p3 >= 0) ? line.indexOf('|', p3 + 1) : -1;
    int p5 = (p4 >= 0) ? line.indexOf('|', p4 + 1) : -1;
    int p6 = (p5 >= 0) ? line.indexOf('|', p5 + 1) : -1;
    if (p1 < 0 || p2 < 0 || p3 < 0 || p4 < 0 || p5 < 0 || p6 < 0) continue;

    String custIdStr = line.substring(0, p1);
    custIdStr.trim();
    unsigned long custId = (unsigned long)custIdStr.toInt();
    if (custId != customerId) continue;

    String epochStr = line.substring(p5 + 1, p6);
    epochStr.trim();
    uint32_t epoch = (uint32_t)epochStr.toInt();
    if (epoch == 0) continue;

    int ry = 0;
    int rm = 0;
    if (!epochToYearMonth(epoch, ry, rm)) continue;
    if (ry == year && rm == month) {
      f.close();
      return true;
    }
  }

  f.close();
  return false;
}

static bool hasReadingForCustomerThisMonth(unsigned long customerId) {
  int y = 0;
  int m = 0;
  uint32_t nowEpoch = deviceEpochNow();
  if (!epochToYearMonth(nowEpoch, y, m)) return false;
  return hasReadingForCustomerInYearMonth(customerId, y, m);
}

bool hasReadingForAccountThisMonth(String accountNo) {
  unsigned long customerId = getCustomerIdByAccount(accountNo);
  if (customerId == 0) return false;
  return hasReadingForCustomerThisMonth(customerId);
}

bool markAllReadingsSynced() {
  if (!isSdReadyForReadings()) return false;
  if (!SD.exists(READINGS_SYNC_FILE)) return true;

  const char* TMP_FILE = "/WATER_DB/READINGS/readings.tmp";

  deselectTftSelectSd();

  File in = SD.open(READINGS_SYNC_FILE, FILE_READ);
  if (!in) return false;

  if (SD.exists(TMP_FILE)) {
    SD.remove(TMP_FILE);
  }

  File out = SD.open(TMP_FILE, FILE_WRITE);
  if (!out) {
    in.close();
    return false;
  }

  while (in.available()) {
    String line = in.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    if (line.startsWith("#")) {
      out.println(line);
      continue;
    }

    // customer_id|device_id|prev|curr|usage|epoch|synced
    int p1 = line.indexOf('|');
    int p2 = (p1 >= 0) ? line.indexOf('|', p1 + 1) : -1;
    int p3 = (p2 >= 0) ? line.indexOf('|', p2 + 1) : -1;
    int p4 = (p3 >= 0) ? line.indexOf('|', p3 + 1) : -1;
    int p5 = (p4 >= 0) ? line.indexOf('|', p4 + 1) : -1;
    int p6 = (p5 >= 0) ? line.indexOf('|', p5 + 1) : -1;
    if (p1 < 0 || p2 < 0 || p3 < 0 || p4 < 0 || p5 < 0 || p6 < 0) {
      // keep malformed line as-is
      out.println(line);
      continue;
    }

    String synced = line.substring(p6 + 1);
    synced.trim();

    if (synced == "1") {
      out.println(line);
    } else {
      // rewrite with synced=1
      out.print(line.substring(0, p6 + 1));
      out.println('1');
    }
  }

  in.close();
  out.close();

  SD.remove(READINGS_SYNC_FILE);
  bool ok = SD.rename(TMP_FILE, READINGS_SYNC_FILE);
  return ok;
}

void exportReadingsForSync() {
  uint32_t startMs = millis();
  Serial.println(F("BEGIN_READINGS"));

  uint32_t exported = 0;

  if (!isSdReadyForReadings() || !SD.exists(READINGS_SYNC_FILE)) {
    Serial.println(F("END_READINGS"));
    uint32_t elapsedMs = millis() - startMs;
    Serial.print(F("Done. ("));
    Serial.print(elapsedMs);
    Serial.println(F(" ms)"));
    Serial.print(F("Total readings exported: "));
    Serial.println(exported);
    return;
  }

  deselectTftSelectSd();

  File f = SD.open(READINGS_SYNC_FILE, FILE_READ);
  if (!f) {
    Serial.println(F("END_READINGS"));
    uint32_t elapsedMs = millis() - startMs;
    Serial.print(F("Done. ("));
    Serial.print(elapsedMs);
    Serial.println(F(" ms)"));
    Serial.print(F("Total readings exported: "));
    Serial.println(exported);
    return;
  }

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    if (line.startsWith("#")) continue;

    // customer_id|device_id|prev|curr|usage|epoch|synced
    int p1 = line.indexOf('|');
    int p2 = (p1 >= 0) ? line.indexOf('|', p1 + 1) : -1;
    int p3 = (p2 >= 0) ? line.indexOf('|', p2 + 1) : -1;
    int p4 = (p3 >= 0) ? line.indexOf('|', p3 + 1) : -1;
    int p5 = (p4 >= 0) ? line.indexOf('|', p4 + 1) : -1;
    int p6 = (p5 >= 0) ? line.indexOf('|', p5 + 1) : -1;
    if (p1 < 0 || p2 < 0 || p3 < 0 || p4 < 0 || p5 < 0 || p6 < 0) {
      continue;
    }

    String synced = line.substring(p6 + 1);
    synced.trim();
    if (synced == "1") continue;

    String customerId = line.substring(0, p1);
    String deviceId = line.substring(p1 + 1, p2);
    String prev = line.substring(p2 + 1, p3);
    String curr = line.substring(p3 + 1, p4);
    String usage = line.substring(p4 + 1, p5);
    String epoch = line.substring(p5 + 1, p6);

    customerId.trim();
    deviceId.trim();
    prev.trim();
    curr.trim();
    usage.trim();
    epoch.trim();

    // Only export readings for customers in this device's barangay
    unsigned long custIdNum = (unsigned long)customerId.toInt();
    if (!isCustomerInDeviceBarangay(custIdNum)) {
      continue;
    }

    Serial.print(F("READ|"));
    Serial.print(customerId);
    Serial.print('|');
    Serial.print(deviceId);
    Serial.print('|');
    Serial.print(prev);
    Serial.print('|');
    Serial.print(curr);
    Serial.print('|');
    Serial.print(usage);
    Serial.print('|');
    Serial.println(epoch);

    exported++;
  }

  f.close();
  Serial.println(F("END_READINGS"));

  uint32_t elapsedMs = millis() - startMs;
  Serial.print(F("Done. ("));
  Serial.print(elapsedMs);
  Serial.println(F(" ms)"));
  Serial.print(F("Total readings exported: "));
  Serial.println(exported);
}

bool recordReadingForCustomerIndex(int customerIndex, unsigned long currReading) {
  Customer* cust = getCustomerAt(customerIndex);
  if (cust == nullptr) return false;

  unsigned long prev = cust->previous_reading;
  if (currReading <= prev) return false;

  unsigned long usage = currReading - prev;
  uint32_t epoch = deviceEpochNow();

  bool ok = appendReadingToSD(cust->customer_id, prev, currReading, usage, epoch);

  // Update the customer's last reading (even if SD append fails, keep in-memory consistent)
  (void)updateCustomerReading(cust->account_no, currReading);

  return ok;
}

static void initReadingsDatabase() {
  // Load time offset if possible (so reading_at has stable dates)
  (void)loadDeviceTimeOffsetFromSD();
  (void)ensureReadingsFileExists();
}

#endif  // READINGS_DATABASE_H
