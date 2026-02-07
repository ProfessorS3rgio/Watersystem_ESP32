#ifndef PRINT_MANAGER_H
#define PRINT_MANAGER_H

#include "../configuration/config.h"
#include <TFT_eSPI.h>
#include <esp_task_wdt.h>
#include "../screens/printer_icon_bar_loading_screen.h"

// ===== EXTERNAL OBJECTS FROM MAIN .INO =====
extern TFT_eSPI tft;

// ===== PRINTER STATUS TRACKING =====
volatile bool isPrinting = false;
volatile bool printComplete = false;
volatile int printProgress = 0;  // 0-100%

// ===== FREERTOS TASK HANDLES =====
TaskHandle_t printTaskHandle = NULL;
TaskHandle_t animationTaskHandle = NULL;

// Forward declarations
void printBill();
void printerTask(void *parameter);
void animationTask(void *parameter);

static PrinterIconBarLoadingState g_printLoadingState;

// Time-based progress estimate (used when we don't have real progress updates).
// Increased so the UI matches typical bill print duration better.
static const uint32_t WS_PRINT_ESTIMATED_DURATION_MS = 42000; // was 33000 (~+9s)

typedef void (*PrintJobFn)();
static volatile PrintJobFn g_printJobFn = nullptr;

static void startParallelPrintingInternal(PrintJobFn job) {
  g_printJobFn = job;

  Serial.println(F("Starting parallel print and animation tasks..."));
  
  // Create printer task (high priority)
  xTaskCreatePinnedToCore(
    printerTask,
    "PrinterTask",
    4096,
    NULL,
    1,
    &printTaskHandle,
    1   // ✅ CORE 1 (critical)
  );
  
  // Small delay to let printer start
  delay(100);
  
  // Create animation task (UI) pinned to core 0
  xTaskCreatePinnedToCore(
    animationTask,
    "AnimationTask",
    4096,
    NULL,
    1,
    &animationTaskHandle,
    0
  );
}

// ===== PRINTER TASK (Runs in background) =====
void printerTask(void *parameter) {
  // Start printing
  isPrinting = true;
  printComplete = false;
  printProgress = 0;
  
  Serial.println(F("[Printer Task] Starting print..."));
  

  // Call the print function
  PrintJobFn job = g_printJobFn;
  if (job) {
    job();
  } else {
    printBill();
  }
  g_printJobFn = nullptr;
  
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
  // Reset local UI state for each print session
  g_printLoadingState = PrinterIconBarLoadingState{};

  int w = tft.width();
  int h = tft.height();
  uint32_t animationStartTime = millis();

  while (!printComplete) {
    // Keep old timing-based estimate, but do not exceed 95% until done.
    uint32_t elapsedTime = millis() - animationStartTime;
    int estimatedProgress = (int)((elapsedTime * 100UL) / WS_PRINT_ESTIMATED_DURATION_MS);
    if (estimatedProgress > 95) estimatedProgress = 95;

    // If some other module updates printProgress, respect it.
    int shownProgress = printProgress;
    if (shownProgress < estimatedProgress) shownProgress = estimatedProgress;
    if (shownProgress > 95) shownProgress = 95;

    printerIconBarScreenUpdate(
      tft,
      g_printLoadingState,
      w,
      h,
      shownProgress,
      false,
      COLOR_BG,
      TFT_WHITE,
      TFT_WHITE,
      TFT_WHITE,
      COLOR_AMOUNT
    );

    vTaskDelay(pdMS_TO_TICKS(30));
  }

  // Final frame (done)
  printerIconBarScreenUpdate(
    tft,
    g_printLoadingState,
    w,
    h,
    100,
    true,
    COLOR_BG,
    TFT_WHITE,
    TFT_WHITE,
    TFT_WHITE,
    COLOR_AMOUNT
  );

  Serial.println(F("[Animation] Print complete!"));
  vTaskDelay(pdMS_TO_TICKS(900));
  vTaskDelete(NULL);
}

// ===== START PARALLEL PRINTING =====
void startParallelPrinting() {
  startParallelPrintingInternal(nullptr);
}

// Start printing a custom job with the same loading animation (e.g., printReceipt)
void startParallelPrintingJob(PrintJobFn job) {
  startParallelPrintingInternal(job);
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
