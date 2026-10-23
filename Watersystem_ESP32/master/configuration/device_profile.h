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
  uint8_t rotation;
};

static const DeviceProfileOption DEVICE_PROFILE_OPTIONS[] = {
  {1, "Dona Josefa", "ESMERALDA S. REBONANZA", "e0:72:a1:6e:3f:ba", 1},
  {2, "Makilas", "DIESELYN BARANDA", "48:f6:ee:13:f6:ee", 1},
  {3, "Buluan", "DIOSDADO A. BALANSAG", "9c:cc:01:7d:38:62", 1},
  {4, "Caparan", "EXPEDITA R. BANAGUA", "ac:a7:04:d7:5d:0e", 3}
};

static const size_t DEVICE_PROFILE_OPTION_COUNT = sizeof(DEVICE_PROFILE_OPTIONS) / sizeof(DEVICE_PROFILE_OPTIONS[0]);

uint32_t g_deviceId = 0;
String g_collectorName;
String g_slaveMac;
String g_printerMac;
String g_printerName;

static Preferences g_deviceProfilePreferences;

static const DeviceProfileOption* defaultPrinterProfile() {
  return &DEVICE_PROFILE_OPTIONS[2]; // Buluan
}

static const DeviceProfileOption* findPrinterProfile(const String& mac) {
  for (size_t i = 0; i < DEVICE_PROFILE_OPTION_COUNT; ++i) {
    if (mac == DEVICE_PROFILE_OPTIONS[i].slaveMac) {
      return &DEVICE_PROFILE_OPTIONS[i];
    }
  }
  return nullptr;
}

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

void applyDeviceProfileRotation() {
  const DeviceProfileOption* option = findDeviceProfileOption(g_deviceId);
  tft.setRotation(option != nullptr ? option->rotation : 3);
}

bool saveDeviceProfile(uint32_t deviceId) {
  const DeviceProfileOption* option = findDeviceProfileOption(deviceId);
  if (option == nullptr) return false;

  // A new device/barangay selection must begin by printing to its paired
  // Slave.  Previously this only updated g_slaveMac, while g_printerMac kept
  // its old saved value (or the Buluan default), so a Makilas Master could
  // still connect to the Buluan printer.
  g_deviceProfilePreferences.putUInt("device_id", deviceId);
  g_deviceProfilePreferences.putString("slave_mac", option->slaveMac);
  g_deviceProfilePreferences.putString("collector", option->collector);
  g_deviceProfilePreferences.putString("printer_mac", option->slaveMac);
  g_deviceProfilePreferences.putString("printer_name", option->barangay);
  g_deviceProfilePreferences.putBool("configured", true);
  g_deviceId = deviceId;
  g_slaveMac = option->slaveMac;
  g_collectorName = option->collector;
  g_printerMac = option->slaveMac;
  g_printerName = option->barangay;
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

  const DeviceProfileOption* defaultPrinter = defaultPrinterProfile();
  g_printerMac = g_deviceProfilePreferences.getString("printer_mac", defaultPrinter->slaveMac);
  g_printerName = g_deviceProfilePreferences.getString("printer_name", defaultPrinter->barangay);
  const DeviceProfileOption* selectedPrinter = findPrinterProfile(g_printerMac);
  if (selectedPrinter == nullptr) {
    g_printerMac = defaultPrinter->slaveMac;
    g_printerName = defaultPrinter->barangay;
    g_deviceProfilePreferences.putString("printer_mac", g_printerMac);
    g_deviceProfilePreferences.putString("printer_name", g_printerName);
  }

  Serial.printf("[PROFILE] Device/Barangay ID: %lu\n", (unsigned long)g_deviceId);
  Serial.print(F("[PROFILE] Collector: "));
  Serial.println(g_collectorName);
  Serial.print(F("[PROFILE] Slave MAC: "));
  Serial.println(g_slaveMac);
  Serial.print(F("[PROFILE] Print target: "));
  Serial.println(g_printerName);
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

void clearDeviceProfile() {
  g_deviceProfilePreferences.clear();
  g_deviceId = 0;
  g_slaveMac = "";
  g_collectorName = "";
  g_printerMac = "";
  g_printerName = "";
}

bool savePrinterProfile(char key) {
  const DeviceProfileOption* option = nullptr;
  if (key >= '1' && key <= '4') {
    option = &DEVICE_PROFILE_OPTIONS[key - '1'];
  }
  if (option == nullptr) return false;

  g_printerMac = option->slaveMac;
  g_printerName = option->barangay;
  g_deviceProfilePreferences.putString("printer_mac", g_printerMac);
  g_deviceProfilePreferences.putString("printer_name", g_printerName);
  return true;
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
    applyDeviceProfileRotation();
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
