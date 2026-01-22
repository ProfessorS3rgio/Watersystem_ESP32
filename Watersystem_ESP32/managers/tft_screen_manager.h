#ifndef TFT_SCREEN_MANAGER_H
#define TFT_SCREEN_MANAGER_H

#include "../configuration/config.h"
#include "../database/customers_database.h"
#include "../database/bill_database.h"
#include "components/bmp_display.h"
#include "components/battery_display.h"
#include <SD.h>

// ===== EXTERNAL OBJECTS FROM MAIN .INO =====
extern Adafruit_ST7735 tft;

// ===== EXTERNAL FROM CUSTOMERS DATABASE =====
extern Customer* currentCustomer;

// ===== FORWARD DECLARATIONS =====
void showWelcomeScreen();
void displayBillOnTFT();
void printBill();

// ===== WORKFLOW STATES =====
enum WorkflowState {
  STATE_WELCOME,           // Initial welcome screen
  STATE_MENU,              // Menu screen
  STATE_ABOUT,             // About screen
  STATE_ENTER_ACCOUNT,     // Waiting for account number entry
  STATE_ACCOUNT_FOUND,     // Account found, showing customer info
  STATE_READING_ALREADY_DONE, // Reading already done this month
  STATE_ENTER_READING,     // Waiting for current meter reading entry
  STATE_BILL_CALCULATED,   // Bill calculated, ready to print
  STATE_PRINTING,          // Printing bill
  STATE_VIEW_RATE          // Viewing current rate
};

WorkflowState currentState = STATE_WELCOME;
String inputBuffer = "";          // Buffer for numeric input
int selectedCustomerIndex = -1;   // Index of selected customer
unsigned long currentReading = 0; // Current meter reading from keypad

// ===== DISPLAY FUNCTIONS FOR WORKFLOW =====

void displayEnterAccountScreen() {
  tft.fillScreen(COLOR_BG);
  
  tft.setTextColor(COLOR_HEADER);
  tft.setTextSize(1);
  tft.setCursor(20, 10);
  tft.println(F("ENTER ACCOUNT #"));
  
  tft.drawLine(0, 20, 160, 20, COLOR_LINE);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(5, 35);
  tft.println(F("Account Number:"));
  
  // Input box
  tft.drawRect(10, 50, 140, 30, COLOR_LINE);
  
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setCursor(20, 58);
  if (inputBuffer.length() > 0) {
    tft.println(inputBuffer);
  } else {
    // Placeholder
    tft.setTextColor(COLOR_LABEL);
    tft.println(F("___"));
  }
  
  tft.setTextSize(1);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(10, 100);
  tft.println(F("D-Confirm B-Clear"));
  tft.setCursor(32, 112);
  tft.println(F("C-Cancel"));
}

void displayMenuScreen() {
  tft.fillScreen(COLOR_BG);
  
  tft.setTextColor(COLOR_HEADER);
  tft.setTextSize(1);
  tft.setCursor(55, 5);
  tft.println(F("MENU"));
  
  tft.drawLine(0, 16, 160, 16, COLOR_LINE);
  
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(10, 25);
  tft.println(F("1. About"));
  tft.setCursor(10, 40);
  tft.println(F("2. Update"));
  tft.setCursor(10, 55);
  tft.println(F("3. Bill Rate"));
  tft.setCursor(10, 70);
  tft.println(F("4. Restart"));
  tft.setCursor(10, 85);
  tft.println(F("5. Printer Test"));
  
  tft.drawLine(0, 100, 160, 100, COLOR_LINE);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setTextSize(1);
  tft.setCursor(20, 108);
  tft.println(F("Press # or press"));
  tft.setCursor(32, 118);
  tft.println(F("C to cancel"));
}

void displayViewRateScreen() {
  tft.fillScreen(COLOR_BG);
  
  tft.setTextColor(COLOR_HEADER);
  tft.setTextSize(1);
  tft.setCursor(25, 5);
  tft.println(F("CUSTOMER RATES"));
  
  tft.drawLine(0, 16, 160, 16, COLOR_LINE);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 22);
  tft.println(F("Type | Rate/m3 | Min m3"));
  
  tft.drawLine(0, 34, 160, 34, COLOR_LINE);
  
  int y = 40;
  for (int i = 0; i < getCustomerTypeCount() && y < 120; i++) {
    CustomerType* type = getCustomerTypeAt(i);
    if (type) {
      tft.setTextColor(COLOR_TEXT);
      tft.setCursor(2, y);
      tft.print(type->type_name);
      tft.setCursor(60, y);
      tft.print(F("P"));
      tft.print(type->rate_per_m3, 2);
      tft.setCursor(110, y);
      tft.print(type->min_m3);
      y += 12;
    }
  }
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(28, 118);
  tft.println(F("Press C to exit"));
}

void displayCustomerInfo() {
  tft.fillScreen(COLOR_BG);
  
  Customer* cust = currentCustomer;
  if (cust == nullptr) return;
  
  tft.setTextColor(COLOR_HEADER);
  tft.setTextSize(1);
  tft.setCursor(20, 5);
  tft.println(F("CUSTOMER INFO"));
  
  tft.drawLine(0, 16, 160, 16, COLOR_LINE);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 22);
  tft.print(F("Acct: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(cust->account_no);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 34);
  tft.print(F("Name: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(cust->customer_name);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 46);
  tft.print(F("Addr: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(cust->address);
  
  tft.drawLine(0, 58, 160, 58, COLOR_LINE);
  
  // Check if customer has existing reading and show correct previous reading
  unsigned long displayPrevReading = cust->previous_reading;
  if (hasReadingThisMonth(cust->customer_id)) {
    unsigned long existingPrev = 0, existingCurr = 0, existingUsage = 0;
    if (getExistingReadingDataThisMonth(cust->customer_id, existingPrev, existingCurr, existingUsage)) {
      displayPrevReading = existingPrev;
    }
  }
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 64);
  tft.print(F("Prev: "));
  tft.setTextColor(COLOR_AMOUNT);
  tft.println(displayPrevReading);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 100);
  tft.println(F("Press D for reading"));
  tft.setCursor(2, 112);
  tft.println(F("Press C to cancel"));
}

void displayEnterReadingScreen() {
  tft.fillScreen(COLOR_BG);
  
  Customer* cust = currentCustomer;
  if (cust == nullptr) return;
  
  tft.setTextColor(COLOR_HEADER);
  tft.setTextSize(1);
  tft.setCursor(15, 10);
  tft.println(F("CURRENT READING"));
  
  tft.drawLine(0, 20, 160, 20, COLOR_LINE);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 28);
  tft.print(F("Name: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(cust->customer_name);
  
  tft.drawLine(0, 40, 160, 40, COLOR_LINE);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 48);
  tft.println(F("Enter current reading:"));

  // Input box
  tft.drawRect(10, 58, 140, 30, COLOR_LINE);

  tft.setTextColor(COLOR_AMOUNT);
  tft.setTextSize(2);
  tft.setCursor(20, 66);
  if (inputBuffer.length() > 0) {
    tft.println(inputBuffer);
  } else {
    // Placeholder
    tft.setTextColor(COLOR_LABEL);
    tft.println(F("___"));
  }
  tft.setTextSize(1);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(20, 95);
  tft.println(F("D - Calculate"));
  tft.setCursor(32, 107);
  tft.println(F("B - Clear"));
  tft.setCursor(28, 119);
  tft.println(F("C - Cancel"));
}

void displayReadingAlreadyDoneScreen() {
  tft.fillScreen(COLOR_BG);

  Customer* cust = currentCustomer;

  tft.setTextColor(COLOR_HEADER);
  tft.setTextSize(1);
  tft.setCursor(12, 10);
  tft.println(F("READING ALREADY DONE"));

  tft.drawLine(0, 22, 160, 22, COLOR_LINE);

  if (cust != nullptr) {
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(2, 30);
    tft.print(F("Acct: "));
    tft.println(cust->account_no);

    tft.setCursor(2, 42);
    tft.print(F("Name: "));
    tft.println(cust->customer_name);
  }

  tft.setTextColor(ST77XX_RED);
  tft.setCursor(10, 62);
  tft.println(F("This month already"));
  tft.setCursor(10, 74);
  tft.println(F("has a reading."));

  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 100);
  tft.println(F("D - Continue"));
  tft.setCursor(2, 112);
  tft.println(F("C - Cancel"));
}

void displayBillCalculated() {
  tft.fillScreen(COLOR_BG);
  
  Customer* cust = currentCustomer;
  if (cust == nullptr) return;
  
  // Generate bill using customer type logic
  bool billGenerated = generateBillForCustomer(cust->account_no, currentReading);
  if (!billGenerated) {
    // Handle error - customer not found or invalid
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(1);
    tft.setCursor(20, 50);
    tft.println(F("Bill generation failed!"));
    return;
  }
  
  // Save temp reading date to SD
  if (isSDCardReady()) {
    deselectTftSelectSd();
    File f = SD.open("/temp_reading_date.txt", FILE_WRITE);
    if (f) {
      f.println(currentBill.readingDateTime);
      f.close();
      Serial.println(F("Temp reading date saved to SD"));
    } else {
      Serial.println(F("Failed to save temp reading date"));
    }
  }
  
  tft.setTextColor(COLOR_HEADER);
  tft.setTextSize(1);
  tft.setCursor(35, 5);
  tft.println(F("BILL SUMMARY"));
  
  tft.drawLine(0, 16, 160, 16, COLOR_LINE);
  
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(2, 22);
  tft.println(currentBill.customerName);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 34);
  tft.print(F("Used: "));
  tft.setTextColor(COLOR_AMOUNT);
  tft.print(currentBill.usage);
  tft.println(F(" m3"));
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 46);
  tft.print(F("Type: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(currentBill.customerType);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 58);
  if (currentBill.usage <= currentBill.minM3 && currentBill.minM3 > 0) {
    tft.print(F("Min Charge: P"));
    tft.println(currentBill.minCharge, 2);
  } else {
    tft.print(F("Rate: P"));
    tft.print(currentBill.rate, 2);
    tft.println(F("/m3"));
  }
  
  // Show deduction if any
  if (currentBill.deductions > 0) {
    tft.setTextColor(COLOR_LABEL);
    tft.setCursor(2, 70);
    tft.print(F("Discount: P"));
    tft.println(currentBill.deductions, 2);
    tft.drawLine(0, 82, 160, 82, COLOR_LINE);
    
    tft.setTextColor(COLOR_HEADER);
    tft.setCursor(15, 89);
    tft.println(F("TOTAL DUE:"));
    
    tft.setTextColor(COLOR_AMOUNT);
    tft.setTextSize(2);
    tft.setCursor(20, 102);
    tft.print(F("P"));
    tft.println(currentBill.total, 2);
    tft.setTextSize(1);
    
    tft.setTextColor(COLOR_LABEL);
    tft.setCursor(20, 120);
    tft.println(F("Press D to print"));
  } else {
    tft.drawLine(0, 70, 160, 70, COLOR_LINE);
    
    tft.setTextColor(COLOR_HEADER);
    tft.setCursor(15, 77);
    tft.println(F("TOTAL DUE:"));
    
    tft.setTextColor(COLOR_AMOUNT);
    tft.setTextSize(2);
    tft.setCursor(20, 90);
    tft.print(F("P"));
    tft.println(currentBill.total, 2);
    tft.setTextSize(1);
    
    tft.setTextColor(COLOR_LABEL);
    tft.setCursor(20, 120);
    tft.println(F("Press D to print"));
  }
}

// ===== WORKFLOW LOGIC =====

void processAccountNumberEntry() {
  // Account found
  if (selectedCustomerIndex != -1) {
    currentState = STATE_ACCOUNT_FOUND;
    inputBuffer = "";  // Clear buffer for next input
    displayCustomerInfo();
    
    Serial.print(F("Account found: "));
    Serial.println(currentCustomer->customer_name);
  } else {
    // Account not found - show error and stay in entry mode
    tft.fillScreen(COLOR_BG);
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(1);
    tft.setCursor(25, 50);
    tft.println(F("Account NOT FOUND"));
    tft.setCursor(30, 70);
    tft.println(F("Try again..."));
    
    Serial.println(F("Account not found!"));
    inputBuffer = "";
    delay(2000);
    displayEnterAccountScreen();
  }
}

void processReadingEntry() {
  currentReading = inputBuffer.toInt();
  
  if (currentReading <= currentCustomer->previous_reading) {
    // Invalid reading - must be higher than previous
    tft.fillScreen(COLOR_BG);
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(1);
    tft.setCursor(20, 50);
    tft.println(F("Invalid reading!"));
    tft.setCursor(10, 70);
    tft.println(F("Must be > previous"));
    
    Serial.println(F("Invalid reading!"));
    inputBuffer = "";
    delay(2000);
    displayEnterReadingScreen();
  } else {
    // Valid reading - calculate bill
    currentState = STATE_BILL_CALCULATED;
    inputBuffer = "";
    displayBillCalculated();
    Serial.println(F("Bill calculated. Ready to print."));
  }
}

void resetWorkflow() {
  if (currentCustomer) {
    delete currentCustomer;
    currentCustomer = nullptr;
  }
  currentState = STATE_WELCOME;
  inputBuffer = "";
  selectedCustomerIndex = -1;
  currentReading = 0;
  showWelcomeScreen();
}

// ===== GENERAL TFT DISPLAY FUNCTIONS =====

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

#endif  // TFT_SCREEN_MANAGER_H
