// ===== MAIN WATERSYSTEM ESP32 SKETCH =====
#include <SPI.h>
SPIClass SPI_SD(VSPI);
#include <SD.h>

// supply the UART pins to the wrapper (mirrors sample code names)
#define RXD2 PRINTER_RX
#define TXD2 PRINTER_TX
#include <Adafruit_MCP23X17.h>
#include <time.h>
#include <Wire.h>
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h>
#include <RTClib.h>
#include <esp_heap_caps.h>
#include <esp_sleep.h>

// TFT_eSPI configuration (must be before #include <TFT_eSPI.h>)
#define ILI9341_DRIVER
#define TFT_CS    15
#define TFT_DC    2
#define TFT_RST   -1
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
#include "managers/sync_manager.h"
#include "components/battery_display.h"
#include "components/bmp_display.h"
#include "printer/bill_printer.h"
#include "printer/ble_printer_test.h"
#include "printer/receipt_printer.h"
#include "screens/boot_screen.h"
#include "configuration/power_save_manager.h"
#include "bluetooth/ble_slave_manager.h"




// ===== TFT DISPLAY =====
TFT_eSPI tft = TFT_eSPI();


// ===== RTC MODULE =====
RTC_DS3231 rtc;

// ===== MCP23017 I/O EXPANDER =====
Adafruit_MCP23X17 mcp;
bool g_mcpReady = false;

// ===== BATTERY MONITOR (MAX17043) =====
SFE_MAX1704X g_lipo;
bool g_lipoDetected = false;
int g_batteryPercent = 0;
bool g_batteryCharging = false;

constexpr bool BLE_SLAVE_LINK_ENABLED = true;

// ===== KEYPAD SIMULATION HELPER =====
bool isValidKeypadKey(char key) {
  const char validKeys[] = {'1','2','3','A','4','5','6','B','7','8','9','C','*','0','#','D'};
  for (char k : validKeys) {
    if (k == key) return true;
  }
  return false;
}

bool isPowerControlKey(char key) {
  return key == 'D';
}

bool isIdleWorkflowState() {
  return currentState == STATE_WELCOME
      || currentState == STATE_MENU
      || currentState == STATE_ABOUT
  || currentState == STATE_PRINTER_STATUS
      || currentState == STATE_VIEW_RATE;
}

String readSerialCommand() {
  static String serialBuffer;

  while (Serial.available()) {
    char ch = static_cast<char>(Serial.read());

    if (ch == '\r' || ch == '\n') {
      if (serialBuffer.length() == 0) {
        continue;
      }

      String cmd = serialBuffer;
      serialBuffer = "";
      cmd.trim();
      return cmd;
    }

    serialBuffer += ch;
  }

  return "";
}

void setup() {
  const bool rxBufferConfigured = Serial.setRxBufferSize(8192);
  Serial.begin(SERIAL_BAUD);
  if (rxBufferConfigured) {
    Serial.println(F("[SYNC] Serial RX buffer set to 8192"));
  } else {
    Serial.println(F("[SYNC] Warning: failed to set Serial RX buffer to 8192"));
  }
  Serial.setTimeout(30000); // 30 seconds timeout for long transmissions

  if (BLE_SLAVE_LINK_ENABLED) {
    Serial.println(F("[BLE] Slave link set to lazy init for printing"));
  } else {
    Serial.println(F("[BLE] Slave link disabled for heap comparison"));
  }
  
  // Initialize I2C for RTC and MAX17043
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

  if (!g_lipo.begin()) {
    g_lipoDetected = false;
    Serial.printf("MAX17043 not detected. Check I2C wiring (SDA=%d, SCL=%d) or power.\n", RTC_SDA, RTC_SCL);
  } else {
    g_lipoDetected = true;
    Serial.println(F("MAX17043 detected"));
    g_lipo.quickStart();
    g_lipo.setThreshold(20); // alert at 20% SOC
  }
  
  // Initialize MCP23017 (used for keypad, etc.)
  g_mcpReady = mcp.begin_I2C(MCP23017_ADDR);
  if (!g_mcpReady) {
    Serial.println(F("Error initializing MCP23017 - continuing without it"));
    // don't block; system can still run in reduced mode
  } else {
    Serial.println(F("MCP23017 initialized"));
    mcp.pinMode(CHARGING_PIN_MCP, INPUT);  // GPB0 for charging state
  }
  
  // Initialize TFT Backlight with PWM
  ledcAttach(TFT_BLK, 5000, 8);  // pin, frequency, resolution
  ledcWrite(TFT_BLK, 255);       // full brightness
  
  // Initialize SPI (SCK, MISO, MOSI)
  SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI);
  
  // Initialize TFT
  tft.begin();  // For 320x240 ILI9341
  tft.setRotation(3);          // Landscape mode (flipped, 240x320)
    // tft.setRotation(1);          // Landscape mode (240x320)
  tft.fillScreen(COLOR_BG);

  // Ensure shared SPI CS pins are in a safe state before SD init
  pinMode(TFT_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(SD_CS, HIGH);

  // Boot screen: console-style checks (SD + settings + printer)
  // show immediately after TFT is ready so we can see early errors
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
  Serial.println(F("Commands: 'P' = Print sample, 'S' = SD status, 'L' = List customers, 'DD' = List deductions, 'CT' = List customer types, 'R' = List readings, 'B' = List bills, 'BT' = List transactions, 'DB' = Display all databases (100 rows), 'DB_ALL' = Display all databases (full), 'DROPDB' = Drop and recreate database, 'HEAP' = Show memory stats"));
#endif
  
  // Show welcome screen on TFT
  currentState = STATE_WELCOME;
  showWelcomeScreen();

  powerSaveBegin(TFT_BLK, POWER_SAVE_TIMEOUT, 25, 255);
}

void loop() {
  const bool allowPowerSave = isIdleWorkflowState() && !bleIsBusyForPrint();
  powerSaveSetEnabled(allowPowerSave);

  // ===== BATTERY MONITORING =====
  static unsigned long lastMeasure = 0;
  static int lastChargingState = -1;
  if (millis() - lastMeasure > 1000) {  // Measure every 1 second
    if (g_lipoDetected) {
      g_batteryPercent = static_cast<int>(g_lipo.getSOC() + 0.5f);
      if (g_batteryPercent < 0) g_batteryPercent = 0;
      if (g_batteryPercent > 100) g_batteryPercent = 100;
    } else {
      g_batteryPercent = 0;
    }

    const bool chargingNow = g_mcpReady
      ? (mcp.digitalRead(CHARGING_PIN_MCP) == LOW)
      : false;
    g_batteryCharging = chargingNow;

    if (lastChargingState != (chargingNow ? 1 : 0)) {
      lastChargingState = chargingNow ? 1 : 0;

      Serial.print(F("[CHG] State changed: "));
      Serial.println(chargingNow ? F("CHARGING") : F("NOT CHARGING"));

      if (g_mcpReady) {
        const int rawLevel = mcp.digitalRead(CHARGING_PIN_MCP);
        Serial.print(F("[CHG] MCP pin "));
        Serial.print(CHARGING_PIN_MCP);
        Serial.print(F(" raw level: "));
        Serial.println(rawLevel ? F("HIGH") : F("LOW"));
      } else {
        Serial.println(F("[CHG] MCP not ready"));
      }
    }

    if (currentState == STATE_WELCOME) {
      updateWelcomeBatteryStatus(g_batteryPercent, g_batteryCharging);
    }

    lastMeasure = millis();
  }
  
  // ===== KEYPAD INPUT =====
  char key = getKey();
  if (key != '\0') {
    if (!powerSaveConsumeWakeKey(key)) {
      return;
    }
    powerSaveNotifyActivity("keypad");
    handleKeypadInput(key);
  }
  
  // ===== SERIAL INPUT =====
  String raw = readSerialCommand();
  if (raw.length() > 0) {
    powerSaveWakeFromSerial();
    powerSaveNotifyActivity("serial-command");

    // Simulate keypad press if single character
    if (raw.length() == 1) {
      char key = raw[0];
      if (isValidKeypadKey(key)) {
        Serial.print(F("Simulating keypad press: "));
        Serial.println(key);
        if (!powerSaveConsumeWakeKey(key)) {
          return;
        }
        powerSaveNotifyActivity("serial-keypad");
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
          "DROP TABLE IF EXISTS bill_reference_sequence;",
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

    if (raw.startsWith("PURGE_DATE ")) {
      String payload = raw.substring(String("PURGE_DATE ").length());
      payload.trim();
      if (payload.length() != 10) {
        Serial.println(F("Usage: PURGE_DATE YYYY-MM-DD"));
        return;
      }

      int year = payload.substring(0, 4).toInt();
      int month = payload.substring(5, 7).toInt();
      int day = payload.substring(8, 10).toInt();
      if (year < 2000 || month < 1 || month > 12 || day < 1 || day > 31) {
        Serial.println(F("Invalid date."));
        return;
      }

      int deletedBills = 0;
      int deletedTransactions = 0;
      int deletedReadings = 0;
      if (purgeBillsAndTransactionsByCreatedDate(payload.c_str(), deletedBills, deletedTransactions, deletedReadings)) {
        Serial.print(F("Purged bills: "));
        Serial.print(deletedBills);
        Serial.print(F(", readings: "));
        Serial.print(deletedReadings);
        Serial.print(F(", bill_transactions: "));
        Serial.println(deletedTransactions);
      } else {
        Serial.println(F("PURGE_DATE failed."));
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
      Serial.println(F("Printing sample bill through BLE slave..."));
      prepareSampleBillForBlePrintTest();
      displayBillOnTFT();
      if (printSampleBillViaBleSlave()) {
        Serial.println(F("Print complete."));
      } else {
        Serial.println(F("Print skipped. BLE slave is not ready."));
      }
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
    else if (cmd == "HEAP" || cmd == "MEM") {
      Serial.println(F("=== HEAP MEMORY STATUS ==="));
      Serial.print(F("Free heap        : "));
      Serial.println(ESP.getFreeHeap());
      Serial.print(F("Min free heap    : "));
      Serial.println(ESP.getMinFreeHeap());
      Serial.print(F("Max alloc heap   : "));
      Serial.println(ESP.getMaxAllocHeap());
      Serial.print(F("Internal free    : "));
      Serial.println(heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
      Serial.print(F("Internal largest : "));
      Serial.println(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
      Serial.print(F("8-bit free heap  : "));
      Serial.println(heap_caps_get_free_size(MALLOC_CAP_8BIT));
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
    else if (cmd.startsWith("NORMALIZE_DATES ") || cmd.startsWith("NORMALITE_DATES ")) {
      String payload = cmd.startsWith("NORMALIZE_DATES ")
        ? cmd.substring(String("NORMALIZE_DATES ").length())
        : cmd.substring(String("NORMALITE_DATES ").length());
      payload.trim();
      if (payload.length() != 10) {
        Serial.println(F("Usage: NORMALIZE_DATES YYYY-MM-DD"));
      } else if (normalizeAllReadingAndBillDatesToDate(payload.c_str())) {
        Serial.println(F("Date normalize done."));
      } else {
        Serial.println(F("Date normalize failed."));
      }
    }
    else if (cmd.length() > 0) {
      Serial.print(F("Unknown: "));
      Serial.println(cmd);
      Serial.println(F("Commands: P, D, S, L, DD, CT, B, BT, DB, DB_ALL, DROPDB, DROPR, DROPB, DROPBT, DROPC, RESET, RESET_BILL_TRANSACTION, START, TIME, HEAP, SET_TIME <YYYY-MM-DD HH:MM:SS>, NORMALIZE_DATES <YYYY-MM-DD>, PURGE_DATE <YYYY-MM-DD>"));
    }
  }
  
}



// ===== TFT DISPLAY FUNCTIONS =====

// ===== THERMAL PRINTER FUNCTIONS =====
