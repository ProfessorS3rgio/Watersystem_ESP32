// ===== THERMAL PRINTER PINS (UART2) =====
#define PRINTER_RX  17   // ESP32 RX <- Printer TX
#define PRINTER_TX  16   // ESP32 TX -> Printer RX
#define PRINTER_BAUD 9600

// watchdog-friendly yield macro used throughout the code
#ifndef YIELD_WDT
#define YIELD_WDT() yield()
#endif

// default collector name
#ifndef COLLECTOR_NAME_VALUE
#define COLLECTOR_NAME_VALUE ""
#endif