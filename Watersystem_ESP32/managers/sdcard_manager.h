#ifndef SDCARD_MANAGER_H
#define SDCARD_MANAGER_H

#include <SD.h>
#include "../configuration/config.h"

extern SPIClass SPI_SD;

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
  
  SPI_SD.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
  
  if (!SD.begin(SD_CS, SPI_SD)) {
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
    DB_ASSETS
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
  
  SPI_SD.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
  
  if (!SD.begin(SD_CS, SPI_SD)) {
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

// Recursive directory deletion helper
bool deleteDirectoryRecursive(const char* path) {
  File dir = SD.open(path);
  if (!dir || !dir.isDirectory()) {
    return SD.remove(path);
  }
  
  File file = dir.openNextFile();
  while (file) {
    char filePath[256];
    sprintf(filePath, "%s/%s", path, file.name());
    
    if (file.isDirectory()) {
      if (!deleteDirectoryRecursive(filePath)) {
        file.close();
        dir.close();
        return false;
      }
    } else {
      if (!SD.remove(filePath)) {
        Serial.print(F("Failed to delete file: "));
        Serial.println(filePath);
        file.close();
        dir.close();
        return false;
      }
    }
    file = dir.openNextFile();
  }
  
  dir.close();
  return SD.rmdir(path);
}

// ===== SD CARD FORMATTING =====
bool formatSDCard() {
  if (!isSDCardReady()) {
    Serial.println(F("SD card not ready for formatting"));
    return false;
  }

  Serial.println(F("Starting SD card format..."));
  Serial.println(F("WARNING: This will delete ALL data on the SD card!"));
  
  // For Arduino SD library, we can't do a full filesystem format
  // Instead, we delete all files and recreate directory structure
  // This preserves the FAT32 filesystem while clearing all data
  
  // Delete all files in WATER_DB directory recursively
  if (SD.exists(DB_ROOT)) {
    Serial.println(F("Deleting existing WATER_DB directory..."));
    if (!deleteDirectoryRecursive(DB_ROOT)) {
      Serial.println(F("Failed to delete WATER_DB directory"));
      return false;
    }
  }
  
  Serial.println(F("Recreating directory structure..."));
  
  // Recreate the directory structure
  if (!SD.mkdir(DB_ROOT)) {
    Serial.println(F("Failed to recreate WATER_DB directory"));
    return false;
  }
  
  if (!SD.mkdir(DB_ASSETS)) {
    Serial.println(F("Failed to recreate ASSETS directory"));
    return false;
  }
  
  Serial.println(F("SD card formatted successfully"));
  Serial.println(F("Note: Filesystem preserved, all application data cleared"));
  return true;
}


#endif  // SDCARD_MANAGER_H
