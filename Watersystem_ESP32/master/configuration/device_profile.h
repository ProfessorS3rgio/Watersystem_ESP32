#ifndef DEVICE_PROFILE_H
#define DEVICE_PROFILE_H

#include <Arduino.h>
#include <Preferences.h>
#include <TFT_eSPI.h>

extern TFT_eSPI tft;
char getKey();

struct DeviceProfileOption {
  uint32_t id;
  const char* barangay;
  const char* collector;
  const char* slaveMac;
};

static const DeviceProfileOption DEVICE_PROFILE_OPTIONS[] = {
  {1, "Dona Josefa", "ESMERALDA S. REBONANZA", "e0:72:a1:6e:3f:ba"},
  {2, "Makilas", "DIESERHEY BARANDA", "d4:05:92:e6:a7:ee"},
  {3, "Buluan", "DIOSDADO A. BALANSAG", "e0:72:a1:6e:3f:ba"},
  {4, "Caparan", "EXPEDITA R. BANAGUA", "ac:a7:04:d7:5d:0e"}
};

static const size_t DEVICE_PROFILE_OPTION_COUNT = sizeof(DEVICE_PROFILE_OPTIONS) / sizeof(DEVICE_PROFILE_OPTIONS[0]);

uint32_t g_deviceId = 0;
String g_collectorName;
String g_slaveMac;

static Preferences g_deviceProfilePreferences;

static bool isValidBleMac(const String& mac) {
  if (mac.length() != 17) return false;
  for (int i = 0; i < 17; ++i) {
    if ((i + 1) % 3 == 0) {
      if (mac[i] != ':') return false;
    } else if (!isxdigit(mac[i])) {
      return false;
    }
  }
  return true;
}

static const DeviceProfileOption* findDeviceProfileOption(uint32_t deviceId) {
  for (size_t i = 0; i < DEVICE_PROFILE_OPTION_COUNT; ++i) {
    if (DEVICE_PROFILE_OPTIONS[i].id == deviceId) {
      return &DEVICE_PROFILE_OPTIONS[i];
    }
  }
  return nullptr;
}

bool saveDeviceProfile(uint32_t deviceId) {
  const DeviceProfileOption* option = findDeviceProfileOption(deviceId);
  if (option == nullptr) return false;

  g_deviceProfilePreferences.putUInt("device_id", deviceId);
  g_deviceProfilePreferences.putString("slave_mac", option->slaveMac);
  g_deviceProfilePreferences.putString("collector", option->collector);
  g_deviceProfilePreferences.putBool("configured", true);
  g_deviceId = deviceId;
  g_slaveMac = option->slaveMac;
  g_collectorName = option->collector;
  return true;
}

void loadDeviceProfile() {
  g_deviceProfilePreferences.begin("device-profile", false);

  const bool hasProfile = g_deviceProfilePreferences.getBool("configured", false);
  if (!hasProfile) {
    Serial.println(F("[PROFILE] No saved profile; setup required"));
  } else {
    g_deviceId = g_deviceProfilePreferences.getUInt("device_id", 0);
    g_slaveMac = g_deviceProfilePreferences.getString("slave_mac", "");
    g_collectorName = g_deviceProfilePreferences.getString("collector", "");
    const DeviceProfileOption* option = findDeviceProfileOption(g_deviceId);
    if (option == nullptr || g_slaveMac != option->slaveMac || g_collectorName != option->collector) {
      Serial.println(F("[PROFILE] Saved profile is invalid or mismatched; setup required"));
    }
  }

  Serial.printf("[PROFILE] Device/Barangay ID: %lu\n", (unsigned long)g_deviceId);
  Serial.print(F("[PROFILE] Collector: "));
  Serial.println(g_collectorName);
  Serial.print(F("[PROFILE] Slave MAC: "));
  Serial.println(g_slaveMac);
}

String deviceProfileStatus() {
  return String(g_deviceId) + " " + g_slaveMac + " " + g_collectorName;
}

bool deviceProfileIsConfigured() {
  const DeviceProfileOption* option = findDeviceProfileOption(g_deviceId);
  return g_deviceProfilePreferences.getBool("configured", false)
      && option != nullptr
      && g_slaveMac == option->slaveMac
      && g_collectorName == option->collector;
}

void displayDeviceProfileSelector() {
  tft.fillScreen(COLOR_BG);
  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_HEADER);
  tft.setCursor(28, 20);
  tft.println(F("SELECT BARANGAY"));
  tft.drawLine(0, 30, 320, 30, COLOR_LINE);

  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_TEXT);
  for (size_t i = 0; i < DEVICE_PROFILE_OPTION_COUNT; ++i) {
    tft.setCursor(20, 60 + (i * 32));
    tft.print(DEVICE_PROFILE_OPTIONS[i].id);
    tft.print(F(" - "));
    tft.println(DEVICE_PROFILE_OPTIONS[i].barangay);
  }
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(25, 210);
  tft.println(F("Enter 1-4 to configure"));
}

void configureDeviceProfileAtBoot(bool forceSelection = false) {
  if (!forceSelection && deviceProfileIsConfigured()) return;

  displayDeviceProfileSelector();
  while (!deviceProfileIsConfigured()) {
    const char key = getKey();
    if (key < '1' || key > '4') {
      delay(20);
      continue;
    }

    const uint32_t selectedId = static_cast<uint32_t>(key - '0');
    saveDeviceProfile(selectedId);
    tft.fillScreen(COLOR_BG);
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(25, 75);
    tft.println(F("Device configured"));
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(25, 100);
    tft.println(g_collectorName);
    delay(1200);
  }
}

#endif // DEVICE_PROFILE_H