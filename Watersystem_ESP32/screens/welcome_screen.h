#ifndef WELCOME_SCREEN_H
#define WELCOME_SCREEN_H

#include "../configuration/config.h"
#include "components/bmp_display.h"
#include "components/battery_display.h"

// ===== EXTERNAL OBJECTS =====
extern TFT_eSPI tft;

void showWelcomeScreen() {
  tft.fillScreen(TFT_BLACK);  // Ensure black background
  
  // ===== TOP BORDER =====
  tft.fillRect(0, 0, 320, 10, TFT_RED);
  
  // ===== DISPLAY LOGO FROM SD CARD =====
  // Draw logo from BMP file on the left side
  int logoX = 20;  // Left side
  int logoY = 25;
  drawBMP("/WATER_DB/ASSETS/water_logo3.bmp", logoX, logoY);
  
  // ===== SERVICE DESCRIPTION (Next to logo) =====
  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(140, 35);
  tft.println(F("Water &"));
  tft.setCursor(140, 50);
  tft.println(F("Sanitation"));
  tft.setCursor(140, 65);
  tft.println(F("Association"));
  
  // ===== DECORATIVE ELEMENTS =====
  
  // ===== STATUS INDICATOR =====
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(100, 120);  // Centered
  tft.println(F("System Ready"));
  
  // ===== BATTERY INDICATOR =====
  drawBattery(120, 150, 100);  // x, y, battery level (0-100)
  
  // ===== ADDITIONAL DESIGN ELEMENTS =====
  
  // ===== INSTRUCTIONS =====
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(50, 185);  // Centered in the box
  tft.println(F("A - Menu"));
  tft.setCursor(50, 200);
  tft.println(F("D - Cash Payment"));
  tft.setCursor(50, 215);
  tft.println(F("B - Bill Release"));
  
  // ===== BOTTOM BORDER =====
  tft.fillRect(0, 230, 320, 10, TFT_RED);
  
}

#endif // WELCOME_SCREEN_H