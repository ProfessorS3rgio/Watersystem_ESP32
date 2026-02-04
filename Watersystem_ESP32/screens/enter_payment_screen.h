#ifndef ENTER_PAYMENT_SCREEN_H
#define ENTER_PAYMENT_SCREEN_H

#include "../configuration/config.h"

// ===== EXTERNAL OBJECTS =====
extern TFT_eSPI tft;
extern String inputBuffer;

void displayEnterPaymentScreen() {
  tft.fillScreen(COLOR_BG);
  
  
  tft.setTextFont(4);  // Sans-serif font for header
  tft.setTextSize(1);
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(80, 20);
  tft.println(F("ENTER PAYMENT"));
  
  tft.drawLine(0, 30, 320, 30, COLOR_LINE);
  
  tft.setTextFont(2);  // Sans-serif font for labels
  tft.setTextSize(1);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(20, 50);
  tft.println(F("Payment Amount (PHP):"));
  
  // Input box
  tft.drawRect(20, 60, 280, 60, COLOR_LINE);
  
  tft.setTextFont(4);  // Sans-serif font for input
  tft.setTextSize(2);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(55, 100);
  if (inputBuffer.length() > 0) {
    tft.print(F("P"));
    tft.println(inputBuffer);
  } else {
    // Placeholder
    tft.setTextColor(COLOR_LABEL);
    tft.println(F("___________"));
  }
  
  tft.setTextFont(2);  // Sans-serif font for instructions
  tft.setTextSize(1);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(35, 180);
  tft.println(F("D-Confirm  B-Clear  C-Cancel"));
  

}

#endif // ENTER_PAYMENT_SCREEN_H