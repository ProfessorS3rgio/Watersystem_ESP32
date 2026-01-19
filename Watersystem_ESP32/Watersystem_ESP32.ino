#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <SD.h>
#include "Adafruit_Thermal.h"
#include <Keypad.h>
#include <time.h>

// ===== REFACTORED HEADER FILES =====
#include "configuration/config.h"
#include "database/customers_database.h"
#include "database/readings_database.h"
#include "database/device_info.h"
#include "database/bill_database.h"
#include "database/deduction_database.h"
#include "database/customer_type_database.h"
#include "database/database_viewer.h"
#include "managers/print_manager.h"
#include "managers/tft_screen_manager.h"
#include "managers/sdcard_manager.h"
#include "managers/keypad_manager.h"
#include "configuration/logo.h"
#include "managers/sync_manager.h"
#include "components/battery_display.h"
#include "components/bmp_display.h"




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

  // Initialize Deductions Database
  initDeductionsDatabase();

  // Initialize Customer Types Database
  initCustomerTypesDatabase();

  // Initialize Bills Database
  initBillsDatabase();

#if WS_SERIAL_VERBOSE
  Serial.println(F("Watersystem ESP32 ready."));
  Serial.println(F("Use keypad or serial commands:"));
  Serial.println(F("Press D or B on keypad to start entering account"));
  Serial.println(F("Commands: 'P' = Print sample, 'S' = SD status, 'L' = List customers, 'DD' = List deductions, 'CT' = List customer types, 'B' = List bills"));
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
    else if (cmd == "START") {
      Serial.println(F("Starting workflow..."));
      currentState = STATE_ENTER_ACCOUNT;
      inputBuffer = "";
      displayEnterAccountScreen();
    }
    else if (cmd == "DB" || cmd == "DATABASE") {
      Serial.println(F("Displaying all database data..."));
      displayAllDatabaseData();
    }
    else if (cmd.length() > 0) {
      Serial.print(F("Unknown: "));
      Serial.println(cmd);
      Serial.println(F("Commands: P, D, S, L, DD, CT, B, DB, START"));
    }
  }
}



// ===== TFT DISPLAY FUNCTIONS =====

// ===== THERMAL PRINTER FUNCTIONS =====

void printBill() {
  printer.wake();
  printer.setDefault();
  YIELD_WDT();

  unsigned long used = currentBill.currReading - currentBill.prevReading;
  float total = currentBill.total;

  customFeed(1);
  YIELD_WDT();

  printer.justify('C');
  // printer.printBitmap(LOGO_WIDTH, LOGO_HEIGHT, logo);
  // YIELD_WDT();  // 🚨 REQUIRED

  // customFeed(1);
  printer.println(F("[PLACEHOLDER]"));
  printer.println(F(""));
  YIELD_WDT();

  printer.setSize('S');
  printer.println(F("DONA JOSEFA M. BULU-AN CAPARAN"));
  printer.println(F("Water & Sanitation Assoc."));
  printer.println(F("Bulu-an, IPIL, Zambo. Sibugay"));
  printer.println(F("TIN: 464-252-005-000"));
  printer.println(F(""));
  printer.println(F("--------------------------------"));
  printer.boldOn();
  printer.println(F("STATEMENT OF ACCOUNT"));
  printer.boldOff();
  printer.println(F("--------------------------------"));
  YIELD_WDT();

  printer.justify('L');
  // Ref No and Date/Time
  printer.print(F("Ref No       : "));
  printer.println(F("REF123456"));  // Placeholder, can be dynamic
  printer.print(F("Date/Time    : "));
  printer.println(currentBill.readingDateTime);
  printer.println(F("--------------------------------"));
  YIELD_WDT();

  // Customer info
  printer.print(F("Customer : "));
  printer.println(currentBill.customerName);
  printer.print(F("Account  : "));
  printer.println(currentBill.accountNo);
  printer.print(F("Classification: "));
  printer.println(currentBill.customerType);
  printer.print(F("Address  : "));
  printer.println(currentBill.address);
  printer.print(F("Barangay : "));
  printer.println(F("Makilas"));
  printer.println(F("--------------------------------"));
  YIELD_WDT();

  // Collector
  printer.justify('C');
  printer.println(F("Collector"));
  printer.println(currentBill.collector);
  printer.println(F(""));
  YIELD_WDT();
  printer.println(F("--------------------------------"));
  YIELD_WDT();

  // Period covered
  printer.justify('C');
  printer.boldOn();
  printer.println(F("Period Covered"));
  printer.boldOff();
  printer.println(getPeriodCovered());
  printer.println(F(""));
  YIELD_WDT();

  // Meter readings
  printer.justify('C');
  printer.boldOn();
  printer.println(F("METER READINGS"));
  printer.boldOff();
  // Replicate Python alignment: center in fields of 10,12,10
  String prevStr = String(currentBill.prevReading);
  String presStr = String(currentBill.currReading);
  String usageStr = String(used);
  String header_cols = centerString("Prev", 10) + centerString("Present", 12) + centerString("Usage", 10);
  String values_cols = centerString(prevStr, 10) + centerString(presStr, 12) + centerString(usageStr, 10);
  printer.println(header_cols);
  printer.println(values_cols);
  printer.println(F("--------------------------------"));
  YIELD_WDT();
// %10lu%8lu%7lu
  // Rate , deduction & Penalty
  printer.justify('C');
  printer.boldOn();
  printer.println(F("BILLING"));
  printer.boldOff();
  printer.println(F(""));
  YIELD_WDT();
  printer.justify('L');
  printer.print(F("Rate/m3  : PHP "));
  printer.println(currentBill.rate, 2);
  printer.print(F("Water Charge : PHP "));
  printer.println(currentBill.subtotal, 2);
  if (currentBill.deductions > 0) {
    printer.print(currentBill.deductionName);
    printer.print(F(" : PHP -"));
    printer.println(currentBill.deductions, 2);
  }
  if (currentBill.penalty > 0) {
    printer.print(F("Penalty  : PHP "));
    printer.println(currentBill.penalty, 2);
  }
  printer.println(F(""));
  printer.println(F("================================"));
  YIELD_WDT();

  // Total
  printer.justify('C');
  printer.setSize('M');
  printer.boldOn();
  printer.println(F("*** TOTAL AMOUNT DUE ***"));
  printer.setSize('L');
  printer.print(F("PHP "));
  printer.println(total, 2);
  printer.boldOff();
  printer.setSize('S');
  printer.println(F("================================"));
  YIELD_WDT();

  // Due and Disconnect dates
  printer.println(F(""));
  printer.justify('L');
  printer.boldOn();
  printer.print(F("Due Date     : "));
  printer.println(currentBill.dueDate);
  printer.print(F("Disconnection Date: "));
  printer.println(F("Feb 10, 2026"));  // Placeholder
  printer.boldOff();
  printer.println(F("--------------------------------"));
  YIELD_WDT();

  // Reminders
  printer.println(F(""));
  printer.println(F("REMAINDERS:"));
  printer.println(F("- Please pay by the due date to avoid disconnection."));
  printer.println(F("- Bring this bill when paying."));
  printer.println(F("- For inquiries, contact the office."));
 
  YIELD_WDT();

  // Footer
  printer.justify('C');
  printer.println(F(""));
  printer.println(F("Please pay on or before due date"));
  printer.println(F("to avoid penalties."));
  printer.println(F(""));
  printer.println(F("Thank you!"));
  printer.println(F("Save Water, Save Life!"));
  printer.justify('L');
  YIELD_WDT();

  customFeed(4);
  YIELD_WDT();

  vTaskDelay(pdMS_TO_TICKS(500)); // ✅ replace delay()
}

String getPeriodCovered() {
  String dateStr = __DATE__;
  String monthStr = dateStr.substring(0,3);
  String yearStr = dateStr.substring(7,11); // Full year
  String months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  int monthIndex = -1;
  for(int i = 0; i < 12; i++) {
    if(months[i] == monthStr) {
      monthIndex = i;
      break;
    }
  }
  if(monthIndex == -1) return "Unknown Period";
  int nextMonthIndex = (monthIndex + 1) % 12;
  String nextMonthStr = months[nextMonthIndex];
  String nextYearStr = (nextMonthIndex == 0) ? String(yearStr.toInt() + 1) : yearStr;
  return monthStr + " " + yearStr + " - " + nextMonthStr + " " + nextYearStr;
}
void customFeed(int lines) {
  for (int i = 0; i < lines; i++) {
    printer.write(0x0A);
  }
}

String centerString(String s, int width) {
  int len = s.length();
  int totalPad = width - len;
  int leftPad = totalPad / 2;
  int rightPad = totalPad - leftPad;
  String result = "";
  for (int i = 0; i < leftPad; i++) result += " ";
  result += s;
  for (int i = 0; i < rightPad; i++) result += " ";
  return result;
}
