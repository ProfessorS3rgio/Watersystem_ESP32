#ifndef WORKFLOW_MANAGER_H
#define WORKFLOW_MANAGER_H

#include "config.h"
#include "customers_database.h"
#include "water_system.h"

// ===== EXTERNAL OBJECTS FROM MAIN .INO =====
extern Adafruit_ST7735 tft;

// ===== FORWARD DECLARATIONS =====
void showWelcomeScreen();
void displayBillOnTFT();
void printBill();

// ===== WORKFLOW STATES =====
enum WorkflowState {
  STATE_WELCOME,           // Initial welcome screen
  STATE_MENU,              // Menu screen
  STATE_ENTER_ACCOUNT,     // Waiting for account number entry
  STATE_ACCOUNT_FOUND,     // Account found, showing customer info
  STATE_ENTER_READING,     // Waiting for current meter reading entry
  STATE_BILL_CALCULATED,   // Bill calculated, ready to print
  STATE_PRINTING           // Printing bill
};

WorkflowState currentState = STATE_WELCOME;
String inputBuffer = "";          // Buffer for numeric input
int selectedCustomerIndex = -1;   // Index of selected customer
unsigned long currentReading = 0; // Current meter reading from keypad
float waterRate = 15.00;          // Default water rate per m3

// ===== DISPLAY FUNCTIONS FOR WORKFLOW =====

void displayEnterAccountScreen() {
  tft.fillScreen(COLOR_BG);
  
  tft.setTextColor(COLOR_HEADER);
  tft.setTextSize(1);
  tft.setCursor(20, 10);
  tft.println(F("ENTER ACCOUNT #"));
  
  tft.drawLine(0, 20, 160, 20, COLOR_LINE);
  
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setCursor(30, 50);
  tft.println(inputBuffer);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setTextSize(1);
  tft.setCursor(32, 85);  // Centered
  tft.println(F("D - Confirm"));
  tft.setCursor(38, 97);  // Centered
  tft.println(F("C - Clear"));
  tft.setCursor(36, 109);  // Centered
  tft.println(F("B - Cancel"));
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

void displayCustomerInfo() {
  tft.fillScreen(COLOR_BG);
  
  Customer* cust = getCustomerAt(selectedCustomerIndex);
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
  tft.println(cust->accountNumber);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 34);
  tft.print(F("Name: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(cust->customerName);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 46);
  tft.print(F("Addr: "));
  tft.setTextColor(COLOR_TEXT);
  tft.println(cust->address);
  
  tft.drawLine(0, 58, 160, 58, COLOR_LINE);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 64);
  tft.print(F("Prev: "));
  tft.setTextColor(COLOR_AMOUNT);
  tft.println(cust->previousReading);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 100);
  tft.println(F("Press D for reading"));
  tft.setCursor(2, 112);
  tft.println(F("Press C to cancel"));
}

void displayEnterReadingScreen() {
  tft.fillScreen(COLOR_BG);
  
  Customer* cust = getCustomerAt(selectedCustomerIndex);
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
  tft.println(cust->customerName);
  
  tft.drawLine(0, 40, 160, 40, COLOR_LINE);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 48);
  tft.println(F("Enter current reading:"));
  
  tft.setTextColor(COLOR_AMOUNT);
  tft.setTextSize(2);
  tft.setCursor(30, 65);
  tft.println(inputBuffer);
  tft.setTextSize(1);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(20, 95);
  tft.println(F("D - Calculate"));
  tft.setCursor(32, 107);
  tft.println(F("B - Clear"));
  tft.setCursor(28, 119);
  tft.println(F("C - Cancel"));
}

void displayBillCalculated() {
  tft.fillScreen(COLOR_BG);
  
  Customer* cust = getCustomerAt(selectedCustomerIndex);
  if (cust == nullptr) return;
  
  // Calculate bill
  unsigned long used = currentReading - cust->previousReading;
  float rate = 15.00;  // Default rate
  float total = used * rate;
  
  // Update the currentBill for printing
  currentBill.customerName = cust->customerName;
  currentBill.accountNo = cust->accountNumber;
  currentBill.address = cust->address;
  currentBill.prevReading = cust->previousReading;
  currentBill.currReading = currentReading;
  currentBill.rate = rate;
  
  tft.setTextColor(COLOR_HEADER);
  tft.setTextSize(1);
  tft.setCursor(35, 5);
  tft.println(F("BILL SUMMARY"));
  
  tft.drawLine(0, 16, 160, 16, COLOR_LINE);
  
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(2, 22);
  tft.println(cust->customerName);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 34);
  tft.print(F("Used: "));
  tft.setTextColor(COLOR_AMOUNT);
  tft.print(used);
  tft.println(F(" m3"));
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 46);
  tft.print(F("Rate: "));
  tft.setTextColor(COLOR_TEXT);
  tft.print(F("PHP "));
  tft.println(waterRate, 2);  // Use global waterRate
  
  tft.drawLine(0, 58, 160, 58, COLOR_LINE);
  
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(15, 65);
  tft.println(F("TOTAL DUE:"));
  
  tft.setTextColor(COLOR_AMOUNT);
  tft.setTextSize(2);
  tft.setCursor(20, 78);
  tft.print(F("P"));
  tft.println(total, 2);
  tft.setTextSize(1);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(20, 108);
  tft.println(F("Press D to print"));
}

// ===== WORKFLOW LOGIC =====

void processAccountNumberEntry() {
  // Account found
  if (selectedCustomerIndex != -1) {
    currentState = STATE_ACCOUNT_FOUND;
    inputBuffer = "";  // Clear buffer for next input
    displayCustomerInfo();
    
    Serial.print(F("Account found: "));
    Serial.println(customers[selectedCustomerIndex].customerName);
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
  
  if (currentReading <= customers[selectedCustomerIndex].previousReading) {
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
  currentState = STATE_WELCOME;
  inputBuffer = "";
  selectedCustomerIndex = -1;
  currentReading = 0;
  showWelcomeScreen();
}

#endif  // WORKFLOW_MANAGER_H
