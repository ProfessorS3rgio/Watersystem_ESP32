#ifndef PRINT_MANAGER_H
#define PRINT_MANAGER_H

#include "config.h"
#include <esp_task_wdt.h>

// ===== EXTERNAL OBJECTS FROM MAIN .INO =====
extern Adafruit_ST7735 tft;

// ===== PRINTER STATUS TRACKING =====
volatile bool isPrinting = false;
volatile bool printComplete = false;
volatile int printProgress = 0;  // 0-100%

// ===== FREERTOS TASK HANDLES =====
TaskHandle_t printTaskHandle = NULL;
TaskHandle_t animationTaskHandle = NULL;

// Forward declarations
void printBill();
void displayPrintingAnimation();

// ===== PRINTER TASK (Runs in background) =====
void printerTask(void *parameter) {
  // Start printing
  isPrinting = true;
  printComplete = false;
  printProgress = 0;
  
  Serial.println(F("[Printer Task] Starting print..."));
  
  // Subscribe to watchdog first, then unsubscribe
  // This ensures proper registration before deletion
  esp_task_wdt_add(NULL);  // Subscribe current task to WDT
  esp_task_wdt_delete(NULL);  // Now unsubscribe it (safely)
  
  // Call the print function
  printBill();
  
  // Mark as complete
  isPrinting = false;
  printComplete = true;
  printProgress = 100;
  
  Serial.println(F("[Printer Task] Print finished!"));
  
  // Delete this task
  vTaskDelete(NULL);
}

// ===== ANIMATION TASK (Monitor & Display) =====
void animationTask(void *parameter) {
  tft.fillScreen(COLOR_BG);
  
  tft.setTextColor(COLOR_HEADER);
  tft.setTextSize(2);
  tft.setCursor(25, 20);
  tft.println(F("PRINTING"));
  
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(1);
  tft.setCursor(35, 45);
  tft.println(F("Please wait..."));
  
  // Draw progress bar border
  tft.drawRect(20, 70, 120, 20, COLOR_LINE);
  
  int lastProgress = 0;
  unsigned long animationStartTime = millis();
  
  // Run while printing or until complete
  while (!printComplete) {
    unsigned long elapsedTime = millis() - animationStartTime;
    
    // Estimate progress based on elapsed time (33 seconds = 33000ms)
    int estimatedProgress = (elapsedTime * 100) / 33000;
    
    // Don't go over 95% until actually done
    if (estimatedProgress > 95) {
      estimatedProgress = 95;
    }
    
    // Update progress bar if changed
    if (estimatedProgress != lastProgress) {
      lastProgress = estimatedProgress;
      printProgress = estimatedProgress;
      
      // Fill progress bar
      int barWidth = (120 * estimatedProgress) / 100;
      tft.fillRect(20, 70, barWidth, 20, COLOR_AMOUNT);
      
      // Update percentage text
      tft.setTextColor(COLOR_BG);
      tft.setTextSize(1);
      tft.setCursor(60, 75);
      if (estimatedProgress < 10) {
        tft.print(F("  "));
      } else if (estimatedProgress < 100) {
        tft.print(F(" "));
      }
      tft.println(estimatedProgress);
      
      Serial.print(F("[Animation] Progress: "));
      Serial.print(estimatedProgress);
      Serial.println(F("%"));
    }
    
    delay(500);  // Update every 500ms
  }
  
  // Print complete - fill bar to 100%
  tft.fillRect(20, 70, 120, 20, COLOR_AMOUNT);
  tft.setTextColor(COLOR_BG);
  tft.setTextSize(1);
  tft.setCursor(60, 75);
  tft.println(F("100%"));
  
  // Show completion message
  tft.setTextColor(COLOR_AMOUNT);
  tft.setTextSize(1);
  tft.setCursor(35, 105);
  tft.println(F("Done!"));
  
  Serial.println(F("[Animation] Print complete!"));
  delay(1000);  // Show completion for 1 second
  
  // Delete this task
  vTaskDelete(NULL);
}

// ===== START PARALLEL PRINTING =====
void startParallelPrinting() {
  Serial.println(F("Starting parallel print and animation tasks..."));
  
  // Create printer task (high priority)
  xTaskCreate(
    printerTask,           // Function to execute
    "PrinterTask",         // Task name
    4096,                  // Stack size (words)
    NULL,                  // Parameter
    2,                     // Priority (higher = more important)
    &printTaskHandle       // Task handle
  );
  
  // Small delay to let printer start
  delay(100);
  
  // Create animation task (medium priority)
  xTaskCreate(
    animationTask,         // Function to execute
    "AnimationTask",       // Task name
    4096,                  // Stack size (words)
    NULL,                  // Parameter
    1,                     // Priority
    &animationTaskHandle   // Task handle
  );
}

// ===== WAIT FOR PRINT COMPLETION =====
void waitForPrintCompletion() {
  unsigned long timeoutTime = millis() + 45000;  // 45 second timeout
  
  while (!printComplete && millis() < timeoutTime) {
    delay(100);  // Check every 100ms
  }
  
  if (!printComplete) {
    Serial.println(F("WARNING: Print timeout after 45 seconds!"));
  }
}

#endif  // PRINT_MANAGER_H
