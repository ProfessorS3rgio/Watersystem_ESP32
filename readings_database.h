#ifndef READINGS_DATABASE_H
#define READINGS_DATABASE_H

#include <SD.h>
#include "config.h"
#include "customers_database.h"

// SD-backed, append-only readings log.
// File format (pipe-separated):
// account_no|previous_reading|current_reading|usage_m3|reading_at_epoch|synced

static const char* READINGS_SYNC_FILE = "/WATER_DB/READINGS/readings.psv";
static const char* TIME_OFFSET_FILE = "/WATER_DB/SETTINGS/time_offset.txt";

static long g_timeOffsetSeconds = 0; // epochNow ~= millis()/1000 + offset

static bool isSdReadyForReadings() {
  return (SD.cardType() != CARD_NONE);
}

static void deselectTftSelectSd() {
  pinMode(TFT_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  // SD library controls SD_CS during operations; keep it high when idle.
  digitalWrite(SD_CS, HIGH);
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

  f.println(F("# account_no|previous_reading|current_reading|usage_m3|reading_at_epoch|synced"));
  f.close();
  return true;
}

static bool appendReadingToSD(const String& accountNo, unsigned long prev, unsigned long curr, unsigned long usage, uint32_t readingAtEpoch) {
  if (!isSdReadyForReadings()) return false;

  (void)ensureReadingsFileExists();

  deselectTftSelectSd();

  File f = SD.open(READINGS_SYNC_FILE, FILE_WRITE);
  if (!f) return false;

  String acct = accountNo;
  acct.trim();
  acct.replace("\r", "");
  acct.replace("\n", "");
  acct.replace("|", " ");

  f.print(acct);
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

static bool markAllReadingsSynced() {
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

    // account|prev|curr|usage|epoch|synced
    int p1 = line.indexOf('|');
    int p2 = (p1 >= 0) ? line.indexOf('|', p1 + 1) : -1;
    int p3 = (p2 >= 0) ? line.indexOf('|', p2 + 1) : -1;
    int p4 = (p3 >= 0) ? line.indexOf('|', p3 + 1) : -1;
    int p5 = (p4 >= 0) ? line.indexOf('|', p4 + 1) : -1;
    if (p1 < 0 || p2 < 0 || p3 < 0 || p4 < 0 || p5 < 0) {
      // keep malformed line as-is
      out.println(line);
      continue;
    }

    String synced = line.substring(p5 + 1);
    synced.trim();

    if (synced == "1") {
      out.println(line);
    } else {
      // rewrite with synced=1
      out.print(line.substring(0, p5 + 1));
      out.println('1');
    }
  }

  in.close();
  out.close();

  SD.remove(READINGS_SYNC_FILE);
  bool ok = SD.rename(TMP_FILE, READINGS_SYNC_FILE);
  return ok;
}

static void exportReadingsForSync() {
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

    // account|prev|curr|usage|epoch|synced
    int p1 = line.indexOf('|');
    int p2 = (p1 >= 0) ? line.indexOf('|', p1 + 1) : -1;
    int p3 = (p2 >= 0) ? line.indexOf('|', p2 + 1) : -1;
    int p4 = (p3 >= 0) ? line.indexOf('|', p3 + 1) : -1;
    int p5 = (p4 >= 0) ? line.indexOf('|', p4 + 1) : -1;
    if (p1 < 0 || p2 < 0 || p3 < 0 || p4 < 0 || p5 < 0) {
      continue;
    }

    String synced = line.substring(p5 + 1);
    synced.trim();
    if (synced == "1") continue;

    String acct = line.substring(0, p1);
    String prev = line.substring(p1 + 1, p2);
    String curr = line.substring(p2 + 1, p3);
    String usage = line.substring(p3 + 1, p4);
    String epoch = line.substring(p4 + 1, p5);

    acct.trim();
    prev.trim();
    curr.trim();
    usage.trim();
    epoch.trim();

    Serial.print(F("READ|"));
    Serial.print(acct);
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

static bool recordReadingForCustomerIndex(int customerIndex, unsigned long currReading) {
  Customer* cust = getCustomerAt(customerIndex);
  if (cust == nullptr) return false;

  unsigned long prev = cust->previous_reading;
  if (currReading <= prev) return false;

  unsigned long usage = currReading - prev;
  uint32_t epoch = deviceEpochNow();

  bool ok = appendReadingToSD(cust->account_no, prev, currReading, usage, epoch);

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
