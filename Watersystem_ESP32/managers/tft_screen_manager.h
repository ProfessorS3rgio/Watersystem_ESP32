#ifndef TFT_SCREEN_MANAGER_H
#define TFT_SCREEN_MANAGER_H

#include "../configuration/config.h"
#include "../database/customers_database.h"
#include "../database/bill_database.h"
#include <TFT_eSPI.h>

// ===== EXTERNAL OBJECTS FROM MAIN .INO =====
extern TFT_eSPI tft;

#include "components/bmp_display.h"
#include "components/battery_display.h"
#include "../screens/welcome_screen.h"
#include "../screens/menu_screen.h"
#include "../screens/enter_account_screen.h"
#include "../screens/customer_info_screen.h"
#include "../screens/enter_reading_screen.h"
#include "../screens/reading_already_done_screen.h"
#include "../screens/bill_calculated_screen.h"
#include "../screens/enter_payment_screen.h"
#include "../screens/payment_summary_screen.h"
#include "../screens/view_rate_screen.h"
#include "../screens/bill_display_screen.h"
#include <SD.h>

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
  STATE_VIEW_RATE,         // Viewing current rate
  STATE_ENTER_PAYMENT,     // Waiting for payment amount entry
  STATE_PAYMENT_SUMMARY    // Payment summary, ready to confirm
};

WorkflowState currentState = STATE_WELCOME;
String inputBuffer = "";          // Buffer for numeric input
int selectedCustomerIndex = -1;   // Index of selected customer
unsigned long currentReading = 0; // Current meter reading from keypad
unsigned long correctPreviousReading = 0; // Correct previous reading for validation
bool isPaymentFlow = false;       // Flag to distinguish billing vs payment flow

// ===== DISPLAY FUNCTIONS FOR WORKFLOW =====

// displayMenuScreen() moved to screens/menu_screen.h

// ===== WORKFLOW LOGIC =====

void processAccountNumberEntry() {
  // Account found
  if (selectedCustomerIndex != -1) {
    inputBuffer = "";  // Clear buffer for next input
    if (isPaymentFlow) {
      currentState = STATE_ENTER_PAYMENT;
      displayEnterPaymentScreen();
    } else {
      currentState = STATE_ACCOUNT_FOUND;
      displayCustomerInfo();
    }
    
    Serial.print(F("Account found: "));
    Serial.println(currentCustomer->customer_name);
    if (!isPaymentFlow) {
      Serial.print(F("Correct previous reading: "));
      Serial.println(correctPreviousReading);
    }
  } else {
    // Account not found - show error and stay in entry mode
    tft.fillScreen(COLOR_BG);
    tft.setTextColor(TFT_RED);
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
  
  Serial.print(F("Entered reading: "));
  Serial.println(currentReading);
  
  if (currentReading <= correctPreviousReading) {
    // Invalid reading - must be higher than previous
    tft.fillScreen(COLOR_BG);
    tft.setTextColor(TFT_RED);
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
  isPaymentFlow = false;
  showWelcomeScreen();
}

// ===== GENERAL TFT DISPLAY FUNCTIONS =====

#endif  // TFT_SCREEN_MANAGER_H
