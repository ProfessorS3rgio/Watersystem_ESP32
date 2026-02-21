#ifndef WELCOME_SCREEN_H
#define WELCOME_SCREEN_H

#include "../configuration/config.h"
#include "components/bmp_display.h"
#include "components/battery_display.h"

// ===== EXTERNAL OBJECTS =====
extern TFT_eSPI tft;
extern BatteryMonitor batteryMonitor;

void updateWelcomeBatteryStatus(int batteryPct) {
  tft.fillRect(80, 136, 170, 22, TFT_BLACK);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(85, 150);
  if (batteryMonitor.isCharging()) {
    tft.printf("Charging: %d%%", batteryPct);
  } else {
    tft.printf("Battery: %d%%", batteryPct);
  }

  drawBattery(258, 15, batteryMonitor);  // x, y, battery monitor
}

void showWelcomeScreen() {
  tft.fillScreen(TFT_BLACK);  // Ensure black background
  
  // ===== TOP BORDER =====
  tft.fillRect(0, 0, 320, 5, TFT_BLUE);
  
  // ===== DISPLAY LOGO FROM SD CARD =====
  // Draw logo from BMP file on the left side
  int logoX = 20;  // Left side
  int logoY = 25;
  drawBMP("/WATER_DB/ASSETS/dmbc_logo.bmp", logoX, logoY);
  
  // ===== SERVICE DESCRIPTION (Next to logo) =====
  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(140, 55);
  tft.println(F("Water &"));
  tft.setCursor(140, 70);
  tft.println(F("Sanitation"));
  tft.setCursor(140, 85);
  tft.println(F("Association"));
  
  // ===== DECORATIVE ELEMENTS =====
  
  // ===== STATUS INDICATOR =====
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(100, 130);  // Centered
  tft.println(F("System Ready"));
  
  // ===== BATTERY STATUS =====
  updateWelcomeBatteryStatus(batteryMonitor.getPercentage());
  
  // ===== ADDITIONAL DESIGN ELEMENTS =====
  
  // ===== INSTRUCTIONS =====
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(30, 200);  // Centered in the box
  tft.println(F("A - Menu   B - Bill   D - Payment"));
  
  // ===== BOTTOM BORDER =====
  tft.fillRect(0, 230, 320, 5, TFT_BLUE);
  
}

#endif // WELCOME_SCREEN_H