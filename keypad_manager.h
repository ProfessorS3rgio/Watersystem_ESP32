#ifndef KEYPAD_MANAGER_H
#define KEYPAD_MANAGER_H

#include <Keypad.h>
#include "config.h"

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

// External variables from workflow_manager.h
extern WorkflowState currentState;
extern String inputBuffer;
extern int selectedCustomerIndex;

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
  if (currentState == STATE_ENTER_ACCOUNT) {
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
    else if (key == 'C') {  // Clear
      inputBuffer = "";
      displayEnterReadingScreen();
    }
  }
  else if (currentState == STATE_BILL_CALCULATED) {
    // Bill ready to print
    if (key == 'D') {  // Print bill
      Serial.println(F("Printing bill..."));
      printBill();
      Serial.println(F("Print complete."));
      delay(1000);
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
