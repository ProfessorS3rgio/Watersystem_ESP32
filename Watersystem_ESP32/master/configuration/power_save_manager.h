#ifndef POWER_SAVE_MANAGER_H
#define POWER_SAVE_MANAGER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "printer/printer_serial.h"
#include <esp32-hal-cpu.h>

struct PowerSaveManager {
  ThermalPrinter* printer;
  uint8_t backlightPin;
  uint32_t timeoutMs;
  uint8_t dimBrightness;
  uint8_t fullBrightness;
  uint8_t powerSaveCpuMhz;
  uint8_t fullCpuMhz;
  volatile bool enabled;
  volatile bool inPowerSave;
  volatile bool waitingForWakeKey;
  volatile uint32_t lastActivityMs;
  TaskHandle_t taskHandle;
  bool initialized;
};

static PowerSaveManager g_powerSave = {
  nullptr,
  0,
  0,
  25,
  255,
  40,  // powerSaveCpuMhz
  240, // fullCpuMhz
  true,
  false,
  false,
  0,
  nullptr,
  false
};

static void powerSaveApplyBacklight(bool on, uint8_t levelWhenOn = 255) {
  uint8_t duty = 0;
  if (on) {
    duty = levelWhenOn;
  } else {
    duty = 0;
  }

#if TFT_BACKLIGHT_ACTIVE_HIGH
  uint8_t pwmValue = duty;
  uint8_t digitalValue = on ? HIGH : LOW;
#else
  uint8_t pwmValue = 255 - duty;
  uint8_t digitalValue = on ? LOW : HIGH;
#endif

  ledcWrite(g_powerSave.backlightPin, pwmValue);
  pinMode(g_powerSave.backlightPin, OUTPUT);
  digitalWrite(g_powerSave.backlightPin, digitalValue);

  Serial.print(F("[PowerSave] BLK apply on="));
  Serial.print(on ? 1 : 0);
  Serial.print(F(" duty="));
  Serial.print(duty);
  Serial.print(F(" pwm="));
  Serial.print(pwmValue);
  Serial.print(F(" dig="));
  Serial.println(digitalValue == HIGH ? F("HIGH") : F("LOW"));
}

void IRAM_ATTR powerSaveKeypadISR() {
  // Not used in logic-only power save mode.
}

void powerSaveNotifyActivity(const char* source = nullptr) {
  g_powerSave.lastActivityMs = millis();
  if (source != nullptr) {
    Serial.print(F("[PowerSave] Activity: "));
    Serial.println(source);
  }
}

bool powerSaveIsActive() {
  return g_powerSave.inPowerSave;
}

bool powerSaveIsWaitingForWakeKey() {
  return g_powerSave.waitingForWakeKey;
}

bool powerSaveConsumeWakeKey(char key) {
  if (!g_powerSave.waitingForWakeKey) {
    return true;
  }

  if (key == 'D') {
    Serial.print(F("[PowerSave] Wake key accepted: "));
    Serial.println(key);
    // waking printer disabled; keep it powered during power save
    // if (g_powerSave.printer != nullptr) {
    //   Serial.println(F("[PowerSave] Waking printer"));
    //   g_powerSave.printer->wake();
    // }

    Serial.println(F("[PowerSave] Restoring CPU frequency"));
    setCpuFrequencyMhz(g_powerSave.fullCpuMhz);
    Serial.println(F("[PowerSave] Turning backlight ON (BLK)"));
    powerSaveApplyBacklight(true, g_powerSave.fullBrightness);
    g_powerSave.inPowerSave = false;
    g_powerSave.waitingForWakeKey = false;
    g_powerSave.lastActivityMs = millis();
    Serial.println(F("[PowerSave] Active mode restored"));
    return false;
  }

  Serial.print(F("[PowerSave] Wake key ignored: "));
  Serial.println(key);
  Serial.println(F("[PowerSave] Waiting for D to unlock"));
  return false;
}

void powerSaveSetEnabled(bool enabled) {
  if (g_powerSave.enabled == enabled) {
    return;
  }

  g_powerSave.enabled = enabled;
  g_powerSave.lastActivityMs = millis();

  if (!enabled) {
    if (g_powerSave.waitingForWakeKey) {
      g_powerSave.waitingForWakeKey = false;
      g_powerSave.inPowerSave = false;
      Serial.println(F("[PowerSave] Turning backlight ON (BLK)"));
      powerSaveApplyBacklight(true, g_powerSave.fullBrightness);
      Serial.println(F("[PowerSave] Wake lock cleared (workflow active)"));
    }
    Serial.println(F("[PowerSave] Disabled for active workflow"));
  } else {
    Serial.println(F("[PowerSave] Enabled for idle workflow"));
  }
}

void powerSaveTask(void* parameter) {
  (void)parameter;

  const TickType_t pollInterval = pdMS_TO_TICKS(200);

  for (;;) {
    if (!g_powerSave.initialized) {
      vTaskDelay(pollInterval);
      continue;
    }

    if (!g_powerSave.enabled) {
      vTaskDelay(pollInterval);
      continue;
    }

    const uint32_t elapsed = millis() - g_powerSave.lastActivityMs;
    if (!g_powerSave.inPowerSave && elapsed >= g_powerSave.timeoutMs) {
      g_powerSave.inPowerSave = true;
      g_powerSave.waitingForWakeKey = true;

      Serial.println(F("[PowerSave] Inactivity timeout reached"));
      Serial.print(F("[PowerSave] Elapsed ms: "));
      Serial.println(elapsed);

      Serial.println(F("[PowerSave] Reducing CPU frequency"));
      setCpuFrequencyMhz(g_powerSave.powerSaveCpuMhz);
      Serial.println(F("[PowerSave] Dimming screen"));
      Serial.println(F("[PowerSave] Turning backlight OFF (BLK)"));
      powerSaveApplyBacklight(false, g_powerSave.dimBrightness);

      // printer sleep disabled - hardware may be needed while idle
      // Serial.println(F("Putting printer to sleep"));
      // if (g_powerSave.printer != nullptr) {
      //   g_powerSave.printer->sleep();
      // }

      Serial.println(F("[PowerSave] Press D to unlock"));
    }

    vTaskDelay(pollInterval);
  }
}

void powerSaveBegin(ThermalPrinter* printer,
                    uint8_t backlightPin,
                    uint32_t timeoutMs,
                    uint8_t dimBrightness = 25,
                    uint8_t fullBrightness = 255) {
  g_powerSave.printer = printer;
  g_powerSave.backlightPin = backlightPin;
  g_powerSave.timeoutMs = timeoutMs;
  g_powerSave.dimBrightness = dimBrightness;
  g_powerSave.fullBrightness = fullBrightness;
  g_powerSave.enabled = true;
  g_powerSave.lastActivityMs = millis();
  g_powerSave.inPowerSave = false;
  g_powerSave.waitingForWakeKey = false;
  g_powerSave.initialized = true;

  powerSaveApplyBacklight(true, g_powerSave.fullBrightness);
  setCpuFrequencyMhz(g_powerSave.fullCpuMhz);

  if (g_powerSave.taskHandle == nullptr) {
    xTaskCreatePinnedToCore(
      powerSaveTask,
      "PowerSaveTask",
      4096,
      nullptr,
      1,
      &g_powerSave.taskHandle,
      1
    );
  }

  Serial.print(F("[PowerSave] Started with timeout(ms): "));
  Serial.println(timeoutMs);
}

#endif
