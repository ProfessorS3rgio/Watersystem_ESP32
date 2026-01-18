#ifndef SDCARD_MANAGER_H
#define SDCARD_MANAGER_H

#include <SD.h>
#include "../configuration/config.h"

// Global SD card status
bool sdCardPresent = false;

// Ensure TFT deselected and SD CS is high before SD operations
void deselectTftSelectSd() {
  pinMode(TFT_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(SD_CS, HIGH);
}

// Forward declarations
void initSDCard();
void initSDCardDatabase();
void checkSDCardStatus();
bool isSDCardReady();

// ===== SD CARD INITIALIZATION =====
void initSDCard() {
#if WS_SERIAL_VERBOSE
  Serial.println(F("Initializing SD card..."));
#endif
  
  // Deselect TFT before initializing SD
  digitalWrite(TFT_CS, HIGH);
  
  if (!SD.begin(SD_CS)) {
    Serial.println(F("SD card initialization failed!"));
    Serial.println(F("Check: 1) Card inserted? 2) Wiring correct? 3) Card formatted as FAT32?"));
    sdCardPresent = false;
  } else {
#if WS_SERIAL_VERBOSE
    Serial.println(F("SD card initialized successfully!"));
#endif
    sdCardPresent = true;
    
    // Print card info
    uint8_t cardType = SD.cardType();
#if WS_SERIAL_VERBOSE
    Serial.print(F("SD Card Type: "));
#endif
    switch (cardType) {
      case CARD_NONE:
#if WS_SERIAL_VERBOSE
        Serial.println(F("No card detected"));
#endif
        sdCardPresent = false;
        break;
      case CARD_MMC:
#if WS_SERIAL_VERBOSE
        Serial.println(F("MMC"));
#endif
        break;
      case CARD_SD:
#if WS_SERIAL_VERBOSE
        Serial.println(F("SDSC"));
#endif
        break;
      case CARD_SDHC:
#if WS_SERIAL_VERBOSE
        Serial.println(F("SDHC"));
#endif
        break;
      default:
#if WS_SERIAL_VERBOSE
        Serial.println(F("Unknown"));
#endif
        break;
    }
    
    if (sdCardPresent) {
      uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    #if WS_SERIAL_VERBOSE
      Serial.print(F("SD Card Size: "));
      Serial.print((uint32_t)cardSize);
      Serial.println(F(" MB"));
    #endif
      
      uint64_t usedBytes = SD.usedBytes() / (1024 * 1024);
      uint64_t totalBytes = SD.totalBytes() / (1024 * 1024);
    #if WS_SERIAL_VERBOSE
      Serial.print(F("Used: "));
      Serial.print((uint32_t)usedBytes);
      Serial.print(F(" MB / "));
      Serial.print((uint32_t)totalBytes);
      Serial.println(F(" MB"));
    #endif
    }
  }
  
  // Reselect TFT
  digitalWrite(SD_CS, HIGH);
  
  // Initialize database directories if SD card is present
  if (sdCardPresent) {
    initSDCardDatabase();
  }
}

// ===== DATABASE DIRECTORY INITIALIZATION =====
void initSDCardDatabase() {
#if WS_SERIAL_VERBOSE
  Serial.println(F("Initializing SD Card Database..."));
#endif
  
  // Deselect TFT
  digitalWrite(TFT_CS, HIGH);
  
  // Define database directories
  const char* dbDirs[] = {
    DB_ROOT,
    DB_CUSTOMERS,
    DB_BILLS,
    DB_READINGS,
    DB_LOGS,
    DB_SETTINGS,
    DB_DEDUCTIONS,
    DB_CUSTOMER_TYPES
  };
  
  const int numDirs = sizeof(dbDirs) / sizeof(dbDirs[0]);
  
  for (int i = 0; i < numDirs; i++) {
    if (!SD.exists(dbDirs[i])) {
      if (SD.mkdir(dbDirs[i])) {
#if WS_SERIAL_VERBOSE
        Serial.print(F("Created: "));
        Serial.println(dbDirs[i]);
#endif
      } else {
#if WS_SERIAL_VERBOSE
        Serial.print(F("Failed to create: "));
        Serial.println(dbDirs[i]);
#endif
      }
    } else {
#if WS_SERIAL_VERBOSE
      Serial.print(F("Exists: "));
      Serial.println(dbDirs[i]);
#endif
    }
  }

#if WS_SERIAL_VERBOSE
  Serial.println(F("Database initialization complete!"));
  Serial.println(F("Directory Structure:"));
  Serial.println(F("/WATER_DB/"));
  Serial.println(F("  ├─ /CUSTOMERS  (Customer data)"));
  Serial.println(F("  ├─ /BILLS      (Generated bills)"));
  Serial.println(F("  ├─ /READINGS   (Meter readings)"));
  Serial.println(F("  ├─ /SETTINGS   (System settings)"));
  Serial.println(F("  └─ /LOGS       (System logs)"));
#endif
  
  // Reselect TFT
  digitalWrite(SD_CS, HIGH);
}

// ===== SD CARD STATUS CHECK =====
void checkSDCardStatus() {
  Serial.println(F("===== SD Card Status ====="));
  
  // Try to reinitialize to check current status
  digitalWrite(TFT_CS, HIGH);
  
  if (!SD.begin(SD_CS)) {
    Serial.println(F("Status: NOT CONNECTED or NO CARD"));
    sdCardPresent = false;
  } else {
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
      Serial.println(F("Status: NO CARD INSERTED"));
      sdCardPresent = false;
    } else {
      Serial.println(F("Status: CONNECTED"));
      sdCardPresent = true;
      
      uint64_t cardSize = SD.cardSize() / (1024 * 1024);
      uint64_t usedBytes = SD.usedBytes() / (1024 * 1024);
      uint64_t totalBytes = SD.totalBytes() / (1024 * 1024);
      
      Serial.print(F("Size: "));
      Serial.print((uint32_t)cardSize);
      Serial.println(F(" MB"));
      
      Serial.print(F("Used: "));
      Serial.print((uint32_t)usedBytes);
      Serial.print(F(" MB / "));
      Serial.print((uint32_t)totalBytes);
      Serial.println(F(" MB"));
    }
  }
  
  digitalWrite(SD_CS, HIGH);
  Serial.println(F("=========================="));
}

// ===== SD CARD STATUS QUERY =====
bool isSDCardReady() {
  return sdCardPresent;
}

// ===== WATER RATE SETTINGS =====
void saveWaterRate(float rate) {
  if (!sdCardPresent) {
    Serial.println(F("Cannot save rate: SD card not available"));
    return;
  }
  
  digitalWrite(TFT_CS, HIGH);
  
  File rateFile = SD.open("/WATER_DB/SETTINGS/settings.txt", FILE_WRITE);
  if (rateFile) {
    rateFile.println(rate, 2);
    rateFile.close();
    Serial.print(F("Water rate saved: "));
    Serial.println(rate, 2);
  } else {
    Serial.println(F("Failed to save water rate"));
  }
  
  digitalWrite(SD_CS, HIGH);
}

float loadWaterRate() {
  float defaultRate = 15.00;
  
  if (!sdCardPresent) {
#if WS_SERIAL_VERBOSE
    Serial.println(F("SD card not available, using default rate"));
#endif
    return defaultRate;
  }
  
  digitalWrite(TFT_CS, HIGH);
  
  if (!SD.exists("/WATER_DB/SETTINGS/settings.txt")) {
#if WS_SERIAL_VERBOSE
    Serial.println(F("Settings file not found, using default rate"));
#endif
    digitalWrite(SD_CS, HIGH);
    return defaultRate;
  }
  
  File rateFile = SD.open("/WATER_DB/SETTINGS/settings.txt", FILE_READ);
  if (rateFile) {
    String rateStr = rateFile.readStringUntil('\n');
    rateFile.close();
    float loadedRate = rateStr.toFloat();
    
    if (loadedRate > 0) {
#if WS_SERIAL_VERBOSE
      Serial.print(F("Water rate loaded: "));
      Serial.println(loadedRate, 2);
#endif
      digitalWrite(SD_CS, HIGH);
      return loadedRate;
    }
  }
  
#if WS_SERIAL_VERBOSE
  Serial.println(F("Failed to load rate, using default"));
#endif
  digitalWrite(SD_CS, HIGH);
  return defaultRate;
}


#endif  // SDCARD_MANAGER_H
