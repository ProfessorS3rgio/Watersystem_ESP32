#include <Arduino.h>
#include <ArduinoJson.h>
#include <BLEDevice.h>
#include <BLESecurity.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "printer/printer_serial.h"

#include <Wire.h> // I2C for fuel gauge
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h> // MAX17043

// fuel gauge object (defaults to MAX17043)
SFE_MAX1704X lipo;

// prototype used by bill_printer.h (defined later in this file)
String getCurrentDateTimeString();

#include "printer/bill_printer.h"
#include "printer/receipt_printer.h"
 
// database structures and globals (stubs for compilation)
#include "database/bill_database.h"          // gives BillData currentBill
#include "database/bill_transactions_database.h" // gives ReceiptData currentReceipt

BillData currentBill;
ReceiptData currentReceipt;

// serial objects used by printer helpers
HardwareSerial printerSerial = Serial2;   // matches ThermalPrinter printer(Serial2)

int getBillDueDaysSetting() { return 30; }
int getDisconnectionDaysSetting() { return 45; }

ThermalPrinter printer(Serial2);      // corresponds to HW UART2

// ---------- printer power management ----------
// track when we last used the printer; if it's idle for more
// than a minute we'll cut power using PRINTER_ENABLE_PIN.
unsigned long g_printerLastActiveMs = 0;
bool g_printerIsEnabled = false;

void enablePrinter() {
    if (!g_printerIsEnabled) {
        printer.wake();             // sets pin high and waits
        printer.setDefault();
        g_printerIsEnabled = true;
        Serial.println("Printer powered on");
    }
    g_printerLastActiveMs = millis();
}

void disablePrinter() {
    if (g_printerIsEnabled) {
        printer.sleep();            // drops pin low
        g_printerIsEnabled = false;
        Serial.println("Printer powered off (idle)");
    }
}

void checkPrinterIdle() {
    if (g_printerIsEnabled && static_cast<long>(millis() - g_printerLastActiveMs) >= 60000) {
        disablePrinter();
    }
}

ThermalPrinter::PaperStatus readPaperStatus(bool autoWakeIfNeeded) {
    bool wasEnabled = g_printerIsEnabled;
    if (!wasEnabled && autoWakeIfNeeded) {
        enablePrinter();
        delay(80);
    }

    if (!g_printerIsEnabled) {
        return ThermalPrinter::PAPER_UNKNOWN;
    }

    ThermalPrinter::PaperStatus status = printer.queryPaperStatus(220);
    g_printerLastActiveMs = millis();

    if (!wasEnabled && g_printerIsEnabled) {
        disablePrinter();
    }

    return status;
}

// ---------- charger detection ----------
// GPIO 23: NPN transistor collector, low when charger active (inverted logic)
#define CHARGER_PIN 23
bool g_lastChargingState = false;
unsigned long g_nextBatteryPrintMs = 0;

// --------------------------------------------------

// BLE service/characteristic UUIDs
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");

// forward declare parser/handler functions
void parseBillPayload(const String &payload);
void parseReceiptPayload(const String &payload);
void handleCommand(const String &cmd);
void printLogoOnly();

BLECharacteristic* pCharacteristic;
BLEServer* g_pServer = nullptr;
BLEAdvertising* g_pAdvertising = nullptr;
volatile bool g_bleClientConnected = false;
volatile bool g_bleRefreshRequested = false;
unsigned long g_bleRefreshAtMs = 0;
unsigned long g_nextPaperPollMs = 0;
ThermalPrinter::PaperStatus g_lastPaperStatus = ThermalPrinter::PAPER_UNKNOWN;

void scheduleBleRefresh(uint32_t delayMs = 250);
void refreshBleAdvertising();

// buffer for incoming BLE data
static String bleBuffer = "";

// callback when data is written by master
class CharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) {
    // on server, getValue() returns an Arduino String
    String val = characteristic->getValue();
    if (val.length() == 0) return;

    // accumulate in buffer and process lines
    bleBuffer += val;
    // if there is a newline, split
    int idx;
    while ((idx = bleBuffer.indexOf('\n')) != -1) {
      String line = bleBuffer.substring(0, idx);
      bleBuffer = bleBuffer.substring(idx + 1);
      line.trim();
      if (line.length() > 0) {
        Serial.print("Received command: ");
        Serial.println(line);
        handleCommand(line);
      }
    }
  }
};

void sendNotification(const String &msg) {
    if (pCharacteristic) {
        pCharacteristic->setValue(msg.c_str());
        pCharacteristic->notify();
    }
}

void sendNotificationLine(const String &msg) {
    sendNotification(msg + "\n");
}

void scheduleBleRefresh(uint32_t delayMs) {
    g_bleRefreshRequested = true;
    g_bleRefreshAtMs = millis() + delayMs;
}

void refreshBleAdvertising() {
    if (g_pAdvertising == nullptr) {
        g_pAdvertising = BLEDevice::getAdvertising();
        if (g_pAdvertising != nullptr) {
            g_pAdvertising->addServiceUUID(serviceUUID);
        }
    }

    if (g_pAdvertising == nullptr) {
        Serial.println("BLE advertising instance unavailable");
        return;
    }

    Serial.println("Refreshing BLE advertising...");
    g_pAdvertising->stop();
    delay(50);
    g_pAdvertising->start();
    Serial.println("BLE advertising restarted");
}

// --------------------------------------------------
// parsers and command handler
// --------------------------------------------------

static void splitString(const String &src, char delim, String dest[], int &count, int maxCount) {
    count = 0;
    int start = 0;
    while (count < maxCount) {
        int idx = src.indexOf(delim, start);
        if (idx == -1) {
            dest[count++] = src.substring(start);
            break;
        }
        dest[count++] = src.substring(start, idx);
        start = idx + 1;
    }
}

void parseBillPayload(const String &payload) {
    String p[20];
    int c;
    splitString(payload, '|', p, c, 20);
    if (c < 11) return; // not enough fields
    currentBill.refNumber      = p[0];
    currentBill.readingDateTime = p[1];
    currentBill.customerName   = p[2];
    currentBill.accountNo      = p[3];
    currentBill.customerType   = p[4];
    currentBill.address        = p[5];
    currentBill.collector      = p[6];
    currentBill.prevReading    = p[7].toInt();
    currentBill.currReading    = p[8].toInt();
    currentBill.rate           = p[9].toFloat();
    currentBill.subtotal       = p[10].toFloat();
    currentBill.deductions     = (c > 11 ? p[11].toFloat() : 0);
    currentBill.penalty        = (c > 12 ? p[12].toFloat() : 0);
    currentBill.total          = (c > 13 ? p[13].toFloat()
                                         : currentBill.subtotal - currentBill.deductions + currentBill.penalty);
    if (c > 14) currentBill.billDate = p[14];
}

void parseReceiptPayload(const String &payload) {
    String p[25];
    int c;
    splitString(payload, '|', p, c, 25);
    if (c < 13) return;
    currentReceipt.receiptNumber   = p[0];
    currentReceipt.paymentDateTime = p[1];
    currentReceipt.customerName    = p[2];
    currentReceipt.accountNo       = p[3];
    currentReceipt.customerType    = p[4];
    currentReceipt.address         = p[5];
    currentReceipt.collector       = p[6];
    currentReceipt.prevReading     = p[7].toInt();
    currentReceipt.currReading     = p[8].toInt();
    currentReceipt.rate            = p[9].toFloat();
    currentReceipt.subtotal        = p[10].toFloat();
    currentReceipt.deductions      = p[11].toFloat();
    currentReceipt.penalty         = p[12].toFloat();
    currentReceipt.total           = (c > 13 ? p[13].toFloat()
                                          : currentReceipt.subtotal - currentReceipt.deductions + currentReceipt.penalty);
    currentReceipt.amountPaid      = (c > 14 ? p[14].toFloat() : currentReceipt.total);
    currentReceipt.change          = (c > 15 ? p[15].toFloat() : 0);
}

void handleCommand(const String &cmd) {
    if (cmd.equalsIgnoreCase("PING")) {
        sendNotificationLine("PONG");
        Serial.println("PONG sent to master");
        return;
    }

    if (cmd.equalsIgnoreCase("PAPER_STATUS") || cmd.equalsIgnoreCase("CHECK_PAPER")) {
        ThermalPrinter::PaperStatus status = readPaperStatus(true);
        if (status == ThermalPrinter::PAPER_PRESENT) {
            sendNotificationLine("PAPER_PRESENT");
            Serial.println("PAPER_PRESENT sent to master");
        } else if (status == ThermalPrinter::PAPER_OUT) {
            sendNotificationLine("PAPER_OUT");
            Serial.println("PAPER_OUT sent to master");
        } else {
            sendNotificationLine("PAPER_UNKNOWN");
            Serial.println("PAPER_UNKNOWN sent to master");
        }
        return;
    }

    if (cmd.equalsIgnoreCase("HELLO") || cmd.startsWith("HELLO|")) {
        sendNotificationLine("CONNECTED");
        Serial.print("Handshake completed with: ");
        if (cmd.length() > 6) {
            Serial.println(cmd.substring(6));
        } else {
            Serial.println("unknown master");
        }
        return;
    }

    if (cmd.equalsIgnoreCase("BATTERY")) {
        double voltage = lipo.getVoltage();
        double soc = lipo.getSOC();
        bool alert = lipo.getAlert();
        char buf[64];
        snprintf(buf, sizeof(buf), "BATTERY V=%.2fV SOC=%.1f%% ALERT=%d", voltage, soc, alert ? 1 : 0);
        sendNotificationLine(String(buf));
        Serial.println(buf);
        return;
    }

    // try JSON first
    StaticJsonDocument<300> doc;
    DeserializationError err = deserializeJson(doc, cmd);
    if (!err) {
        const char *c = doc["cmd"];
        if (c != nullptr) {
            if (strcmp(c, "PRINT_BILL") == 0) {
                // TODO: copy additional fields from JSON into currentBill
                enablePrinter();
                printBill();
                Serial.println("printed bill (json)");
                return;
            }
            if (strcmp(c, "PRINT_RECEIPT") == 0) {
                enablePrinter();
                printReceipt();
                Serial.println("printed receipt (json)");
                return;
            }
        }
    }
    // fall back to legacy pipe-delimited format
    if (cmd.startsWith("PRINT_BILL")) {
        String payload = cmd.substring(strlen("PRINT_BILL") + 1);
        parseBillPayload(payload);
        enablePrinter();
        printBill();
        Serial.println("printed bill");
    } else if (cmd.startsWith("PRINT_RECEIPT")) {
        String payload = cmd.substring(strlen("PRINT_RECEIPT") + 1);
        parseReceiptPayload(payload);
        enablePrinter();
        printReceipt();
        Serial.println("printed receipt");
    } else {
        Serial.print("unknown command: ");
        Serial.println(cmd);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32 Slave BLE Server starting...");
    Serial.println("Serial commands: RESTART, PRN_SLEEP, PRN_WAKE, PRN_TEST, PRINT_LOGO, PAPER_STATUS, BATTERY");
    Serial.println("           (printer automatically powers off after 60s idle)");

    // initialize I2C and fuel gauge (SDA=GPIO26, SCL=GPIO27 on this board)
    Wire.begin(26, 27);
    if (!lipo.begin()) {
        Serial.println("MAX17043 not detected. Check I2C wiring (SDA=26, SCL=27) or power.");
    } else {
        Serial.println("MAX17043 detected");
        lipo.quickStart();
        lipo.setThreshold(20); // alert at 20% SOC, not currently read
    }

    // initialize MOSFET gate pin and keep printer powered off initially
    pinMode(PRINTER_ENABLE_PIN, OUTPUT);
    digitalWrite(PRINTER_ENABLE_PIN, LOW);
    g_printerIsEnabled = false;
    g_printerLastActiveMs = millis();

    // initialize charger detection pin
    pinMode(CHARGER_PIN, INPUT);
    g_lastChargingState = digitalRead(CHARGER_PIN);

    BLEDevice::init("ESP32_Slave");
    BLESecurity::setAuthenticationMode(false, false, false);
    BLEDevice::setPower(ESP_PWR_LVL_P9);
    
    class ServerCallbacks : public BLEServerCallbacks {
      void onConnect(BLEServer* pServer) {
                                        (void)pServer;
                    g_bleClientConnected = true;
                                        g_bleRefreshRequested = false;
          Serial.println("Master connected");
      }
      void onDisconnect(BLEServer* pServer) {
                    (void)pServer;
                    g_bleClientConnected = false;
                                        bleBuffer = "";
          Serial.println("Master disconnected");
                                        scheduleBleRefresh();
      }
    };
    
        g_pServer = BLEDevice::createServer();
        g_pServer->setCallbacks(new ServerCallbacks());
        BLEService *pService = g_pServer->createService(serviceUUID);
    pCharacteristic = pService->createCharacteristic(
                        charUUID,
                        BLECharacteristic::PROPERTY_WRITE |
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setCallbacks(new CharacteristicCallbacks());

    pService->start();
    g_pAdvertising = BLEDevice::getAdvertising();
    g_pAdvertising->addServiceUUID(serviceUUID);
    g_pAdvertising->start();
    Serial.println("Waiting for client to connect...");

    printer.begin();                 // init UART2 for printer
    g_nextPaperPollMs = millis() + 1000;
    // …other init (RTC, database, etc.)
}

void loop() {
    if (g_bleRefreshRequested && !g_bleClientConnected && static_cast<long>(millis() - g_bleRefreshAtMs) >= 0) {
        g_bleRefreshRequested = false;
        refreshBleAdvertising();
    }

    // power management for the printer: if idle for more than a minute
    // cut power. any command that uses the printer should call enablePrinter().
    checkPrinterIdle();

    // check charger state and notify if changed
    bool currentCharging = !digitalRead(CHARGER_PIN);  // invert: low = charging, high = not charging
    if (currentCharging != g_lastChargingState) {
        g_lastChargingState = currentCharging;
        Serial.print("Charger state changed: ");
        Serial.println(currentCharging ? "CHARGING" : "NOT CHARGING");
    }

    // periodically print battery status every 5 seconds
    if (static_cast<long>(millis() - g_nextBatteryPrintMs) >= 0) {
        g_nextBatteryPrintMs = millis() + 5000;
        double voltage = lipo.getVoltage();
        double soc = lipo.getSOC();
        bool alert = lipo.getAlert();
        Serial.print("Battery: ");
        Serial.print(voltage, 2);
        Serial.print("V, ");
        Serial.print(soc, 1);
        Serial.print("%, Alert: ");
        Serial.println(alert ? "YES" : "NO");
    }

    // handle serial monitor commands (must send newline/CR+LF from terminal)
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        Serial.print("Serial input: "); Serial.println(line); // echo for debugging
        if (line.length() == 0) {
            // nothing meaningful received, maybe no newline was sent
            Serial.println("(no command received, check line ending setting)");
        }
        if (line.equalsIgnoreCase("RESTART")) {
            Serial.println("Restarting device...");
            ESP.restart();
        } else if (line.equalsIgnoreCase("PRN_SLEEP")) {
            // explicitly power down immediately and clear timer
            disablePrinter();
            Serial.println("Printer sleep command executed (powered off)");
        } else if (line.equalsIgnoreCase("PRN_WAKE")) {
            // wake and start idle countdown
            enablePrinter();
            Serial.println("Printer wake command executed (powered on)");
        } else if (line.equalsIgnoreCase("PRN_TEST")) {
            // perform test print and restart idle countdown once complete
            enablePrinter();          // this updates g_printerLastActiveMs
            printer.println("[PRN_TEST] slave printer is awake");
            printer.feed(2);
            // also refresh timer explicitly in case enablePrinter was skipped
            g_printerLastActiveMs = millis();
            Serial.println("PRN_TEST line sent; idle timer reset");
        } else if (line.equalsIgnoreCase("PRINT_LOGO")) {
            enablePrinter();
            printLogoOnly();
            printer.feed(2);
            Serial.println("PRINT_LOGO done");
        } else if (line.equalsIgnoreCase("PAPER_STATUS")) {
            ThermalPrinter::PaperStatus status = readPaperStatus(true);
            if (status == ThermalPrinter::PAPER_OUT) {
                Serial.print("Paper status: OUT (raw=0x");
                Serial.print(printer.lastPaperRawStatus(), HEX);
                Serial.println(")");
            } else if (status == ThermalPrinter::PAPER_PRESENT) {
                Serial.print("Paper status: PRESENT (raw=0x");
                Serial.print(printer.lastPaperRawStatus(), HEX);
                Serial.println(")");
            } else {
                Serial.println("Paper status: UNKNOWN (no reply from printer)");
            }
        } else if (line.equalsIgnoreCase("BATTERY")) {
            double voltage = lipo.getVoltage();
            double soc = lipo.getSOC();
            bool alert = lipo.getAlert();
            Serial.print("Battery: ");
            Serial.print(voltage, 2);
            Serial.print("V, ");
            Serial.print(soc, 1);
            Serial.print("%, Alert: ");
            Serial.println(alert ? "YES" : "NO");
        }
    }

    if (static_cast<long>(millis() - g_nextPaperPollMs) >= 0) {
        g_nextPaperPollMs = millis() + 1000;
        if (!g_printerIsEnabled) {
            if (g_lastPaperStatus != ThermalPrinter::PAPER_UNKNOWN) {
                g_lastPaperStatus = ThermalPrinter::PAPER_UNKNOWN;
                Serial.println("[PAPER] PRINTER_OFF (poll skipped)");
            }
            delay(10);
            return;
        }

        ThermalPrinter::PaperStatus status = readPaperStatus(false);
        if (status != g_lastPaperStatus) {
            g_lastPaperStatus = status;
            if (status == ThermalPrinter::PAPER_OUT) {
                Serial.print("[PAPER] OUT (raw=0x");
                Serial.print(printer.lastPaperRawStatus(), HEX);
                Serial.println(")");
            } else if (status == ThermalPrinter::PAPER_PRESENT) {
                Serial.print("[PAPER] PRESENT (raw=0x");
                Serial.print(printer.lastPaperRawStatus(), HEX);
                Serial.println(")");
            } else {
                Serial.println("[PAPER] UNKNOWN (no reply)");
            }
        }
    }

    // nothing else to do; work happens in callback
    delay(10);
}

// ---------------------------------------------------------------------------
// stub helpers needed by printer modules
// ---------------------------------------------------------------------------

String getCurrentDateTimeString() {
    // return a fixed date/time for testing
    return "2026-02-23 12:00:00";
}

String calculateDueDate(String billDate, int daysToAdd) {
    return billDate;
}

void printLogoOnly() {
    // Keep this focused: print just the company logo and leave alignment/defaults sane.
    printer.setDefault();
    printer.setBitmapDarkProfile();
    printer.justify('C');
    printer.printBitmap(LOGO_WIDTH, LOGO_HEIGHT, logo);
    printer.justify('L');
    printer.setDefault();
    g_printerLastActiveMs = millis();
}
