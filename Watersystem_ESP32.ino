#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <SD.h>
#include "Adafruit_Thermal.h"
#include <Keypad.h>

// ===== REFACTORED HEADER FILES =====
#include "config.h"
#include "customers_database.h"
#include "readings_database.h"
#include "device_info.h"
#include "water_system.h"
#include "print_manager.h"
#include "workflow_manager.h"
#include "sdcard_manager.h"
#include "keypad_manager.h"
#include "logo.h"




// ===== TFT DISPLAY =====
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// ===== THERMAL PRINTER =====
HardwareSerial printerSerial(2);  // Use UART2 on ESP32
Adafruit_Thermal printer(&printerSerial);

// ===== BOOT SCREEN HELPERS =====
static bool checkPrinterCommunication(uint16_t timeoutMs = 250) {
  // Sends a standard ESC/POS status request (DLE EOT 1) and waits briefly for a reply.
  // If the printer is powered and UART wiring is correct, many printers will respond.
  while (printerSerial.available()) {
    printerSerial.read();
  }

  printerSerial.write(0x10);
  printerSerial.write(0x04);
  printerSerial.write(0x01);
  printerSerial.flush();

  unsigned long deadline = millis() + timeoutMs;
  while (millis() < deadline) {
    if (printerSerial.available() > 0) {
      (void)printerSerial.read();
      return true;
    }
    delay(5);
  }
  return false;
}

static void showBootScreen() {
  tft.fillScreen(COLOR_BG);
  tft.setTextSize(1);

  int y = 2;
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(2, y);
  tft.println(F("[BOOT] Watersystem ESP32"));
  y += 12;

  tft.setCursor(2, y);
  tft.println(F("[CHK] SD Card..."));

  initSDCard();
  y += 12;
  tft.setCursor(2, y);
  tft.print(F("[SD ] Status: "));
  if (isSDCardReady()) {
    tft.setTextColor(ST77XX_GREEN);
    tft.println(F("OK"));
  } else {
    tft.setTextColor(ST77XX_RED);
    tft.println(F("FAIL"));
  }
  tft.setTextColor(ST77XX_GREEN);

  y += 12;
  tft.setCursor(2, y);
  tft.println(F("[CFG] Loading rate..."));
  waterRate = loadWaterRate();

  y += 12;
  tft.setCursor(2, y);
  tft.print(F("[CFG] Rate: PHP "));
  tft.println(waterRate, 2);

  y += 12;
  tft.setCursor(2, y);
  tft.println(F("[CHK] Printer..."));

  // Initialize Printer
  printerSerial.begin(PRINTER_BAUD, SERIAL_8N1, PRINTER_RX, PRINTER_TX);
  printer.begin();
  printer.wake();
  printer.setDefault();

  y += 12;
  tft.setCursor(2, y);
  tft.print(F("[PRN] UART: "));
  if (checkPrinterCommunication()) {
    tft.setTextColor(ST77XX_GREEN);
    tft.println(F("OK"));
  } else {
    tft.setTextColor(ST77XX_RED);
    tft.println(F("NO RESP"));
  }
  tft.setTextColor(ST77XX_GREEN);

  y += 12;
  tft.setCursor(2, y);
  tft.println(F("[OK ] Starting UI..."));

  int countdownY = y + 12;
  for (int i = 5; i >= 1; i--) {
    tft.fillRect(0, countdownY, 160, 10, COLOR_BG);
    tft.setTextColor(COLOR_LABEL);
    tft.setCursor(2, countdownY);
    tft.print(F("Booting in "));
    tft.print(i);
    tft.print(F("..."));
    delay(1000);
  }
  tft.fillRect(0, countdownY, 160, 10, COLOR_BG);
  delay(300);
}



void setup() {
  Serial.begin(SERIAL_BAUD);
  
  // Initialize TFT Backlight
  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);  // Turn on backlight
  
  // Initialize TFT
  tft.initR(INITR_BLACKTAB);  // For 128x160 ST7735S
  tft.setRotation(1);          // Landscape mode (160x128)
  tft.fillScreen(COLOR_BG);

  // Ensure shared SPI CS pins are in a safe state before SD init
  pinMode(TFT_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(SD_CS, HIGH);

  // Boot screen: console-style checks (SD + settings + printer)
  showBootScreen();

  // Initialize Readings database (time offset + readings log)
  initReadingsDatabase();

  // Initialize Device Info (about/last sync/print count)
  initDeviceInfo();

  // Initialize Customers Database
  initCustomersDatabase();

#if WS_SERIAL_VERBOSE
  Serial.println(F("Watersystem ESP32 ready."));
  Serial.println(F("Use keypad or serial commands:"));
  Serial.println(F("Press D or B on keypad to start entering account"));
  Serial.println(F("Commands: 'P' = Print sample, 'S' = SD status, 'L' = List customers"));
#endif
  
  // Show welcome screen on TFT
  currentState = STATE_WELCOME;
  showWelcomeScreen();
}

void loop() {
  // ===== KEYPAD INPUT =====
  char key = keypad.getKey();
  if (key) {
    handleKeypadInput(key);
  }
  
  // ===== SERIAL INPUT =====
  if (Serial.available()) {
    String raw = Serial.readStringUntil('\n');
    raw.trim();

    // ---- Sync protocol (do NOT uppercase; payload may be mixed-case) ----
    if (raw == "EXPORT_CUSTOMERS") {
      Serial.println(F("Exporting customers..."));
      exportCustomersForSync();
      return;
    }

    if (raw == "EXPORT_DEVICE_INFO") {
      Serial.println(F("Exporting device info..."));
      exportDeviceInfoForSync();
      return;
    }

    if (raw.startsWith("SET_TIME|")) {
      // SET_TIME|<epoch_seconds>
      String payload = raw.substring(String("SET_TIME|").length());
      payload.trim();
      uint32_t epoch = (uint32_t)payload.toInt();
      if (epoch > 0) {
        setDeviceEpoch(epoch);
        Serial.print(F("ACK|SET_TIME|"));
        Serial.println(epoch);
      } else {
        Serial.println(F("ERR|BAD_TIME"));
      }
      return;
    }

    if (raw.startsWith("SET_LAST_SYNC|")) {
      // SET_LAST_SYNC|<epoch_seconds>
      String payload = raw.substring(String("SET_LAST_SYNC|").length());
      payload.trim();
      uint32_t epoch = (uint32_t)payload.toInt();
      if (epoch > 0) {
        setLastSyncEpoch(epoch);
        Serial.print(F("ACK|SET_LAST_SYNC|"));
        Serial.println(epoch);
      } else {
        Serial.println(F("ERR|BAD_LAST_SYNC"));
      }
      return;
    }

    if (raw == "EXPORT_READINGS") {
      Serial.println(F("Exporting readings..."));
      exportReadingsForSync();
      return;
    }

    if (raw == "READINGS_SYNCED") {
      bool ok = markAllReadingsSynced();
      if (ok) {
        setLastSyncEpoch(deviceEpochNow());
        Serial.println(F("ACK|READINGS_SYNCED"));
      } else {
        Serial.println(F("ERR|READINGS_SYNC_FAILED"));
      }
      return;
    }

    if (raw.startsWith("UPSERT_CUSTOMER|")) {
      // UPSERT_CUSTOMER|account_no|customer_name|address|previous_reading|is_active
      String payload = raw.substring(String("UPSERT_CUSTOMER|").length());

      int p1 = payload.indexOf('|');
      int p2 = (p1 >= 0) ? payload.indexOf('|', p1 + 1) : -1;
      int p3 = (p2 >= 0) ? payload.indexOf('|', p2 + 1) : -1;
      int p4 = (p3 >= 0) ? payload.indexOf('|', p3 + 1) : -1;
      if (p1 < 0 || p2 < 0 || p3 < 0 || p4 < 0) {
        Serial.println(F("ERR|BAD_FORMAT"));
        return;
      }

      String accountNo = payload.substring(0, p1);
      String name = payload.substring(p1 + 1, p2);
      String address = payload.substring(p2 + 1, p3);
      String prevStr = payload.substring(p3 + 1, p4);
      String activeStr = payload.substring(p4 + 1);

      accountNo.trim();
      name.trim();
      address.trim();
      prevStr.trim();
      activeStr.trim();

      unsigned long prev = (unsigned long)prevStr.toInt();
      bool active = (activeStr == "1" || activeStr == "true" || activeStr == "TRUE" || activeStr == "Y" || activeStr == "y");

      if (upsertCustomerFromSync(accountNo, name, address, prev, active)) {
        Serial.print(F("ACK|UPSERT|"));
        Serial.println(accountNo);
      } else {
        Serial.println(F("ERR|UPSERT_FAILED"));
      }
      return;
    }

    if (raw.startsWith("SET_WATER_RATE|")) {
      // SET_WATER_RATE|<rate>
      String payload = raw.substring(String("SET_WATER_RATE|").length());
      payload.trim();
      float rate = payload.toFloat();
      if (rate > 0) {
        saveWaterRate(rate);
        Serial.print(F("ACK|SET_WATER_RATE|"));
        Serial.println(rate, 2);
      } else {
        Serial.println(F("ERR|BAD_RATE"));
      }
      return;
    }

    if (raw == "RELOAD_SD") {
      Serial.println(F("Reloading SD card and settings..."));
      initSDCard();
      waterRate = loadWaterRate();
      Serial.print(F("New water rate: PHP "));
      Serial.println(waterRate, 2);
      Serial.println(F("SD reloaded successfully"));
      return;
    }

    if (raw.startsWith("REMOVE_CUSTOMER|")) {
      // REMOVE_CUSTOMER|<account_no>
      String payload = raw.substring(String("REMOVE_CUSTOMER|").length());
      payload.trim();
      if (payload.length() > 0) {
        if (removeCustomerByAccount(payload)) {
          Serial.print(F("ACK|REMOVE_CUSTOMER|"));
          Serial.println(payload);
        } else {
          Serial.println(F("ERR|CUSTOMER_NOT_FOUND"));
        }
      } else {
        Serial.println(F("ERR|BAD_ACCOUNT_NO"));
      }
      return;
    }

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
    else if (cmd == "START") {
      Serial.println(F("Starting workflow..."));
      currentState = STATE_ENTER_ACCOUNT;
      inputBuffer = "";
      displayEnterAccountScreen();
    }
    else if (cmd.length() > 0) {
      Serial.print(F("Unknown: "));
      Serial.println(cmd);
      Serial.println(F("Commands: P, D, S, L, START"));
    }
  }
}



// ===== TFT DISPLAY FUNCTIONS =====

// Function to draw BMP image from SD card
void drawBMP(const char *filename, int16_t x, int16_t y) {
  File bmpFile;
  int bmpWidth, bmpHeight;
  uint8_t bmpDepth;
  uint32_t bmpImageoffset;
  uint32_t rowSize;
  boolean goodBmp = false;
  boolean flip = true;
  int w, h, row, col;
  uint8_t r, g, b;
  uint32_t pos = 0;

  if ((x >= tft.width()) || (y >= tft.height())) return;

  bmpFile = SD.open(filename);
  if (!bmpFile) {
    Serial.print(F("BMP not found: "));
    Serial.println(filename);
    return;
  }

  // Parse BMP header
  if (read16(bmpFile) == 0x4D42) { // BMP signature
    (void)read32(bmpFile); // File size
    (void)read32(bmpFile); // Creator bytes
    bmpImageoffset = read32(bmpFile);
    (void)read32(bmpFile); // Header size
    bmpWidth = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    
    if (read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile);
      if ((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed
        goodBmp = true;

        rowSize = (bmpWidth * 3 + 3) & ~3;
        if (bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip = false;
        }

        w = bmpWidth;
        h = bmpHeight;
        if ((x + w - 1) >= tft.width()) w = tft.width() - x;
        if ((y + h - 1) >= tft.height()) h = tft.height() - y;

        for (row = 0; row < h; row++) {
          if (flip)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else
            pos = bmpImageoffset + row * rowSize;
          
          if (bmpFile.position() != pos) {
            bmpFile.seek(pos);
          }

          for (col = 0; col < w; col++) {
            b = bmpFile.read();
            g = bmpFile.read();
            r = bmpFile.read();
            tft.drawPixel(x + col, y + row, tft.color565(r, g, b));
          }
        }
      }
    }
  }

  bmpFile.close();
  if (!goodBmp) Serial.println(F("BMP format not recognized."));
}

uint16_t read16(File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read();
  ((uint8_t *)&result)[1] = f.read();
  return result;
}

uint32_t read32(File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read();
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read();
  return result;
}

void drawBattery(int x, int y, int level) {
  // Clamp level
  level = constrain(level, 0, 100);

  // Colors by level
  uint16_t color =
    (level > 60) ? ST77XX_GREEN :
    (level > 30) ? ST77XX_YELLOW :
                   ST77XX_RED;

  // Dimensions
  int w = 34;
  int h = 14;
  int tipW = 3;
  int radius = 3;

  // Outline
  tft.drawRoundRect(x, y, w, h, radius, ST77XX_WHITE);
  tft.fillRoundRect(x + w, y + 4, tipW, h - 8, 2, ST77XX_WHITE);

  // Fill
  int fillW = map(level, 0, 100, 0, w - 4);
  tft.fillRoundRect(x + 2, y + 2, fillW, h - 4, radius - 1, color);

  // Percentage (optional, small & subtle)
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(x + w + tipW + 6, y + 3);
  tft.print(level);
  tft.print('%');
}

void showWelcomeScreen() {
  tft.fillScreen(COLOR_BG);
  
  // ===== TOP BORDER =====
  tft.fillRect(0, 0, 160, 3, COLOR_HEADER);
  
  // ===== DISPLAY LOGO FROM SD CARD =====
  // Draw logo from BMP file on the left side
  int logoX = 10;  // Left side
  int logoY = 15;
  drawBMP("/WATER_DB/ASSETS/water_logo3.bmp", logoX, logoY);
  
  // ===== SERVICE DESCRIPTION (Next to logo) =====
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(1);
  tft.setCursor(80, 20);
  tft.println(F("Water &"));
  tft.setCursor(80, 30);
  tft.println(F("Sanitation"));
  tft.setCursor(80, 40);
  tft.println(F("Association"));
  
  // ===== DECORATIVE SEPARATOR =====
  // tft.fillRect(20, 105, 120, 1, COLOR_LINE);
  // tft.fillRect(30, 107, 100, 1, COLOR_LINE);
  
  // ===== STATUS INDICATOR =====
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(44, 70);  // Centered
  tft.println(F("System Ready"));
  
  // ===== BATTERY INDICATOR =====
  drawBattery(50, 88, 100);  // x, y, battery level (0-100)
  
  // ===== INSTRUCTIONS =====
  tft.setTextColor(ST77XX_MAGENTA);
  tft.setTextSize(1);
  tft.setCursor(20, 115);  // Moved to bottom
  tft.println(F("A-Menu  D/B-Billing"));
  
  // ===== BOTTOM BORDER =====
  tft.fillRect(0, 125, 160, 3, COLOR_HEADER);
}

void displayBillOnTFT() {
  tft.fillScreen(COLOR_BG);
  
  // Calculate used and total
  unsigned long used = currentBill.currReading - currentBill.prevReading;
  float total = used * currentBill.rate + currentBill.penalty;
  
  // Header
  tft.setTextColor(COLOR_HEADER);
  tft.setTextSize(1);
  tft.setCursor(35, 5);
  tft.println(F("WATER BILL"));
  
  tft.drawLine(0, 16, 160, 16, COLOR_LINE);
  
  // Customer info
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 22);
  tft.print(F("Name: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(currentBill.customerName);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 34);
  tft.print(F("Acct: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(currentBill.accountNo);
  
  tft.drawLine(0, 46, 160, 46, COLOR_LINE);
  
  // Meter readings
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 52);
  tft.print(F("Prev: "));
  tft.setTextColor(COLOR_TEXT);
  tft.print(currentBill.prevReading);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(85, 52);
  tft.print(F("Cur: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(currentBill.currReading);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 64);
  tft.print(F("Used: "));
  tft.setTextColor(COLOR_AMOUNT);
  tft.print(used);
  tft.println(F(" m3"));
  
  tft.drawLine(0, 76, 160, 76, COLOR_LINE);
  
  // Due date
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 82);
  tft.print(F("Due: "));
  tft.setTextColor(ST77XX_RED);
  tft.println(currentBill.dueDate);
  
  tft.drawLine(0, 94, 160, 94, COLOR_LINE);
  
  // Total amount - big and prominent
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(25, 100);
  tft.println(F("TOTAL DUE:"));
  
  tft.setTextColor(COLOR_AMOUNT);
  tft.setTextSize(2);
  tft.setCursor(20, 112);
  tft.print(F("P"));
  tft.println(total, 2);
  tft.setTextSize(1);
}

// ===== THERMAL PRINTER FUNCTIONS =====

void printBill() {
  printer.wake();
  printer.setDefault();
  YIELD_WDT();

  unsigned long used = currentBill.currReading - currentBill.prevReading;
  float total = used * currentBill.rate + currentBill.penalty;

  customFeed(1);
  YIELD_WDT();

  printer.justify('C');
  printer.printBitmap(LOGO_WIDTH, LOGO_HEIGHT, logo);
  YIELD_WDT();  // 🚨 REQUIRED

  customFeed(1);
  YIELD_WDT();

  printer.setSize('S');
  printer.println(F("DONA JOSEFA M. BULU-AN CAPARAN"));
  printer.println(F("Water & Sanitation Assoc."));
  printer.println(F("Bulu-an, IPIL, Zambo. Sibugay"));
  printer.println(F("TIN: 464-252-005-000"));
  printer.println(F("--------------------------------"));
  YIELD_WDT();

  printer.justify('L');
  printer.boldOn();
  printer.println(F("WATER BILL"));
  printer.boldOff();
  YIELD_WDT();

  printer.print(F("Bill No: "));
  printer.println((unsigned long)(millis() / 1000));
  printer.print(F("Date   : "));
  printer.println(F(__DATE__));
  YIELD_WDT();

  printer.println(F("--------------------------------"));
  YIELD_WDT();

  printer.print(F("Customer : "));
  printer.println(currentBill.customerName);
  printer.print(F("Account  : "));
  printer.println(currentBill.accountNo);
  printer.print(F("Address  : "));
  printer.println(currentBill.address);
  YIELD_WDT();

  printer.println(F("--------------------------------"));
  YIELD_WDT();

  printer.print(F("Collector: "));
  printer.println(currentBill.collector);
  printer.print(F("Penalty  : "));
  if (currentBill.penalty > 0) {
    printer.print(F("PHP "));
    printer.println(currentBill.penalty, 2);
  } else {
    printer.println(F("None"));
  }
  YIELD_WDT();

  printer.print(F("Due Date : "));
  printer.println(currentBill.dueDate);
  printer.println(F("--------------------------------"));
  YIELD_WDT();

  printer.boldOn();
  printer.println(F("METER READINGS"));
  printer.boldOff();
  YIELD_WDT();

  printer.print(F("Previous : "));
  printer.println(currentBill.prevReading);
  printer.print(F("Current  : "));
  printer.println(currentBill.currReading);
  printer.print(F("Used     : "));
  printer.print(used);
  printer.println(F(" m3"));
  YIELD_WDT();

  printer.println(F("--------------------------------"));
  YIELD_WDT();

  printer.print(F("Rate/m3  : PHP "));
  printer.println(currentBill.rate, 2);
  printer.println(F("================================"));
  YIELD_WDT();

  printer.justify('C');
  printer.setSize('M');
  printer.boldOn();
  printer.println(F("*** TOTAL AMOUNT DUE ***"));
  printer.setSize('L');
  printer.print(F("PHP "));
  printer.println(total, 2);
  printer.boldOff();
  printer.setSize('S');
  printer.justify('L');
  YIELD_WDT();

  printer.println(F("================================"));
  YIELD_WDT();

  printer.justify('C');
  printer.println(F(""));
  printer.println(F("Please pay on or before due date"));
  printer.println(F("to avoid penalties."));
  printer.println(F(""));
  printer.println(F("Thank you!"));
  printer.justify('L');
  YIELD_WDT();

  customFeed(4);
  YIELD_WDT();

  vTaskDelay(pdMS_TO_TICKS(500)); // ✅ replace delay()
}

void customFeed(int lines) {
  for (int i = 0; i < lines; i++) {
    printer.write(0x0A);
  }
}
