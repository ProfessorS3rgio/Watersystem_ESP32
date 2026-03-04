#ifndef WELCOME_SCREEN_H
#define WELCOME_SCREEN_H

#include "../configuration/config.h"
#include "components/bmp_display.h"
#include "components/battery_display.h"

// ===== EXTERNAL OBJECTS =====
extern TFT_eSPI tft;
extern BatteryMonitor batteryMonitor;
extern RTC_DS3231 rtc;  // real-time clock

void updateWelcomeBatteryStatus(int batteryPct) {

  // ===== CLEAR HEADER AREA (TIME + BATTERY ICON) =====
  tft.fillRect(0, 5, 320, 25, TFT_BLACK);

  // ===== CLEAR CENTER DATE/TIME AREA =====
  tft.fillRect(80, 120, 200, 50, TFT_BLACK);

  // ===== GET RTC TIME =====
  DateTime now = rtc.now();

  char dateBuf[20];
  char timeBuf[20];

  // Format Date: MM/DD/YYYY
  sprintf(dateBuf, "%02d/%02d/%04d",
          now.month(),
          now.day(),
          now.year());

  // Format Time: 12-hour format
  int hour12 = now.hour() % 12;
  if (hour12 == 0) hour12 = 12;

  sprintf(timeBuf, "%d:%02d%s",
          hour12,
          now.minute(),
          now.hour() >= 12 ? "PM" : "AM");

  // ===== DISPLAY DATE (CENTERED LOOK) =====
  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(110, 135);
  tft.println(dateBuf);

  // ===== DISPLAY TIME BELOW DATE (use yellow color) =====
  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(125, 160);
  tft.println(timeBuf);

  // ===== KEEP BATTERY ICON ON TOP RIGHT =====
  drawBattery(258, 20, batteryMonitor);
}


void showWelcomeScreen() {
  tft.fillScreen(TFT_BLACK);

  // ===== TOP BORDER =====
  tft.fillRect(0, 0, 320, 5, TFT_BLUE);

  // ===== DISPLAY LOGO FROM SD CARD =====
  int logoX = 20;
  int logoY = 25;
  drawBMP("/WATER_DB/ASSETS/dmbc_logo.bmp", logoX, logoY);

  // ===== SERVICE DESCRIPTION =====
  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_TEXT);

  tft.setCursor(140, 55);
  tft.println(F("Water &"));

  tft.setCursor(140, 70);
  tft.println(F("Sanitation"));

  tft.setCursor(140, 85);
  tft.println(F("Association"));


  // ===== BATTERY + TIME =====
  updateWelcomeBatteryStatus(batteryMonitor.getPercentage());

  // ===== INSTRUCTIONS =====
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(30, 200);
  tft.println(F("A - Menu   B - Bill   D - Payment"));

  // ===== BOTTOM BORDER =====
  tft.fillRect(0, 230, 320, 5, TFT_BLUE);
}

#endif // WELCOME_SCREEN_H