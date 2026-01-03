#ifndef SDCARD_MANAGER_H
#define SDCARD_MANAGER_H

#include <SD.h>
#include "config.h"

// Global SD card status
bool sdCardPresent = false;

// Forward declarations
void initSDCard();
void initSDCardDatabase();
void checkSDCardStatus();
bool isSDCardReady();

// ===== SD CARD INITIALIZATION =====
void initSDCard() {
  Serial.println(F("Initializing SD card..."));
  
  // Deselect TFT before initializing SD
  digitalWrite(TFT_CS, HIGH);
  
  if (!SD.begin(SD_CS)) {
    Serial.println(F("SD card initialization failed!"));
    Serial.println(F("Check: 1) Card inserted? 2) Wiring correct? 3) Card formatted as FAT32?"));
    sdCardPresent = false;
  } else {
    Serial.println(F("SD card initialized successfully!"));
    sdCardPresent = true;
    
    // Print card info
    uint8_t cardType = SD.cardType();
    Serial.print(F("SD Card Type: "));
    switch (cardType) {
      case CARD_NONE:
        Serial.println(F("No card detected"));
        sdCardPresent = false;
        break;
      case CARD_MMC:
        Serial.println(F("MMC"));
        break;
      case CARD_SD:
        Serial.println(F("SDSC"));
        break;
      case CARD_SDHC:
        Serial.println(F("SDHC"));
        break;
      default:
        Serial.println(F("Unknown"));
    }
    
    if (sdCardPresent) {
      uint64_t cardSize = SD.cardSize() / (1024 * 1024);
      Serial.print(F("SD Card Size: "));
      Serial.print((uint32_t)cardSize);
      Serial.println(F(" MB"));
      
      uint64_t usedBytes = SD.usedBytes() / (1024 * 1024);
      uint64_t totalBytes = SD.totalBytes() / (1024 * 1024);
      Serial.print(F("Used: "));
      Serial.print((uint32_t)usedBytes);
      Serial.print(F(" MB / "));
      Serial.print((uint32_t)totalBytes);
      Serial.println(F(" MB"));
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
  Serial.println(F("Initializing SD Card Database..."));
  
  // Deselect TFT
  digitalWrite(TFT_CS, HIGH);
  
  // Define database directories
  const char* dbDirs[] = {
    DB_ROOT,
    DB_CUSTOMERS,
    DB_BILLS,
    DB_READINGS,
    DB_LOGS
  };
  
  const int numDirs = sizeof(dbDirs) / sizeof(dbDirs[0]);
  
  for (int i = 0; i < numDirs; i++) {
    if (!SD.exists(dbDirs[i])) {
      if (SD.mkdir(dbDirs[i])) {
        Serial.print(F("Created: "));
        Serial.println(dbDirs[i]);
      } else {
        Serial.print(F("Failed to create: "));
        Serial.println(dbDirs[i]);
      }
    } else {
      Serial.print(F("Exists: "));
      Serial.println(dbDirs[i]);
    }
  }
  
  Serial.println(F("Database initialization complete!"));
  Serial.println(F("Directory Structure:"));
  Serial.println(F("/WATER_DB/"));
  Serial.println(F("  ├─ /CUSTOMERS  (Customer data)"));
  Serial.println(F("  ├─ /BILLS      (Generated bills)"));
  Serial.println(F("  ├─ /READINGS   (Meter readings)"));
  Serial.println(F("  └─ /LOGS       (System logs)"));
  
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

#endif  // SDCARD_MANAGER_H
