#include <Arduino.h>
#include <ArduinoJson.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "printer/printer_serial.h"

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

// BLE service/characteristic UUIDs
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");

// forward declare parser/handler functions
void parseBillPayload(const String &payload);
void parseReceiptPayload(const String &payload);
void handleCommand(const String &cmd);

BLECharacteristic* pCharacteristic;

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
    // try JSON first
    StaticJsonDocument<300> doc;
    DeserializationError err = deserializeJson(doc, cmd);
    if (!err) {
        const char *c = doc["cmd"];
        if (c != nullptr) {
            if (strcmp(c, "PRINT_BILL") == 0) {
                // TODO: copy additional fields from JSON into currentBill
                printBill();
                Serial.println("printed bill (json)");
                return;
            }
            if (strcmp(c, "PRINT_RECEIPT") == 0) {
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
        printBill();
        Serial.println("printed bill");
    } else if (cmd.startsWith("PRINT_RECEIPT")) {
        String payload = cmd.substring(strlen("PRINT_RECEIPT") + 1);
        parseReceiptPayload(payload);
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

    BLEDevice::init("ESP32_Slave");
    
    class ServerCallbacks : public BLEServerCallbacks {
      void onConnect(BLEServer* pServer) {
          Serial.println("Master connected");
      }
      void onDisconnect(BLEServer* pServer) {
          Serial.println("Master disconnected");
      }
    };
    
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    BLEService *pService = pServer->createService(serviceUUID);
    pCharacteristic = pService->createCharacteristic(
                        charUUID,
                        BLECharacteristic::PROPERTY_WRITE |
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setCallbacks(new CharacteristicCallbacks());

    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(serviceUUID);
    pAdvertising->start();
    Serial.println("Waiting for client to connect...");

    printer.begin();                 // init UART2 for printer
    // …other init (RTC, database, etc.)
}

void loop() {
    // handle serial monitor restart command
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.equalsIgnoreCase("RESTART")) {
            Serial.println("Restarting device...");
            ESP.restart();
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
