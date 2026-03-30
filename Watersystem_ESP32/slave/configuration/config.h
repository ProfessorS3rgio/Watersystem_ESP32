// ===== THERMAL PRINTER PINS (UART2) =====
#define PRINTER_RX  16   // ESP32 RX <- Printer TX
#define PRINTER_TX  17   // ESP32 TX -> Printer RX
#define PRINTER_BAUD 9600

// gate pin for IRLZ44N MOSFET controlling printer 5V supply
#define PRINTER_ENABLE_PIN 14  // drive high to power the printer, low to cut power

// printer auto-sleep timeout (minutes)
// Set to 3 minutes for testing; increase for production use.
#ifndef PRINTER_IDLE_TIMEOUT_MINUTES
#define PRINTER_IDLE_TIMEOUT_MINUTES 3UL
#endif

#ifndef PRINTER_IDLE_TIMEOUT_MS
#define PRINTER_IDLE_TIMEOUT_MS (PRINTER_IDLE_TIMEOUT_MINUTES * 60UL * 1000UL)
#endif

// CPU frequency profile for low-power operation on slave.
// Keep 240 MHz for BLE/printing responsiveness and use 80 MHz at idle for BLE stability.
#ifndef CPU_ACTIVE_MHZ
#define CPU_ACTIVE_MHZ 240
#endif

#ifndef CPU_IDLE_MHZ
#define CPU_IDLE_MHZ 80
#endif

// watchdog-friendly yield macro used throughout the code
#ifndef YIELD_WDT
#define YIELD_WDT() yield()
#endif

// default collector name
#ifndef COLLECTOR_NAME_VALUE
#define COLLECTOR_NAME_VALUE ""
#endif