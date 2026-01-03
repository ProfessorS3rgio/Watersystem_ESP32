#ifndef KEYPAD_MANAGER_H
#define KEYPAD_MANAGER_H

#include <Keypad.h>
#include "config.h"
#include "Adafruit_Thermal.h"

// Forward declarations for workflow and display functions (defined in main .ino and workflow_manager.h)
void handleKeypadInput(char key);
void displayBillOnTFT();
void printBill();
void showWelcomeScreen();
void displayEnterAccountScreen();
void displayCustomerInfo();
void displayEnterReadingScreen();
void displayBillCalculated();
void processAccountNumberEntry();
void processReadingEntry();
void resetWorkflow();
void displayMenuScreen();

// External variables from workflow_manager.h
extern WorkflowState currentState;
extern String inputBuffer;
extern int selectedCustomerIndex;
extern float waterRate;

// External objects from main .ino
extern Adafruit_Thermal printer;

// Keypad setup
char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[KEYPAD_ROWS] = {13, 14, 27, 26};  // R1, R2, R3, R4
byte colPins[KEYPAD_COLS] = {25, 32, 33, 21};  // C1, C2, C3, C4

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS);

// ===== KEYPAD HANDLER WITH WORKFLOW =====
void handleKeypadInput(char key) {
  Serial.print(F("Key pressed: "));
  Serial.println(key);
  
  // Handle based on current workflow state
  if (currentState == STATE_WELCOME) {
    // On welcome screen, A opens menu, D or B starts billing
    if (key == 'A') {
      currentState = STATE_MENU;
      displayMenuScreen();
    }
    else if (key == 'D' || key == 'B') {
      currentState = STATE_ENTER_ACCOUNT;
      inputBuffer = "";
      displayEnterAccountScreen();
    }
  }
  else if (currentState == STATE_MENU) {
    // Menu navigation
    if (key == '1') {
      // About
      tft.fillScreen(COLOR_BG);
      tft.setTextColor(COLOR_HEADER);
      tft.setTextSize(1);
      tft.setCursor(50, 10);
      tft.println(F("ABOUT"));
      tft.drawLine(0, 20, 160, 20, COLOR_LINE);
      tft.setTextColor(COLOR_TEXT);
      tft.setCursor(5, 30);
      tft.println(F("Water Billing v1.0"));
      tft.setCursor(5, 45);
      tft.println(F("DONA JOSEFA M."));
      tft.setCursor(5, 55);
      tft.println(F("BULU-AN CAPARAN"));
      tft.setCursor(5, 70);
      tft.println(F("Zamboanga Sibugay"));
      tft.setTextColor(COLOR_LABEL);
      tft.setCursor(28, 110);
      tft.println(F("Press C to exit"));
    }
    else if (key == '2') {
      // Update - placeholder
      tft.fillScreen(COLOR_BG);
      tft.setTextColor(COLOR_HEADER);
      tft.setCursor(45, 50);
      tft.println(F("UPDATE"));
      tft.setTextColor(COLOR_TEXT);
      tft.setCursor(20, 70);
      tft.println(F("Coming soon..."));
      delay(2000);
      displayMenuScreen();
    }
    else if (key == '3') {
      // Bill Rate
      tft.fillScreen(COLOR_BG);
      tft.setTextColor(COLOR_HEADER);
      tft.setCursor(35, 10);
      tft.println(F("BILL RATE"));
      tft.drawLine(0, 20, 160, 20, COLOR_LINE);
      tft.setTextColor(COLOR_LABEL);
      tft.setCursor(5, 35);
      tft.println(F("Current Rate:"));
      tft.setTextColor(COLOR_AMOUNT);
      tft.setTextSize(2);
      tft.setCursor(20, 50);
      tft.print(F("P"));
      tft.print(waterRate, 2);
      tft.println(F("/m3"));
      tft.setTextSize(1);
      tft.setTextColor(COLOR_LABEL);
      tft.setCursor(28, 110);
      tft.println(F("Press C to exit"));
    }
    else if (key == '4') {
      // Restart
      tft.fillScreen(COLOR_BG);
      tft.setTextColor(ST77XX_RED);
      tft.setCursor(35, 50);
      tft.println(F("Restarting..."));
      delay(1000);
      ESP.restart();
    }
    else if (key == '5') {
      // Printer Test
      tft.fillScreen(COLOR_BG);
      tft.setTextColor(COLOR_HEADER);
      tft.setCursor(25, 50);
      tft.println(F("Printing test..."));
      printer.wake();
      printer.println(F("=== PRINTER TEST ==="));
      printer.println(F("Water Billing System"));
      printer.println(F("Test Print OK"));
      printer.feed(3);
      delay(2000);
      displayMenuScreen();
    }
    else if (key == 'C' || key == '#') {
      // Exit menu
      resetWorkflow();
    }
  }
  else if (currentState == STATE_ENTER_ACCOUNT) {
    // Entering account number
    if (key >= '0' && key <= '9') {
      inputBuffer += key;
      displayEnterAccountScreen();
    }
    else if (key == 'D') {  // Confirm account
      selectedCustomerIndex = findCustomerByAccount(inputBuffer);
      processAccountNumberEntry();
    }
    else if (key == 'C') {  // Clear
      inputBuffer = "";
      displayEnterAccountScreen();
    }
    else if (key == 'B') {  // Cancel - go back to welcome
      resetWorkflow();
    }
  }
  else if (currentState == STATE_ACCOUNT_FOUND) {
    // Customer info displayed
    if (key == 'D') {  // Proceed to reading
      currentState = STATE_ENTER_READING;
      inputBuffer = "";
      displayEnterReadingScreen();
    }
    else if (key == 'C') {  // Cancel and go back
      resetWorkflow();
    }
  }
  else if (currentState == STATE_ENTER_READING) {
    // Entering meter reading
    if (key >= '0' && key <= '9') {
      inputBuffer += key;
      displayEnterReadingScreen();
    }
    else if (key == 'D') {  // Confirm reading
      processReadingEntry();
    }
    else if (key == 'B') {  // Clear input
      inputBuffer = "";
      displayEnterReadingScreen();
    }
    else if (key == 'C') {  // Cancel - go back
      resetWorkflow();
    }
  }
  else if (currentState == STATE_BILL_CALCULATED) {
    // Bill ready to print
    if (key == 'D') {  // Print bill
      Serial.println(F("Starting print job..."));
      currentState = STATE_PRINTING;
      
      // Start printing and animation in parallel using FreeRTOS
      startParallelPrinting();
      
      // Wait for both tasks to complete
      waitForPrintCompletion();
      
      Serial.println(F("Print workflow complete."));
      delay(500);
      resetWorkflow();
    }
    else if (key == 'C') {  // Cancel
      resetWorkflow();
    }
  }
  else if (currentState == STATE_WELCOME) {
    // Welcome screen
    if (key == 'D' || key == 'B') {  // D or B to start
      currentState = STATE_ENTER_ACCOUNT;
      inputBuffer = "";
      displayEnterAccountScreen();
    }
  }
}


#endif  // KEYPAD_MANAGER_H
