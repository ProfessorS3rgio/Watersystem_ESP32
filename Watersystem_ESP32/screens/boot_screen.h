#ifndef BOOT_SCREEN_H
#define BOOT_SCREEN_H

#include <TFT_eSPI.h>
#include "Adafruit_Thermal.h"
#include "../managers/sdcard_manager.h"
#include "../configuration/config.h"

// Extern declarations for global objects
extern TFT_eSPI tft;
extern HardwareSerial printerSerial;
extern Adafruit_Thermal printer;

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
  tft.setTextColor(TFT_GREEN);
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
    tft.setTextColor(TFT_GREEN);
    tft.println(F("OK"));
  } else {
    tft.setTextColor(TFT_RED);
    tft.println(F("FAIL"));
  }
  tft.setTextColor(TFT_GREEN);

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
    tft.setTextColor(TFT_GREEN);
    tft.println(F("OK"));
  } else {
    tft.setTextColor(TFT_RED);
    tft.println(F("NO RESP"));
  }
  tft.setTextColor(TFT_GREEN);

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

#endif  // BOOT_SCREEN_H