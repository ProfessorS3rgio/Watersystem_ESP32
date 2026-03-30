#ifndef PRINTER_POWER_MANAGER_H
#define PRINTER_POWER_MANAGER_H

#include <Arduino.h>
#include <esp32-hal-cpu.h>
#include "../configuration/config.h"
#include "../printer/printer_serial.h"

// External printer object is defined in slave.ino
extern ThermalPrinter printer;

// Printer power/runtime state
unsigned long g_printerLastActiveMs = 0;
bool g_printerIsEnabled = false;
volatile bool g_printJobInProgress = false;
bool g_lowPowerCpuMode = false;

void setCpuPowerMode(bool lowPower) {
    const int targetMhz = lowPower ? CPU_IDLE_MHZ : CPU_ACTIVE_MHZ;
    if (getCpuFrequencyMhz() != targetMhz) {
        setCpuFrequencyMhz(targetMhz);
        Serial.print("[PWR] CPU set to ");
        Serial.print(targetMhz);
        Serial.println(" MHz");
    }
    g_lowPowerCpuMode = lowPower;
}

void ensureActiveCpuMode() {
    if (g_lowPowerCpuMode) {
        setCpuPowerMode(false);
    }
}

void enablePrinter() {
    if (!g_printerIsEnabled) {
        printer.wake();
        printer.setDefault();
        g_printerIsEnabled = true;
        Serial.println("Printer powered on");
    }
    g_printerLastActiveMs = millis();
}

void disablePrinter() {
    if (g_printerIsEnabled) {
        printer.sleep();
        g_printerIsEnabled = false;
        Serial.println("Printer powered off (idle)");
    }
}

void checkPrinterIdle() {
    if (g_printerIsEnabled
        && static_cast<long>(millis() - g_printerLastActiveMs) >= static_cast<long>(PRINTER_IDLE_TIMEOUT_MS)) {
        disablePrinter();
    }
}

void beginPrintJob() {
    ensureActiveCpuMode();
    g_printJobInProgress = true;
    enablePrinter();
}

void endPrintJob() {
    g_printerLastActiveMs = millis();
    g_printJobInProgress = false;
}

ThermalPrinter::PaperStatus readPaperStatus(bool autoWakeIfNeeded, bool keepAwakeAfterQuery = false) {
    if (g_printJobInProgress) {
        return ThermalPrinter::PAPER_UNKNOWN;
    }

    const bool wasEnabled = g_printerIsEnabled;
    if (!wasEnabled && autoWakeIfNeeded) {
        enablePrinter();
        delay(80);
    }

    if (!g_printerIsEnabled) {
        return ThermalPrinter::PAPER_UNKNOWN;
    }

    ThermalPrinter::PaperStatus status = printer.queryPaperStatus(220);
    if (autoWakeIfNeeded) {
        g_printerLastActiveMs = millis();
    }

    if (!wasEnabled && g_printerIsEnabled && !keepAwakeAfterQuery) {
        disablePrinter();
    }

    return status;
}

#endif // PRINTER_POWER_MANAGER_H
