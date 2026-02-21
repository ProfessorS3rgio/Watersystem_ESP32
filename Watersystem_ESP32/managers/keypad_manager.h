#ifndef KEYPAD_MANAGER_H
#define KEYPAD_MANAGER_H

#include <Adafruit_MCP23X17.h>
#include "configuration/config.h"
#include "Adafruit_Thermal.h"
#include "../database/readings_database.h"
#include "../database/customers_database.h"
#include "../database/barangay_database.h"
#include "../database/bill_transactions_database.h"
#include "../database/test_data_generator.h"
#include "../printer/receipt_printer.h"
#include "../screens/warning_screen.h"
#include <SD.h>

// ===== EXTERNAL FROM CUSTOMERS DATABASE =====
extern Customer* currentCustomer;

// Forward declarations for workflow and display functions (defined in main .ino and workflow_manager.h)
void handleKeypadInput(char key);
void displayBillOnTFT();
void printBill();
void showWelcomeScreen();
void displayEnterAccountScreen();
void displayCustomerInfo();
void displayEnterReadingScreen();
void displayBillCalculated();
void displayEnterPaymentScreen();
void displayPaymentSummary();
void displayPaymentConfirmation();
void processAccountNumberEntry();
void processReadingEntry();
void resetWorkflow();
void displayMenuScreen();
void displayViewRateScreen();
void startParallelPrintingJob(void (*job)());
void waitForPrintCompletion();

// External variables from workflow_manager.h
extern WorkflowState currentState;
extern String inputBuffer;
extern int selectedCustomerIndex;
extern unsigned long currentReading;
extern bool isPaymentFlow;
extern bool isVoidFlow;
extern float paymentAmount;

// External objects from main .ino
extern Adafruit_Thermal printer;

// Keypad setup
char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// MCP23017 pins for keypad
// Rows: GPA0-GPA3, Columns: GPA4-GPA7

// ===== KEYPAD SCANNING FUNCTION =====
char getKey() {
  // Set columns as outputs HIGH, rows as inputs with pullup
  for (int c = 0; c < KEYPAD_COLS; c++) {
    mcp.pinMode(KEYPAD_COL_PINS[c], OUTPUT);
    mcp.digitalWrite(KEYPAD_COL_PINS[c], HIGH);
  }
  for (int r = 0; r < KEYPAD_ROWS; r++) {
    mcp.pinMode(KEYPAD_ROW_PINS[r], INPUT_PULLUP);
  }

  // Scan for press
  for (int c = 0; c < KEYPAD_COLS; c++) {
    mcp.digitalWrite(KEYPAD_COL_PINS[c], LOW);
    delay(1);  // Small delay for stability
    for (int r = 0; r < KEYPAD_ROWS; r++) {
      if (mcp.digitalRead(KEYPAD_ROW_PINS[r]) == LOW) {
        // Debounce
        delay(10);
        if (mcp.digitalRead(KEYPAD_ROW_PINS[r]) == LOW) {
          // Wait for release
          while (true) {
            bool released = true;
            for (int cc = 0; cc < KEYPAD_COLS; cc++) {
              mcp.digitalWrite(KEYPAD_COL_PINS[cc], LOW);
              for (int rr = 0; rr < KEYPAD_ROWS; rr++) {
                if (mcp.digitalRead(KEYPAD_ROW_PINS[rr]) == LOW) {
                  released = false;
                  break;
                }
              }
              mcp.digitalWrite(KEYPAD_COL_PINS[cc], HIGH);
              if (!released) break;
            }
            if (released) break;
            delay(10);
          }
          mcp.digitalWrite(KEYPAD_COL_PINS[c], HIGH);
          return keys[r][c];
        }
      }
    }
    mcp.digitalWrite(KEYPAD_COL_PINS[c], HIGH);
  }
  return '\0';  // No key pressed
}

// ===== KEYPAD HANDLER WITH WORKFLOW =====
void handleKeypadInput(char key) {
  Serial.print(F("Key pressed: "));
  Serial.println(key);
  
  // Handle based on current workflow state
  if (currentState == STATE_WELCOME) {
    // On welcome screen, A opens menu, B starts billing, D starts payment
    if (key == 'A') {
      currentState = STATE_MENU;
      displayMenuScreen();
    }
    else if (key == 'B') {
      isPaymentFlow = false;
      currentState = STATE_ENTER_ACCOUNT;
      inputBuffer = "";
      displayEnterAccountScreen();
    }
    else if (key == 'D') {
      isPaymentFlow = true;
      currentState = STATE_ENTER_ACCOUNT;
      inputBuffer = "";
      displayEnterAccountScreen();
    }
  }
  else if (currentState == STATE_MENU) {
    // Menu navigation
    if (key == '1') {
      // About
      currentState = STATE_ABOUT;
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
      tft.setCursor(8, 110);
      tft.println(F("Press C to go back"));
      tft.setCursor(28, 120);
      tft.println(F("to the Menu"));
    }
    else if (key == '2') {
      // Update - placeholder
      tft.fillScreen(COLOR_BG);
      tft.setTextColor(COLOR_HEADER);
      tft.setCursor(62, 50);
      tft.println(F("UPDATE"));
      tft.setTextColor(COLOR_TEXT);
      tft.setCursor(38, 70);
      tft.println(F("Coming soon..."));
      delay(2000);
      displayMenuScreen();
    }
    else if (key == '3') {
      // Void payments
      isVoidFlow = true;
      currentState = STATE_VOID_ENTER_ACCOUNT;
      inputBuffer = "";
      displayEnterAccountScreen();
    }
    else if (key == '4') {
      // Restart
      tft.fillScreen(COLOR_BG);
      tft.setTextColor(TFT_RED);
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
    else if (key == '6') {
      // Generate Test Data
      tft.fillScreen(COLOR_BG);
      tft.setTextColor(COLOR_HEADER);
      tft.setCursor(20, 50);
      tft.println(F("Generating test data..."));
      generateTestReadingsAndBills(1000);
      // Also generate transactions for existing bills if needed
      generateTestBillTransactions(1000);
      tft.setCursor(20, 70);
      tft.println(F("Done!"));
      delay(2000);
      displayMenuScreen();
    }
    else if (key == 'C' || key == '#') {
      // Exit menu
      resetWorkflow();
    }
  }
  else if (currentState == STATE_ABOUT) {
    // About screen
    if (key == 'C') {
      currentState = STATE_MENU;
      displayMenuScreen();
    }
  }
  else if (currentState == STATE_ENTER_ACCOUNT) {
    // Entering account number
    if (key >= '0' && key <= '9') {
      inputBuffer += key;
      displayEnterAccountScreen();
    }
    else if (key == 'D') {  // Confirm account
      // Format account number with barangay prefix
      String prefix = getCurrentBarangayPrefix();
      String accountNum = inputBuffer;
      // Pad with leading zero if needed to make 3 digits
      while (accountNum.length() < 3) {
        accountNum = "0" + accountNum;
      }
      String fullAccount = prefix + "-" + accountNum;
      selectedCustomerIndex = findCustomerByAccount(fullAccount);
      processAccountNumberEntry();
    }
    else if (key == 'B') {  // Clear
      inputBuffer = "";
      displayEnterAccountScreen();
    }
    else if (key == 'C') {  // Cancel - go back to welcome
      resetWorkflow();
    }
  }
  else if (currentState == STATE_ACCOUNT_FOUND) {
    // Customer info displayed
    if (key == 'D') {  // Proceed to reading
      if (currentCustomer != nullptr && hasReadingThisMonth(currentCustomer->customer_id)) {
        bool billRetrieved = getBillForCustomer(currentCustomer->account_no);
        if (billRetrieved && currentBill.status.equalsIgnoreCase("paid")) {
          displayWarningScreen(F("ALREADY PAID"),
                               String("Void bill/payment first"),
                               String("before continuing."),
                               F("Press C to cancel"));
          delay(1800);
          displayCustomerInfo();
          return;
        }
      }

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

      // Track print count for device info
      incrementPrintCount();
      
      // Start printing and animation in parallel using FreeRTOS
      startParallelPrinting();
      
      // Wait for both tasks to complete
      waitForPrintCompletion();
      
      Serial.println(F("Print workflow complete."));
      
      // Remove temp reading date file
      deselectTftSelectSd();
      SD.remove("/temp_reading_date.txt");
      Serial.println(F("Temp reading date removed from SD"));
      
      delay(500);
      resetWorkflow();
    }
    else if (key == 'C') {  // Cancel
      resetWorkflow();
    }
  }
  else if (currentState == STATE_VIEW_RATE) {
    // Viewing current rate
    if (key == 'C') {
      // Exit to menu
      currentState = STATE_MENU;
      displayMenuScreen();
    }
  }
  else if (currentState == STATE_ENTER_PAYMENT) {
    // Entering payment amount
    if ((key >= '0' && key <= '9') || key == '.') {
      inputBuffer += key;
      displayEnterPaymentScreen();
    }
    else if (key == 'D') {  // Confirm payment
      if (inputBuffer.length() == 0) {
        displayWarningScreen(F("INVALID PAYMENT"),
                             String("Enter payment amount"),
                             String(""),
                             F("Press B to clear"));
        delay(1200);
        displayEnterPaymentScreen();
        return;
      }

      paymentAmount = inputBuffer.toFloat();
      if (paymentAmount <= 0.0f) {
        displayWarningScreen(F("INVALID PAYMENT"),
                             String("Amount must be > 0"),
                             String(""),
                             F("Press B to clear"));
        delay(1200);
        displayEnterPaymentScreen();
        return;
      }

      // Rule: do not accept underpayment. Overpay is allowed (we return change).
      // Use cents to avoid float comparison issues.
      long dueCents = lroundf(currentBill.total * 100.0f);
      long payCents = lroundf(paymentAmount * 100.0f);

      if (payCents < dueCents) {
        displayWarningScreen(F("INSUFFICIENT"),
                             String("Payment is less than") + String(" total due"),
                             String("Total Due: P") + String(currentBill.total, 2),
                             F("B-Clear  C-Cancel"));
        delay(1500);
        displayEnterPaymentScreen();
        return;
      }

      currentState = STATE_PAYMENT_CONFIRMATION;
      displayPaymentConfirmation();
    }
    else if (key == 'B') {  // Clear input
      inputBuffer = "";
      displayEnterPaymentScreen();
    }
    else if (key == 'C') {  // Cancel - go back to account entry
      currentState = STATE_ENTER_ACCOUNT;
      inputBuffer = "";
      displayEnterAccountScreen();
    }
  }
  else if (currentState == STATE_PAYMENT_SUMMARY) {
    // Payment summary showing bill details
    if (key == 'D') {  // Pay or Print
      if (currentBill.status.equalsIgnoreCase("paid")) {
        // Already paid, re-print receipt
        // Load the latest transaction for this bill
        BillTransaction latestTxn;
        if (getLatestTransactionForBill(currentBill.refNumber, latestTxn)) {
          // Populate receipt data from current bill + transaction
          currentReceipt.receiptNumber = String("OR-") + currentBill.refNumber;
          currentReceipt.paymentDateTime = latestTxn.transaction_date;
          currentReceipt.billRefNumber = currentBill.refNumber;

          currentReceipt.customerName = currentBill.customerName;
          currentReceipt.accountNo = currentBill.accountNo;
          currentReceipt.customerType = currentBill.customerType;
          currentReceipt.address = currentBill.address;
          currentReceipt.collector = currentBill.collector;
          if (currentReceipt.collector.length() == 0) {
            currentReceipt.collector = COLLECTOR_NAME_VALUE;
          }

          currentReceipt.prevReading = currentBill.prevReading;
          currentReceipt.currReading = currentBill.currReading;
          currentReceipt.usage = currentBill.usage;

          currentReceipt.rate = currentBill.rate;
          currentReceipt.subtotal = currentBill.subtotal;
          currentReceipt.deductions = currentBill.deductions;
          currentReceipt.penalty = currentBill.penalty;
          currentReceipt.total = currentBill.total;

          currentReceipt.amountPaid = latestTxn.cash_received;
          currentReceipt.change = latestTxn.change;

          // Print receipt
          startParallelPrintingJob(printReceipt);
          waitForPrintCompletion();

          // Track print count for device info
          incrementPrintCount();

          tft.fillScreen(COLOR_BG);
          tft.setTextColor(TFT_GREEN);
          tft.setCursor(30, 50);
          tft.println(F("Receipt Re-printed"));
          delay(1500);
          resetWorkflow();
        } else {
          tft.fillScreen(COLOR_BG);
          tft.setTextColor(ST77XX_RED);
          tft.setCursor(30, 50);
          tft.println(F("No transaction found"));
          tft.setCursor(30, 70);
          tft.println(F("Cannot re-print"));
          delay(2000);
          resetWorkflow();
        }
      } else {
        // Not paid, proceed to payment
        currentState = STATE_ENTER_PAYMENT;
        inputBuffer = "";
        displayEnterPaymentScreen();
      }
    }
    else if (key == 'C') {  // Cancel
      resetWorkflow();
    }
  }
  else if (currentState == STATE_PAYMENT_CONFIRMATION) {
    // Payment confirmation
    if (key == 'D') {  // Print receipt and save transaction
      float changeAmount = paymentAmount - currentBill.total;
      if (changeAmount < 0.0f) changeAmount = 0.0f;

      bool saved = recordPaymentTransactionByBillRef(currentBill.refNumber, currentBill.total, paymentAmount, changeAmount);
      if (saved) {
        Serial.println(F("Payment transaction saved successfully"));
      } else {
        Serial.println(F("Failed to save payment transaction"));
      }

      // Populate receipt data from current bill + payment
      currentReceipt.receiptNumber = String("OR-") + currentBill.refNumber;
      currentReceipt.paymentDateTime = getCurrentDateTimeString();
      currentReceipt.billRefNumber = currentBill.refNumber;

      currentReceipt.customerName = currentBill.customerName;
      currentReceipt.accountNo = currentBill.accountNo;
      currentReceipt.customerType = currentBill.customerType;
      currentReceipt.address = currentBill.address;
      currentReceipt.collector = currentBill.collector;
      if (currentReceipt.collector.length() == 0) {
        currentReceipt.collector = COLLECTOR_NAME_VALUE;
      }

      currentReceipt.prevReading = currentBill.prevReading;
      currentReceipt.currReading = currentBill.currReading;
      currentReceipt.usage = currentBill.usage;

      currentReceipt.rate = currentBill.rate;
      currentReceipt.subtotal = currentBill.subtotal;
      currentReceipt.deductions = currentBill.deductions;
      currentReceipt.penalty = currentBill.penalty;
      currentReceipt.total = currentBill.total;

      currentReceipt.amountPaid = paymentAmount;
      currentReceipt.change = changeAmount;

      // Print receipt with loading screen (even if DB save failed, still allow printing)
      startParallelPrintingJob(printReceipt);
      waitForPrintCompletion();

      // Track print count for device info
      incrementPrintCount();

      tft.fillScreen(COLOR_BG);
      tft.setTextColor(saved ? TFT_GREEN : TFT_YELLOW);
      tft.setCursor(30, 50);
      tft.println(saved ? F("Receipt Printed") : F("Receipt Printed"));
      tft.setCursor(30, 70);
      tft.println(saved ? F("Transaction Saved") : F("Txn Save Failed"));
      delay(1500);
      resetWorkflow();
    }
    else if (key == 'C') {  // Cancel
      resetWorkflow();
    }
  }
  else if (currentState == STATE_VOID_ENTER_ACCOUNT) {
    // Entering account number for void
    if (key >= '0' && key <= '9') {
      inputBuffer += key;
      displayEnterAccountScreen();
    }
    else if (key == 'D') {  // Confirm account
      // Format account number with barangay prefix
      String prefix = getCurrentBarangayPrefix();
      String accountNum = inputBuffer;
      // Pad with leading zero if needed to make 3 digits
      while (accountNum.length() < 3) {
        accountNum = "0" + accountNum;
      }
      String fullAccount = prefix + "-" + accountNum;
      selectedCustomerIndex = findCustomerByAccount(fullAccount);
      processAccountNumberEntry();
    }
    else if (key == 'B') {  // Clear
      inputBuffer = "";
      displayEnterAccountScreen();
    }
    else if (key == 'C') {  // Cancel - go back to menu
      resetWorkflow();
    }
  }
  else if (currentState == STATE_VOID_DISPLAY_TRANSACTION) {
    // Displaying transaction details for void
    if (key == 'D') {  // Void payment
      bool voided = voidPaymentTransaction(currentTransaction.bill_transaction_id);
      if (voided) {
        tft.fillScreen(COLOR_BG);
        tft.setTextColor(TFT_GREEN);
        tft.setCursor(50, 50);
        tft.println(F("Payment voided successfully"));
        delay(2000);
      } else {
        tft.fillScreen(COLOR_BG);
        tft.setTextColor(ST77XX_RED);
        tft.setCursor(50, 50);
        tft.println(F("Failed to void payment"));
        delay(2000);
      }
      resetWorkflow();
    }
    else if (key == 'C') {  // Cancel
      resetWorkflow();
    }
  }
}


#endif  // KEYPAD_MANAGER_H
