#ifndef DMBC_WIRELESS_SYNC_SCREEN_H
#define DMBC_WIRELESS_SYNC_SCREEN_H
#include "../managers/wireless_sync_manager.h"
extern TFT_eSPI tft;

static void displayWirelessSyncScreen() {
  tft.fillScreen(COLOR_BG);
  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(62, 22); tft.println(F("WIRELESS SYNC"));
  tft.drawLine(0, 32, 320, 32, COLOR_LINE);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(12, 57); tft.println(F("1. Join this Wi-Fi on laptop:"));
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(12, 81); tft.println(g_wirelessSsid);
  tft.setCursor(12, 105); tft.println(F("Open network - no password"));
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(12, 134); tft.println(F("2. Open Sync Workspace > Wi-Fi"));
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(12, 158); tft.print(F("Pair code: ")); tft.println(g_wirelessToken);
  tft.setCursor(12, 182); tft.println(F("Device: 192.168.4.1"));
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(12, 216); tft.println(F("C / # = Exit. Idle timeout: 10m"));
}
#endif
