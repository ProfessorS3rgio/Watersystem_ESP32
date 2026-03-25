// ===== THERMAL PRINTER PINS (UART2) =====
#define PRINTER_RX  16   // ESP32 RX <- Printer TX
#define PRINTER_TX  17   // ESP32 TX -> Printer RX
#define PRINTER_BAUD 9600

// gate pin for IRLZ44N MOSFET controlling printer 5V supply
#define PRINTER_ENABLE_PIN 14  // drive high to power the printer, low to cut power

// watchdog-friendly yield macro used throughout the code
#ifndef YIELD_WDT
#define YIELD_WDT() yield()
#endif

// default collector name
#ifndef COLLECTOR_NAME_VALUE
#define COLLECTOR_NAME_VALUE ""
#endif