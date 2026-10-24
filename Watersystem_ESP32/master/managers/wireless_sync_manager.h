#ifndef DMBC_WIRELESS_SYNC_MANAGER_H
#define DMBC_WIRELESS_SYNC_MANAGER_H
#include <WiFi.h>
#include <WebServer.h>
#include <esp_system.h>
#include "sync_transport.h"

bool handleSyncCommands(String command);
void bleShutdownAfterPrint();
extern uint32_t g_deviceId;
static WebServer g_wirelessSyncServer(80);
static bool g_wirelessSyncActive = false;
static bool g_wirelessRoutesReady = false;
static String g_wirelessSsid, g_wirelessToken;
static uint32_t g_wirelessLastActivity = 0;

// Stream exports with a small fixed buffer instead of holding the HTTP body in RAM.
class WirelessSyncOutput : public Print {
  char buffer[1024];
  size_t used = 0;
public:
  using Print::write;
  size_t write(uint8_t value) override {
    buffer[used++] = static_cast<char>(value);
    if (used == sizeof(buffer)) flush();
    return 1;
  }
  void flush() override {
    if (used) { g_wirelessSyncServer.sendContent(buffer, used); used = 0; delay(0); }
  }
};

static bool wirelessCommandAllowed(const String& command) {
  const char* exact[] = { "EXPORT_DEVICE_INFO", "EXPORT_READINGS", "EXPORT_ALL_READINGS", "EXPORT_BILLS", "EXPORT_BILL_TRANSACTIONS", "READINGS_SYNCED", "BILLS_SYNCED", "BILL_TRANSACTIONS_SYNCED", "RELOAD_SD" };
  for (const char* value : exact) if (command == value) return true;
  const char* prefixes[] = { "SET_TIME|", "SET_LAST_SYNC|", "QUERY_READINGS|", "UPSERT_NEW_CUSTOMER_JSON_CHUNK|", "UPSERT_UPDATED_CUSTOMER_JSON_CHUNK|", "UPSERT_CUSTOMERS_JSON_CHUNK|", "UPSERT_CUSTOMERS_JSON|", "UPSERT_BILLS_JSON_CHUNK|", "UPSERT_CUSTOMER_TYPE|", "UPSERT_DEDUCTION|", "UPSERT_BARANGAY|", "UPSERT_SETTINGS|", "NORMALIZE_DATES ", "NORMALIZE_READING_IDS " };
  for (const char* value : prefixes) if (command.startsWith(value)) return true;
  return false;
}

static bool wirelessAuthorize() {
  if (!g_wirelessSyncActive || g_wirelessSyncServer.header("X-DMBC-Token") != g_wirelessToken) {
    g_wirelessSyncServer.send(401, "text/plain", "Pairing code rejected. Check the master display.");
    return false;
  }
  g_wirelessLastActivity = millis();
  return true;
}

static void wirelessInstallRoutes() {
  if (g_wirelessRoutesReady) return;
  const char* headers[] = { "X-DMBC-Token" };
  g_wirelessSyncServer.collectHeaders(headers, 1);
  g_wirelessSyncServer.on("/sync/hello", HTTP_GET, []() {
    if (!wirelessAuthorize()) return;
    g_wirelessSyncServer.sendHeader("Cache-Control", "no-store");
    g_wirelessSyncServer.send(200, "application/json", String("{\"service\":\"DMBC_SYNC\",\"protocol\":1,\"device_id\":") + g_deviceId + "}");
  });
  g_wirelessSyncServer.on("/sync/command", HTTP_POST, []() {
    if (!wirelessAuthorize()) return;
    String command = g_wirelessSyncServer.arg("plain");
    if (!command.length() || command.length() > 4096 || command.indexOf('\n') >= 0 || command.indexOf('\r') >= 0 || !wirelessCommandAllowed(command)) {
      g_wirelessSyncServer.send(422, "text/plain", "Unsupported wireless sync command. Use USB for device restart or SD formatting.");
      return;
    }
    g_wirelessSyncServer.sendHeader("Cache-Control", "no-store");
    g_wirelessSyncServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
    g_wirelessSyncServer.send(200, "text/plain", "");
    WirelessSyncOutput output;
    {
      SyncOutputScope scope(output);
      if (!handleSyncCommands(command)) output.println("ERR|COMMAND_FAILED");
    }
    output.flush();
    g_wirelessSyncServer.sendContent("");
    g_wirelessLastActivity = millis();
  });
  g_wirelessSyncServer.onNotFound([]() { g_wirelessSyncServer.send(404, "text/plain", "DMBC wireless sync endpoint"); });
  g_wirelessRoutesReady = true;
}

static bool wirelessSyncActive() { return g_wirelessSyncActive; }
static void stopWirelessSync() {
  g_wirelessSyncServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  g_wirelessSyncActive = false;
  g_wirelessToken = "";
}
static bool startWirelessSync() {
  if (g_wirelessSyncActive) return true;
  bleShutdownAfterPrint();
  char randomCode[9];
  snprintf(randomCode, sizeof(randomCode), "%08lX", static_cast<unsigned long>(esp_random()));
  g_wirelessToken = randomCode;
  g_wirelessSsid = String("DMBC-SYNC-") + g_deviceId;
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
  if (!WiFi.softAP(g_wirelessSsid.c_str(), nullptr, 1, false, 1)) {
    stopWirelessSync();
    return false;
  }
  wirelessInstallRoutes();
  g_wirelessSyncActive = true;
  g_wirelessLastActivity = millis();
  g_wirelessSyncServer.begin();
  return true;
}
static void pollWirelessSync() {
  g_wirelessSyncServer.handleClient();
  // Never leave an unattended association-data hotspot running indefinitely.
  if (millis() - g_wirelessLastActivity > 600000UL) stopWirelessSync();
}
#endif
