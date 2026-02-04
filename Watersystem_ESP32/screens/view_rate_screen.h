#ifndef VIEW_RATE_SCREEN_H
#define VIEW_RATE_SCREEN_H

#include "../configuration/config.h"
#include "../database/customer_type_database.h"

// ===== EXTERNAL OBJECTS =====
extern TFT_eSPI tft;

void displayViewRateScreen() {
  tft.fillScreen(COLOR_BG);
  
  tft.setTextColor(COLOR_HEADER);
  tft.setTextSize(1);
  tft.setCursor(25, 5);
  tft.println(F("CUSTOMER RATES"));
  
  tft.drawLine(0, 16, 160, 16, COLOR_LINE);
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(2, 22);
  tft.println(F("Type | Rate/m3 | Min m3"));
  
  tft.drawLine(0, 34, 160, 34, COLOR_LINE);
  
  int y = 40;
  for (int i = 0; i < getCustomerTypeCount() && y < 120; i++) {
    CustomerType* type = getCustomerTypeAt(i);
    if (type) {
      tft.setTextColor(COLOR_TEXT);
      tft.setCursor(2, y);
      tft.print(type->type_name);
      tft.setCursor(60, y);
      tft.print(F("P"));
      tft.print(type->rate_per_m3, 2);
      tft.setCursor(110, y);
      tft.print(type->min_m3);
      y += 12;
    }
  }
  
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(28, 118);
  tft.println(F("Press C to exit"));
}

#endif // VIEW_RATE_SCREEN_H