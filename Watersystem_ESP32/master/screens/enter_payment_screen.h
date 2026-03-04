#ifndef ENTER_PAYMENT_SCREEN_H
#define ENTER_PAYMENT_SCREEN_H

#include "../configuration/config.h"

// ===== EXTERNAL OBJECTS =====
extern TFT_eSPI tft;
extern String inputBuffer;

void displayEnterPaymentScreen() {
  tft.fillScreen(COLOR_BG);

  // Match Enter Reading screen styling
  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(90, 20);
  tft.println(F("ENTER PAYMENT"));

  tft.drawLine(0, 30, 320, 30, COLOR_LINE);

  // Prompt
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(20, 75);
  tft.println(F("Payment Amount (PHP):"));

  // Input box (same position/size as reading screen)
  tft.drawRect(20, 85, 280, 60, COLOR_LINE);

  tft.setFreeFont(&FreeSans18pt7b);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(55, 125);
  if (inputBuffer.length() > 0) {
    tft.print(F("P"));
    tft.println(inputBuffer);
  } else {
    tft.setTextColor(COLOR_LABEL);
    tft.println(F("_____"));
  }

  // Footer controls
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(35, 180);
  tft.println(F("D-Confirm  B-Clear  C-Cancel"));

}

#endif // ENTER_PAYMENT_SCREEN_H