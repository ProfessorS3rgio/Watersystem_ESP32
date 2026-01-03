#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <SD.h>
#include "Adafruit_Thermal.h"
#include <Keypad.h>

// ===== REFACTORED HEADER FILES =====
#include "config.h"
#include "customers_database.h"
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



void setup() {
  Serial.begin(SERIAL_BAUD);
  
  // Initialize TFT Backlight
  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);  // Turn on backlight
  
  // Initialize TFT
  tft.initR(INITR_BLACKTAB);  // For 128x160 ST7735S
  tft.setRotation(1);          // Landscape mode (160x128)
  tft.fillScreen(COLOR_BG);
  
  // Initialize SD Card
  initSDCard();
  
  // Initialize Customers Database
  initCustomersDatabase();
  
  // Initialize Printer
  printerSerial.begin(PRINTER_BAUD, SERIAL_8N1, PRINTER_RX, PRINTER_TX);
  printer.begin();
  
  Serial.println(F("Watersystem ESP32 ready."));
  Serial.println(F("Use keypad or serial commands:"));
  Serial.println(F("Press D or B on keypad to start entering account"));
  Serial.println(F("Commands: 'P' = Print sample, 'S' = SD status, 'L' = List customers"));
  
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
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
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

void showWelcomeScreen() {
  tft.fillScreen(COLOR_BG);
  
  tft.setTextColor(COLOR_HEADER);
  tft.setTextSize(1);
  tft.setCursor(20, 10);
  tft.println(F("DONA JOSEFA M."));
  tft.setCursor(15, 22);
  tft.println(F("BULU-AN CAPARAN"));
  
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(25, 38);
  tft.println(F("Water & Sanitation"));
  tft.setCursor(45, 48);
  tft.println(F("Association"));
  
  tft.drawLine(10, 60, 150, 60, COLOR_LINE);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(30, 70);
  tft.println(F("System Ready"));
  
  tft.setTextColor(ST77XX_MAGENTA);
  tft.setTextSize(1);
  tft.setCursor(20, 95);
  tft.println(F("Press D or B"));
  tft.setCursor(15, 107);
  tft.println(F("to start billing"));
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

  // Calculate values
  unsigned long used = currentBill.currReading - currentBill.prevReading;
  float total = used * currentBill.rate + currentBill.penalty;

  customFeed(1);

  // Logo
  printer.justify('C');
  printer.printBitmap(LOGO_WIDTH, LOGO_HEIGHT, logo);
  customFeed(1);

  // Header
  printer.justify('C');
  printer.setSize('S');
  printer.println(F("DONA JOSEFA M. BULU-AN CAPARAN"));
  printer.println(F("Water & Sanitation Assoc."));
  printer.println(F("Bulu-an, IPIL, Zambo. Sibugay"));
  printer.println(F("TIN: 464-252-005-000"));
  printer.println(F("--------------------------------"));

  // Bill info
  printer.justify('L');
  printer.boldOn();
  printer.println(F("WATER BILL"));
  printer.boldOff();

  printer.print(F("Bill No: "));
  printer.println((unsigned long)(millis() / 1000));
  printer.print(F("Date   : "));
  printer.println(F(__DATE__));

  printer.println(F("--------------------------------"));

  // Customer info
  printer.print(F("Customer : "));
  printer.println(currentBill.customerName);
  printer.print(F("Account  : "));
  printer.println(currentBill.accountNo);
  printer.print(F("Address  : "));
  printer.println(currentBill.address);

  printer.println(F("--------------------------------"));

  // Collector & Due date
  printer.print(F("Collector: "));
  printer.println(currentBill.collector);
  printer.print(F("Penalty  : "));
  if (currentBill.penalty > 0) {
    printer.print(F("PHP "));
    printer.println(currentBill.penalty, 2);
  } else {
    printer.println(F("None"));
  }
  printer.print(F("Due Date : "));
  printer.println(currentBill.dueDate);

  printer.println(F("--------------------------------"));

  // Meter readings
  printer.boldOn();
  printer.println(F("METER READINGS"));
  printer.boldOff();

  printer.print(F("Previous : "));
  printer.println(currentBill.prevReading);
  printer.print(F("Current  : "));
  printer.println(currentBill.currReading);
  printer.print(F("Used     : "));
  printer.print(used);
  printer.println(F(" m3"));

  printer.println(F("--------------------------------"));

  // Billing
  printer.print(F("Rate/m3  : PHP "));
  printer.println(currentBill.rate, 2);

  printer.println(F("================================"));

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

  printer.println(F("================================"));

  // Footer
  printer.justify('C');
  printer.setSize('S');
  printer.println(F(""));
  printer.println(F("Please pay on or before due date"));
  printer.println(F("to avoid penalties."));
  printer.println(F(""));
  printer.println(F("Thank you!"));
  printer.justify('L');

  customFeed(4);
  delay(500);
}

void customFeed(int lines) {
  for (int i = 0; i < lines; i++) {
    printer.write(0x0A);
  }
}
