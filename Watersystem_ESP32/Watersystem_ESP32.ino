// ===== MAIN WATERSYSTEM ESP32 SKETCH =====
#include <SPI.h>
SPIClass SPI_SD(VSPI);
#include <SD.h>
#include "Adafruit_Thermal.h"
#include <Keypad.h>
#include <time.h>
#include <Wire.h>
#include <RTClib.h>

// TFT_eSPI configuration (must be before #include <TFT_eSPI.h>)
#define ILI9341_DRIVER
#define TFT_CS    15
#define TFT_DC    2
#define TFT_RST   4
#define TFT_BL    21
#define TFT_MOSI  13
#define TFT_SCLK  14
#define TFT_MISO  12
#include <TFT_eSPI.h>

// ===== REFACTORED HEADER FILES =====
#include "configuration/config.h"
#include "database/customers_database.h"
#include "database/readings_database.h"
#include "database/device_info.h"
#include "database/database_manager.h"
#include "database/deduction_database.h"
#include "database/customer_type_database.h"
#include "database/barangay_database.h"
#include "database/database_viewer.h"
#include "database/bill_transactions_database.h"
#include "managers/print_manager.h"
#include "managers/tft_screen_manager.h"
#include "managers/sdcard_manager.h"
#include "managers/keypad_manager.h"
#include "configuration/logo.h"
#include "managers/sync_manager.h"
#include "components/battery_display.h"
#include "components/bmp_display.h"
#include "printer/bill_printer.h"
#include "printer/receipt_printer.h"
#include "screens/boot_screen.h"




// ===== TFT DISPLAY =====
TFT_eSPI tft = TFT_eSPI();

// ===== THERMAL PRINTER =====
HardwareSerial printerSerial(2);  // Use UART2 on ESP32
Adafruit_Thermal printer(&printerSerial);

// ===== RTC MODULE =====
RTC_DS3231 rtc;

// ===== KEYPAD SIMULATION HELPER =====
bool isValidKeypadKey(char key) {
  const char validKeys[] = {'1','2','3','A','4','5','6','B','7','8','9','C','*','0','#','D'};
  for (char k : validKeys) {
    if (k == key) return true;
  }
  return false;
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.setRxBufferSize(262144); // 256KB for large JSON payloads
  Serial.setTimeout(30000); // 30 seconds timeout for long transmissions
  
  // Initialize I2C for RTC
  Wire.begin(RTC_SDA, RTC_SCL);
  if (! rtc.begin()) {
    Serial.println(F("Couldn't find RTC"));
  } else {
    Serial.println(F("RTC initialized"));
    if (rtc.lostPower()) {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      Serial.println(F("RTC time set to compile time"));
    } else {
      Serial.println(F("RTC time preserved"));
    }
  }
  
  // Initialize TFT Backlight
  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);  // Turn on backlight
  
  // Initialize SPI (SCK, MISO, MOSI)
  SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI);
  
  // Initialize TFT
  tft.begin();  // For 320x240 ILI9341
  tft.setRotation(1);          // Landscape mode (240x320)
  tft.fillScreen(COLOR_BG);

  // Ensure shared SPI CS pins are in a safe state before SD init
  pinMode(TFT_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(SD_CS, HIGH);

  // Boot screen: console-style checks (SD + settings + printer)
  showBootScreen();

  // Initialize SQLite Database
  initDatabase();

  // Initialize Readings database (time offset + readings log)
  initReadingsDatabase();

  // Initialize Device Info (about/last sync/print count)
  initDeviceInfo();

  // Initialize Customers Database
  initCustomersDatabase();

  // Initialize Deductions Database
  initDeductionsDatabase();

  // Initialize Barangays Database
  initBarangaysDatabase();

  // Initialize Customer Types Database
  initCustomerTypesDatabase();

  // Initialize Settings Database
  initSettingsDatabase();

  // Initialize Bills Database
  initBillsDatabase();

#if WS_SERIAL_VERBOSE
  Serial.println(F("Watersystem ESP32 ready."));
  Serial.println(F("Use keypad or serial commands:"));
  Serial.println(F("Press D or B on keypad to start entering account"));
  Serial.println(F("Commands: 'P' = Print sample, 'S' = SD status, 'L' = List customers, 'DD' = List deductions, 'CT' = List customer types, 'R' = List readings, 'B' = List bills, 'BT' = List transactions, 'DB' = Display all databases (100 rows), 'DB_ALL' = Display all databases (full), 'DROPDB' = Drop and recreate database"));
#endif
  
  // Show welcome screen on TFT
  currentState = STATE_WELCOME;
  showWelcomeScreen();
}

void loop() {
  // ===== KEYPAD INPUT =====
  // char key = keypad.getKey();  // Commented out since keypad is disabled
  // if (key) {
  //   handleKeypadInput(key);
  // }
  
  // ===== SERIAL INPUT =====
  if (Serial.available()) {
    String raw = Serial.readStringUntil('\n');
    raw.trim();

    // Simulate keypad press if single character
    if (raw.length() == 1) {
      char key = raw[0];
      if (isValidKeypadKey(key)) {
        Serial.print(F("Simulating keypad press: "));
        Serial.println(key);
        handleKeypadInput(key);
        return;
      }
    }

    // Check for DROPDB command first (local command, not sync)
    if (raw == "DROPDB") {
      Serial.println(F("Dropping and recreating the database..."));
      if (db) {
        // Drop all tables
        const char* dropTables[] = {
          "DROP TABLE IF EXISTS bill_transactions;",
          "DROP TABLE IF EXISTS bills;",
          "DROP TABLE IF EXISTS readings;",
          "DROP TABLE IF EXISTS customers;",
          "DROP TABLE IF EXISTS customer_types;",
          "DROP TABLE IF EXISTS deductions;",
          "DROP TABLE IF EXISTS barangay_sequence;",
          "DROP TABLE IF EXISTS device_info;",
          NULL
        };
        
        for (int i = 0; dropTables[i] != NULL; i++) {
          sqlite3_exec(db, dropTables[i], NULL, NULL, NULL);
        }
        
        Serial.println(F("All tables dropped."));
        
        // Recreate all tables
        createAllTables();
        initializeDefaultDevice();
        
        // Clear in-memory data
        // customers.clear();  // Removed, no global vector
        readings.clear();
        customerTypes.clear();
        deductions.clear();
        bills.clear();
        
        // Reload all data
        // loadCustomersFromDB();  // Removed, lazy load instead
        loadReadingsFromDB();
        loadCustomerTypesFromDB();
        loadDeductionsFromDB();
        loadBillsFromDB();
        
        Serial.println(F("Database reinitialized."));
      } else {
        Serial.println(F("Database not open."));
      }
      return;
    }

    if (raw == "DROPR") {
      Serial.println(F("Dropping readings table..."));
      if (db) {
        sqlite3_exec(db, "DROP TABLE IF EXISTS readings;", NULL, NULL, NULL);
        readings.clear();
        Serial.println(F("Readings table dropped."));
      } else {
        Serial.println(F("Database not open."));
      }
      return;
    }

    if (raw == "DROPB") {
      Serial.println(F("Dropping bills table..."));
      if (db) {
        sqlite3_exec(db, "DROP TABLE IF EXISTS bills;", NULL, NULL, NULL);
        bills.clear();
        Serial.println(F("Bills table dropped."));
      } else {
        Serial.println(F("Database not open."));
      }
      return;
    }

    if (raw == "DROPBT") {
      Serial.println(F("Dropping bill_transactions table..."));
      if (db) {
        sqlite3_exec(db, "DROP TABLE IF EXISTS bill_transactions;", NULL, NULL, NULL);
        Serial.println(F("Bill transactions table dropped."));
      } else {
        Serial.println(F("Database not open."));
      }
      return;
    }

    if (raw == "DROPC") {
      Serial.println(F("Deleting all customers..."));
      if (db) {
        int rc = sqlite3_exec(db, "DELETE FROM customers;", NULL, NULL, NULL);
        if (rc == SQLITE_OK) {
          Serial.println(F("All customers deleted."));
        } else {
          Serial.print(F("Error deleting customers: "));
          Serial.println(sqlite3_errmsg(db));
        }
      } else {
        Serial.println(F("Database not open."));
      }
      return;
    }

    if (raw == "RESET") {
      Serial.println(F("Resetting sync status for readings, bills, and bill transactions..."));
      if (db) {
        // Reset readings sync status
        int rc1 = sqlite3_exec(db, "UPDATE readings SET synced = 0, last_sync = NULL WHERE synced = 1;", NULL, NULL, NULL);
        if (rc1 == SQLITE_OK) {
          Serial.println(F("Readings sync status reset."));
        } else {
          Serial.print(F("Error resetting readings: "));
          Serial.println(sqlite3_errmsg(db));
        }

        // Reset bills sync status
        int rc2 = sqlite3_exec(db, "UPDATE bills SET synced = 0, last_sync = NULL WHERE synced = 1;", NULL, NULL, NULL);
        if (rc2 == SQLITE_OK) {
          Serial.println(F("Bills sync status reset."));
        } else {
          Serial.print(F("Error resetting bills: "));
          Serial.println(sqlite3_errmsg(db));
        }

        // Reset bill_transactions sync status (batched)
        if (resetBillTransactionsSyncStatusBatched(200)) {
          Serial.println(F("Bill transactions sync status reset."));
        } else {
          Serial.print(F("Error resetting bill transactions: "));
          Serial.println(sqlite3_errmsg(db));
        }

        Serial.println(F("Sync reset complete. All unsynced data will be exported on next sync."));
      } else {
        Serial.println(F("Database not open."));
      }
      return;
    }

    if (raw == "RESET_BILL_TRANSACTION") {
      Serial.println(F("Resetting sync status for bill transactions..."));
      if (db) {
        // Reset bill_transactions sync status (batched)
        if (resetBillTransactionsSyncStatusBatched(200)) {
          Serial.println(F("Bill transactions sync status reset."));
        } else {
          Serial.print(F("Error resetting bill transactions: "));
          Serial.println(sqlite3_errmsg(db));
        }

        Serial.println(F("Bill transactions reset complete. All unsynced transactions will be exported on next sync."));
      } else {
        Serial.println(F("Database not open."));
      }
      return;
    }

    if (handleSyncCommands(raw)) return;

    // ---- Existing console commands ----

    String cmd = raw;
    cmd.toUpperCase();
    
    if (cmd == "P" || cmd == "PRINT") {
      Serial.println(F("Printing sample bill..."));
      displayBillOnTFT();
      printBill();
      Serial.println(F("Print complete."));
    } 
    else if (cmd == "D" || cmd == "DISPLAY") {
      Serial.println(F("Displaying sample bill on TFT..."));
      displayBillOnTFT();
    }
    else if (cmd == "S" || cmd == "SD" || cmd == "SDCARD") {
      checkSDCardStatus();
    }
    else if (cmd == "L" || cmd == "LIST") {
      printCustomersList();
    }
    else if (cmd == "DD" || cmd == "DEDUCTIONS") {
      printDeductionsList();
    }
    else if (cmd == "CT" || cmd == "CUSTOMER_TYPES") {
      printCustomerTypesList();
    }
    else if (cmd == "B" || cmd == "BILLS") {
      printBillsList();
    }
    else if (cmd == "BT" || cmd == "TRANSACTIONS") {
      printBillTransactionsList();
    }
    else if (cmd == "R" || cmd == "READINGS") {
      printReadingsList();
    }
    else if (cmd == "START") {
      Serial.println(F("Starting workflow..."));
      currentState = STATE_ENTER_ACCOUNT;
      inputBuffer = "";
      displayEnterAccountScreen();
    }
    else if (cmd == "DB" || cmd == "DATABASE") {
      Serial.println(F("Displaying all database data (limited to 100 rows per table)..."));
      displayAllDatabaseData();
    }
    else if (cmd == "DB_ALL") {
      Serial.println(F("Displaying all database data (full)..."));
      displayAllDatabaseDataFull();
    }
    else if (cmd == "TIME") {
      String dt = getCurrentDateTimeString();
      Serial.print(F("Current time: "));
      Serial.println(dt);
    }
    else if (raw.startsWith("SET_TIME ")) {
      String payload = raw.substring(String("SET_TIME ").length());
      // Assume format YYYY-MM-DD HH:MM:SS
      int year = payload.substring(0,4).toInt();
      int month = payload.substring(5,7).toInt();
      int day = payload.substring(8,10).toInt();
      int hour = payload.substring(11,13).toInt();
      int minute = payload.substring(14,16).toInt();
      int second = payload.substring(17,19).toInt();
      rtc.adjust(DateTime(year, month, day, hour, minute, second));
      Serial.println(F("RTC time set"));
    }
    else if (cmd.length() > 0) {
      Serial.print(F("Unknown: "));
      Serial.println(cmd);
      Serial.println(F("Commands: P, D, S, L, DD, CT, B, BT, DB, DB_ALL, DROPDB, DROPR, DROPB, DROPBT, DROPC, RESET, RESET_BILL_TRANSACTION, START, TIME, SET_TIME <YYYY-MM-DD HH:MM:SS>"));
    }
  }
}



// ===== TFT DISPLAY FUNCTIONS =====

// ===== THERMAL PRINTER FUNCTIONS =====
