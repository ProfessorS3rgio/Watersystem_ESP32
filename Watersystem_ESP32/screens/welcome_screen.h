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
  tft.fillRect(0, 0, 240, 10, TFT_RED);
  
  // ===== DISPLAY LOGO FROM SD CARD =====
  // Draw logo from BMP file on the left side
  int logoX = 20;  // Left side
  int logoY = 25;
  drawBMP("/WATER_DB/ASSETS/dmbc_logo.bmp", logoX, logoY);
  
  // ===== SERVICE DESCRIPTION (Next to logo) =====
  tft.setTextFont(4);  // Sans-serif font, similar to FreeSansBold
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(80, 55);
  tft.println(F("Water &"));
  tft.setCursor(80, 70);
  tft.println(F("Sanitation"));
  tft.setCursor(80, 85);
  tft.println(F("Association"));
  
  // ===== DECORATIVE ELEMENTS =====
  
  // ===== STATUS INDICATOR =====
  tft.setTextFont(2);  // Sans-serif font, similar to FreeSans
  tft.setTextSize(1);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(70, 130);  // Centered
  tft.println(F("System Ready"));
  
  // ===== BATTERY INDICATOR =====
  drawBattery(200, 15, 100);  // x, y, battery level (0-100)
  
  // ===== ADDITIONAL DESIGN ELEMENTS =====
  
  // ===== INSTRUCTIONS =====
  tft.setTextFont(2);  // Sans-serif font, similar to FreeSans
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(30, 280);  // Centered in the box
  tft.println(F("A - Menu   B - Bill   D - Payment"));
  
  // ===== BOTTOM BORDER =====
  tft.fillRect(0, 310, 240, 10, TFT_RED);
  
}

#endif // WELCOME_SCREEN_H