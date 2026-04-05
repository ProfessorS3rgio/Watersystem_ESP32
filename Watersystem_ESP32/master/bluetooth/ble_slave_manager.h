#ifndef BLE_SLAVE_MANAGER_H
#define BLE_SLAVE_MANAGER_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <esp_bt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

NimBLEClient* pBleClient = nullptr;
NimBLERemoteCharacteristic* pBleCharacteristic = nullptr;

void bleSlaveManagerBegin();
void bleRequestReconnect();
bool bleIsConnected();
bool bleIsReady();
bool bleSend(const String &cmd);
bool bleSendBillCommand(const String &cmd, uint32_t ackTimeoutMs = 4500, uint8_t maxAttempts = 3);
bool blePrepareForPrint(uint32_t timeoutMs = 8000);
bool bleIsBusyForPrint();
bool bleCheckPaperPresent(uint32_t timeoutMs = 1500);
bool bleRequestSlaveStatus(uint32_t timeoutMs = 2200);
int bleSlaveBatteryPercent();
String bleSlaveBatteryLevelText();
String bleSlaveChargingStatusText();
String bleSlavePaperStatusText();
void bleShutdownAfterPrint();
bool bleWaitForReady(uint32_t timeoutMs = 3000);
String bleConnectionStatusText();

namespace {
constexpr char BLE_MASTER_DEVICE_NAME[] = "WaterSystem";
constexpr bool BLE_DIRECT_CONNECT_ENABLED = true;
constexpr char BLE_SLAVE_KNOWN_MAC[] = "ac:a7:04:d7:5d:0e";
constexpr uint32_t BLE_SCAN_SECONDS = 5;
constexpr uint32_t BLE_RETRY_DELAY_MS = 1200;
constexpr uint32_t BLE_STATUS_POLL_MS = 1000;
constexpr uint32_t BLE_HEARTBEAT_INTERVAL_MS = 5000;
constexpr uint32_t BLE_HEARTBEAT_TIMEOUT_MS = 15000;
constexpr uint32_t BLE_CONNECT_TIMEOUT_MS = 30000;
constexpr uint32_t BLE_HANDSHAKE_TIMEOUT_MS = 3000;
constexpr uint8_t BLE_MAX_CONSECUTIVE_CONNECT_FAILURES = 3;
constexpr uint32_t BLE_POST_RESET_COOLDOWN_MS = 250;
constexpr size_t BLE_CHUNK_SIZE = 20;
constexpr bool BLE_SCAN_STATUS_LOGGING_ENABLED = true;
constexpr bool BLE_BACKGROUND_RECONNECT_ENABLED = false;

NimBLEUUID g_serviceUuid("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
NimBLEUUID g_characteristicUuid("beb5483e-36e1-4688-b7f5-ea07361b26a8");
bool g_classicBtMemoryReleased = false;
 
struct BleSlaveManagerState {
	SemaphoreHandle_t mutex;
	SemaphoreHandle_t txMutex;
	TaskHandle_t taskHandle;
	volatile bool initialized;
	volatile bool stackInitialized;
	volatile bool reconnectRequested;
	volatile bool connectionAttemptInProgress;
	volatile bool notifyRegistered;
	volatile bool handshakeComplete;
	volatile bool awaitingPong;
	volatile bool intentionalShutdown;
	volatile bool prepareForPrintInProgress;
	volatile uint8_t consecutiveConnectFailures;
	uint32_t lastAttemptMs;
	uint32_t lastRxMs;
	uint32_t lastPingMs;
	volatile uint32_t statusSeq;
	volatile int16_t batterySocTenths;
	volatile int16_t batteryVoltageCenti;
	volatile int8_t batteryAlert;
	volatile int8_t chargingStatus;
	volatile int8_t paperStatus;
	volatile int8_t billAckStatus;
};

BleSlaveManagerState g_bleSlave = {
	nullptr,
	nullptr,
	nullptr,
	false,
	false,
	true,
	false,
	false,
	false,
	false,
	false,
	false,
	0,
	0,
	0,
	0,
	0,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1
};

String g_bleIncomingBuffer;

void bleScanStatusLog(const __FlashStringHelper* message) {
	if (BLE_SCAN_STATUS_LOGGING_ENABLED) {
		Serial.println(message);
	}
}

void bleScanStatusLog(const char* message) {
	if (BLE_SCAN_STATUS_LOGGING_ENABLED) {
		Serial.println(message);
	}
}

bool bleLock(TickType_t timeout = pdMS_TO_TICKS(250)) {
	return g_bleSlave.mutex != nullptr && xSemaphoreTake(g_bleSlave.mutex, timeout) == pdTRUE;
}

bool bleTxLock(TickType_t timeout = pdMS_TO_TICKS(2000)) {
	return g_bleSlave.txMutex != nullptr && xSemaphoreTake(g_bleSlave.txMutex, timeout) == pdTRUE;
}

void bleUnlock() {
	if (g_bleSlave.mutex != nullptr) {
		xSemaphoreGive(g_bleSlave.mutex);
	}
}

void bleTxUnlock() {
	if (g_bleSlave.txMutex != nullptr) {
		xSemaphoreGive(g_bleSlave.txMutex);
	}
}

void bleDisposeFoundDevice() {}

void bleClearConnectionState() {
	if (bleLock()) {
		pBleCharacteristic = nullptr;
		g_bleSlave.notifyRegistered = false;
		g_bleSlave.handshakeComplete = false;
		g_bleSlave.awaitingPong = false;
		g_bleSlave.intentionalShutdown = false;
		g_bleSlave.lastRxMs = 0;
		g_bleSlave.lastPingMs = 0;
		g_bleSlave.statusSeq = 0;
		g_bleSlave.batterySocTenths = -1;
		g_bleSlave.batteryVoltageCenti = -1;
		g_bleSlave.batteryAlert = -1;
		g_bleSlave.chargingStatus = -1;
		g_bleSlave.paperStatus = -1;
		g_bleSlave.billAckStatus = -1;
		bleUnlock();
	}
}

bool bleParseIntField(const String& line, const char* key, int& valueOut) {
	const String token = String(key) + "=";
	const int keyPos = line.indexOf(token);
	if (keyPos < 0) {
		return false;
	}

	const int valueStart = keyPos + token.length();
	int valueEnd = valueStart;
	while (valueEnd < static_cast<int>(line.length())
			&& line[valueEnd] != ' '
			&& line[valueEnd] != 'V'
			&& line[valueEnd] != '%') {
		valueEnd++;
	}

	const String numText = line.substring(valueStart, valueEnd);
	if (numText.length() == 0) {
		return false;
	}

	valueOut = numText.toInt();
	return true;
}

bool bleParseFloatField(const String& line, const char* key, float& valueOut) {
	const String token = String(key) + "=";
	const int keyPos = line.indexOf(token);
	if (keyPos < 0) {
		return false;
	}

	const int valueStart = keyPos + token.length();
	int valueEnd = valueStart;
	while (valueEnd < static_cast<int>(line.length())
			&& line[valueEnd] != ' '
			&& line[valueEnd] != 'V'
			&& line[valueEnd] != '%') {
		valueEnd++;
	}

	const String numText = line.substring(valueStart, valueEnd);
	if (numText.length() == 0) {
		return false;
	}

	valueOut = numText.toFloat();
	return true;
}

void bleParseBatteryTelemetry(const String& line) {
	float voltage = -1.0f;
	float soc = -1.0f;
	int alert = -1;
	int chg = -1;

	const bool hasVoltage = bleParseFloatField(line, "V", voltage);
	const bool hasSoc = bleParseFloatField(line, "SOC", soc);
	const bool hasAlert = bleParseIntField(line, "ALERT", alert);
	const bool hasChg = bleParseIntField(line, "CHG", chg);

	if (bleLock()) {
		if (hasVoltage) {
			g_bleSlave.batteryVoltageCenti = static_cast<int16_t>(lroundf(voltage * 100.0f));
		}
		if (hasSoc) {
			g_bleSlave.batterySocTenths = static_cast<int16_t>(lroundf(soc * 10.0f));
		}
		if (hasAlert) {
			g_bleSlave.batteryAlert = static_cast<int8_t>(alert != 0 ? 1 : 0);
		}
		if (hasChg) {
			g_bleSlave.chargingStatus = static_cast<int8_t>(chg != 0 ? 1 : 0);
		}
		g_bleSlave.statusSeq++;
		bleUnlock();
	}
}

void bleHandleIncomingLine(const String& line) {
	if (line.length() == 0) {
		return;
	}

	String upperLine = line;
	upperLine.toUpperCase();

	const uint32_t nowMs = millis();
	g_bleSlave.lastRxMs = nowMs;

	if (line.equalsIgnoreCase("CONNECTED")) {
		g_bleSlave.handshakeComplete = true;
		g_bleSlave.awaitingPong = false;
		Serial.println(F("[BLE] Slave handshake complete"));
		return;
	}

	if (line.equalsIgnoreCase("PONG")) {
		g_bleSlave.handshakeComplete = true;
		g_bleSlave.awaitingPong = false;
		return;
	}

	if (upperLine.indexOf("PAPER_PRESENT") >= 0) {
		if (bleLock()) {
			g_bleSlave.paperStatus = 1;
			g_bleSlave.statusSeq++;
			bleUnlock();
		}
		return;
	}

	if (upperLine.indexOf("PAPER_OUT") >= 0) {
		if (bleLock()) {
			g_bleSlave.paperStatus = 0;
			g_bleSlave.statusSeq++;
			bleUnlock();
		}
		return;
	}

	if (line.equalsIgnoreCase("ACK_PRINT_BILL")) {
		if (bleLock()) {
			g_bleSlave.billAckStatus = 1;
			g_bleSlave.statusSeq++;
			bleUnlock();
		}
		return;
	}

	if (line.equalsIgnoreCase("NACK_PRINT_BILL")) {
		if (bleLock()) {
			g_bleSlave.billAckStatus = 0;
			g_bleSlave.statusSeq++;
			bleUnlock();
		}
		return;
	}

	if (line.equalsIgnoreCase("PAPER_UNKNOWN")) {
		if (bleLock()) {
			g_bleSlave.paperStatus = -1;
			g_bleSlave.statusSeq++;
			bleUnlock();
		}
		return;
	}

	if (line.equalsIgnoreCase("CHARGING")) {
		if (bleLock()) {
			g_bleSlave.chargingStatus = 1;
			g_bleSlave.statusSeq++;
			bleUnlock();
		}
		return;
	}

	if (line.equalsIgnoreCase("NOT_CHARGING")) {
		if (bleLock()) {
			g_bleSlave.chargingStatus = 0;
			g_bleSlave.statusSeq++;
			bleUnlock();
		}
		return;
	}

	if (line.equalsIgnoreCase("CHARGING_UNKNOWN")) {
		if (bleLock()) {
			g_bleSlave.chargingStatus = -1;
			g_bleSlave.statusSeq++;
			bleUnlock();
		}
		return;
	}

	if (line.startsWith("BATTERY ")) {
		bleParseBatteryTelemetry(line);
		return;
	}

	if (line.startsWith("SOC=")) {
		const float soc = line.substring(4).toFloat();
		if (bleLock()) {
			g_bleSlave.batterySocTenths = static_cast<int16_t>(lroundf(soc * 10.0f));
			g_bleSlave.statusSeq++;
			bleUnlock();
		}
		return;
	}

	if (line.startsWith("VOLT=")) {
		const float voltage = line.substring(5).toFloat();
		if (bleLock()) {
			g_bleSlave.batteryVoltageCenti = static_cast<int16_t>(lroundf(voltage * 100.0f));
			g_bleSlave.statusSeq++;
			bleUnlock();
		}
		return;
	}

	if (line.startsWith("STATUS ")) {
		bleParseBatteryTelemetry(line);
		int paper = -2;
		if (bleParseIntField(line, "PAPER", paper) && bleLock()) {
			g_bleSlave.paperStatus = static_cast<int8_t>(paper);
			g_bleSlave.statusSeq++;
			bleUnlock();
		}
		return;
	}

	Serial.print(F("[BLE] Slave says: "));
	Serial.println(line);
}

void bleNotifyCallback(NimBLERemoteCharacteristic* characteristic, uint8_t* data, size_t length, bool isNotify) {
	(void)characteristic;
	(void)isNotify;

	if (data == nullptr || length == 0) {
		return;
	}

	for (size_t i = 0; i < length; ++i) {
		const char ch = static_cast<char>(data[i]);
		if (ch == '\r') {
			continue;
		}

		if (ch == '\n') {
			String line = g_bleIncomingBuffer;
			g_bleIncomingBuffer = "";
			line.trim();
			bleHandleIncomingLine(line);
			continue;
		}

		g_bleIncomingBuffer += ch;
	}
}

class BleClientCallbacks final : public NimBLEClientCallbacks {
 public:
	void onConnect(NimBLEClient* client) override {
		(void)client;
		g_bleSlave.reconnectRequested = false;
		Serial.println(F("[BLE] Connected to slave"));
	}

	void onDisconnect(NimBLEClient* client, int reason) override {
		(void)client;
		(void)reason;
		const bool intentional = g_bleSlave.intentionalShutdown;
		bleClearConnectionState();
		g_bleSlave.reconnectRequested = intentional ? false : true;
		if (intentional) {
			Serial.println(F("[BLE] Slave link closed after print"));
		} else {
			Serial.println(F("[BLE] Slave disconnected; reconnect scheduled"));
		}
	}
};
BleClientCallbacks g_bleClientCallbacks;

bool bleScanForSlave();
bool bleConnectToSlave();
bool bleEnsureInitialized();
void bleResetClientStack(const __FlashStringHelper* reason);

bool bleAttemptConnectionNow() {
	if (g_bleSlave.connectionAttemptInProgress) {
		return false;
	}

	g_bleSlave.connectionAttemptInProgress = true;
	g_bleSlave.lastAttemptMs = millis();

	bool connected = false;
	if (bleEnsureInitialized()) {
		if (BLE_DIRECT_CONNECT_ENABLED) {
			connected = bleConnectToSlave();
		}

		if (!connected && bleScanForSlave()) {
			connected = bleConnectToSlave();
		}
	}

	g_bleSlave.connectionAttemptInProgress = false;
	return connected;
}

bool bleScanForSlave() {
	bleScanStatusLog(F("[BLE] Scan fallback disabled in NimBLE direct-connect mode"));
	return false;
}

bool bleConnectToSlave() {
	if (pBleClient == nullptr) {
		pBleClient = NimBLEDevice::createClient();
		if (pBleClient == nullptr) {
			Serial.println(F("[BLE] Failed to create client"));
			bleDisposeFoundDevice();
			return false;
		}
		pBleClient->setClientCallbacks(&g_bleClientCallbacks);
	}

	if (pBleClient->isConnected()) {
		bleDisposeFoundDevice();
		return true;
	}

	if (!BLE_DIRECT_CONNECT_ENABLED) {
		Serial.println(F("[BLE] Direct connect disabled; no active scan fallback"));
		return false;
	}

	Serial.print(F("[BLE] Connecting directly to known slave: "));
	Serial.println(BLE_SLAVE_KNOWN_MAC);
	Serial.print(F("[BLE] Heap before connect: "));
	Serial.println(esp_get_free_heap_size());
	NimBLEAddress knownAddress(std::string(BLE_SLAVE_KNOWN_MAC), BLE_ADDR_PUBLIC);
	const bool connected = pBleClient->connect(knownAddress);
	Serial.print(F("[BLE] Heap after connect attempt: "));
	Serial.println(esp_get_free_heap_size());

	if (!connected) {
		Serial.println(F("[BLE] Connect failed"));
		g_bleSlave.consecutiveConnectFailures++;
		if (g_bleSlave.consecutiveConnectFailures >= BLE_MAX_CONSECUTIVE_CONNECT_FAILURES) {
			bleResetClientStack(F("[BLE] Too many connect failures; resetting client stack"));
		}
		bleDisposeFoundDevice();
		return false;
	}
	g_bleSlave.consecutiveConnectFailures = 0;
	Serial.println(F("[BLE] Link established; discovering remote service"));

	NimBLERemoteService* remoteService = pBleClient->getService(g_serviceUuid);
	if (remoteService == nullptr) {
		Serial.println(F("[BLE] Remote service not found"));
		pBleClient->disconnect();
		bleDisposeFoundDevice();
		return false;
	}

	NimBLERemoteCharacteristic* remoteCharacteristic = remoteService->getCharacteristic(g_characteristicUuid);
	if (remoteCharacteristic == nullptr) {
		Serial.println(F("[BLE] Remote characteristic not found"));
		pBleClient->disconnect();
		bleDisposeFoundDevice();
		return false;
	}
	Serial.println(F("[BLE] Remote characteristic ready"));

	if (bleLock()) {
		pBleCharacteristic = remoteCharacteristic;
		g_bleSlave.notifyRegistered = false;
		g_bleSlave.handshakeComplete = false;
		g_bleSlave.awaitingPong = false;
		g_bleSlave.lastRxMs = millis();
		g_bleSlave.lastPingMs = 0;
		bleUnlock();
	}

	if (remoteCharacteristic->canNotify()) {
		if (remoteCharacteristic->subscribe(true, bleNotifyCallback)) {
			g_bleSlave.notifyRegistered = true;
			Serial.println(F("[BLE] Notifications enabled"));
		} else {
			Serial.println(F("[BLE] Notification subscribe failed"));
		}
	}

	g_bleSlave.reconnectRequested = false;
	Serial.println(F("[BLE] Transport connected; waiting for slave handshake"));
	bleSend(String(F("HELLO|")) + BLE_MASTER_DEVICE_NAME);
	if (bleWaitForReady(BLE_HANDSHAKE_TIMEOUT_MS)) {
		Serial.println(F("[BLE] Slave link ready for printer forwarding"));
		g_bleSlave.consecutiveConnectFailures = 0;
	} else {
		Serial.println(F("[BLE] Handshake timeout; forcing reconnect"));
		g_bleSlave.consecutiveConnectFailures++;
		if (g_bleSlave.consecutiveConnectFailures >= BLE_MAX_CONSECUTIVE_CONNECT_FAILURES) {
			bleResetClientStack(F("[BLE] Repeated handshake timeouts; resetting client stack"));
		}
		bleRequestReconnect();
		bleDisposeFoundDevice();
		return false;
	}
	bleDisposeFoundDevice();
	return true;
}

bool bleEnsureInitialized() {
	if (g_bleSlave.stackInitialized) {
		return true;
	}

	if (!g_classicBtMemoryReleased) {
		esp_err_t releaseResult = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
		if (releaseResult == ESP_OK) {
			Serial.println(F("[BLE] Released Classic BT memory"));
			g_classicBtMemoryReleased = true;
		} else {
			Serial.print(F("[BLE] Classic BT memory release skipped, code="));
			Serial.println(static_cast<int>(releaseResult));
		}
	}

	NimBLEDevice::init(BLE_MASTER_DEVICE_NAME);
	NimBLEDevice::setMTU(185);
	NimBLEDevice::setPower(ESP_PWR_LVL_P9);
	g_bleSlave.stackInitialized = true;
	Serial.println(F("[BLE] NimBLE client stack initialized"));
	return true;
}

void bleResetClientStack(const __FlashStringHelper* reason) {
	if (reason != nullptr) {
		Serial.println(reason);
	}

	if (pBleClient != nullptr && pBleClient->isConnected()) {
		g_bleSlave.intentionalShutdown = true;
		pBleClient->disconnect();
		delay(120);
	}

	bleDisposeFoundDevice();
	bleClearConnectionState();
	pBleCharacteristic = nullptr;

	// Avoid hard NimBLE teardown on failure recovery; repeated deinit/delete after
	// connection timeout can trigger heap free asserts on some ESP32/NimBLE builds.
	// Keep the stack initialized and reuse client objects on the next attempt.
	if (!g_bleSlave.stackInitialized) {
		g_bleSlave.stackInitialized = true;
	}

	g_bleSlave.intentionalShutdown = false;
	g_bleSlave.connectionAttemptInProgress = false;
	g_bleSlave.lastAttemptMs = millis();
	g_bleSlave.consecutiveConnectFailures = 0;
	g_bleIncomingBuffer = "";
	g_bleSlave.reconnectRequested = true;
	delay(BLE_POST_RESET_COOLDOWN_MS);
}

bool bleProcessHeartbeat() {
	const uint32_t nowMs = millis();

	if (g_bleSlave.lastRxMs != 0 && (nowMs - g_bleSlave.lastRxMs) > BLE_HEARTBEAT_TIMEOUT_MS) {
		Serial.println(F("[BLE] Heartbeat timeout; reconnecting"));
		bleRequestReconnect();
		return false;
	}

	if ((nowMs - g_bleSlave.lastPingMs) < BLE_HEARTBEAT_INTERVAL_MS) {
		return true;
	}

	g_bleSlave.lastPingMs = nowMs;
	g_bleSlave.awaitingPong = true;

	if (!bleSend(F("PING"))) {
		Serial.println(F("[BLE] Ping failed; reconnecting"));
		bleRequestReconnect();
		return false;
	}

	return true;
}

void bleSlaveTask(void* parameter) {
	(void)parameter;

	for (;;) {
		if (!g_bleSlave.initialized) {
			vTaskDelay(pdMS_TO_TICKS(BLE_STATUS_POLL_MS));
			continue;
		}

		if (bleIsConnected()) {
			bleProcessHeartbeat();
			vTaskDelay(pdMS_TO_TICKS(BLE_STATUS_POLL_MS));
			continue;
		}

		const uint32_t nowMs = millis();
		if (!g_bleSlave.reconnectRequested && (nowMs - g_bleSlave.lastAttemptMs) < BLE_RETRY_DELAY_MS) {
			vTaskDelay(pdMS_TO_TICKS(BLE_STATUS_POLL_MS));
			continue;
		}

		if (g_bleSlave.connectionAttemptInProgress) {
			vTaskDelay(pdMS_TO_TICKS(BLE_STATUS_POLL_MS));
			continue;
		}

		bleAttemptConnectionNow();
		vTaskDelay(pdMS_TO_TICKS(BLE_RETRY_DELAY_MS));
	}
}

}  // namespace

void bleSlaveManagerBegin() {
	if (g_bleSlave.mutex == nullptr) {
		g_bleSlave.mutex = xSemaphoreCreateMutex();
	}
	if (g_bleSlave.txMutex == nullptr) {
		g_bleSlave.txMutex = xSemaphoreCreateMutex();
	}

	g_bleSlave.initialized = true;
	g_bleSlave.reconnectRequested = true;
	g_bleSlave.lastAttemptMs = 0;

	if (BLE_BACKGROUND_RECONNECT_ENABLED && g_bleSlave.taskHandle == nullptr) {
		xTaskCreatePinnedToCore(
			bleSlaveTask,
			"BleSlaveTask",
			6144,
			nullptr,
			1,
			&g_bleSlave.taskHandle,
			1
		);
		Serial.println(F("[BLE] Background reconnect task started"));
	} else if (!BLE_BACKGROUND_RECONNECT_ENABLED) {
		Serial.println(F("[BLE] Direct print-connect mode enabled"));
	}
}

void bleRequestReconnect() {
	g_bleSlave.reconnectRequested = true;
	bleClearConnectionState();

	if (pBleClient != nullptr && pBleClient->isConnected()) {
		pBleClient->disconnect();
	}
}

bool bleIsConnected() {
	if (!bleLock()) {
		return false;
	}

	const bool connected = pBleClient != nullptr
			&& pBleCharacteristic != nullptr
			&& pBleClient->isConnected();

	bleUnlock();
	return connected;
}

bool bleIsReady() {
	if (!bleLock()) {
		return false;
	}

	const bool ready = pBleClient != nullptr
			&& pBleCharacteristic != nullptr
			&& pBleClient->isConnected()
			&& g_bleSlave.handshakeComplete;

	bleUnlock();
	return ready;
}

bool bleSend(const String &cmd) {
	NimBLEClient* localClient = nullptr;
	NimBLERemoteCharacteristic* localCharacteristic = nullptr;
	if (!bleLock()) {
		return false;
	}

	localClient = pBleClient;
	localCharacteristic = pBleCharacteristic;
	const bool connected = localClient != nullptr
			&& localCharacteristic != nullptr
			&& localClient->isConnected();
	bleUnlock();

	if (!connected) {
		return false;
	}

	if (!bleTxLock()) {
		return false;
	}

	String payload = cmd;
	if (!payload.endsWith("\n")) {
		payload += "\n";
	}

	const char* data = payload.c_str();
	const size_t len = payload.length();
	bool success = true;
	size_t offset = 0;
	while (offset < len) {
		if (localClient == nullptr || localCharacteristic == nullptr || !localClient->isConnected()) {
			success = false;
			break;
		}

		const size_t chunk = min(BLE_CHUNK_SIZE, len - offset);
		const bool writeOk = localCharacteristic->writeValue(
			reinterpret_cast<const uint8_t*>(data + offset),
			chunk,
			true
		);
		if (!writeOk) {
			success = false;
			break;
		}
		offset += chunk;
		// Give BLE server callback time to append each chunk into its line buffer.
		delay(30);
	}

	bleTxUnlock();

	if (!success) {
		bleRequestReconnect();
	}

	return success;
}

bool bleSendBillCommand(const String &cmd, uint32_t ackTimeoutMs, uint8_t maxAttempts) {
	if (maxAttempts == 0) {
		maxAttempts = 1;
	}

	for (uint8_t attempt = 1; attempt <= maxAttempts; ++attempt) {
		if (bleLock()) {
			g_bleSlave.billAckStatus = -1;
			bleUnlock();
		}

		if (!bleSend(cmd)) {
			Serial.print(F("[BLE] Bill send failed (attempt "));
			Serial.print(attempt);
			Serial.println(F(")"));
			delay(120);
			continue;
		}

		const uint32_t startMs = millis();
		while ((millis() - startMs) < ackTimeoutMs) {
			int8_t ack = -1;
			if (bleLock()) {
				ack = g_bleSlave.billAckStatus;
				bleUnlock();
			}

			if (ack == 1) {
				return true;
			}

			if (ack == 0) {
				Serial.print(F("[BLE] Bill NACK from slave (attempt "));
				Serial.print(attempt);
				Serial.println(F(")"));
				break;
			}

			delay(20);
		}

		Serial.print(F("[BLE] Bill ACK timeout (attempt "));
		Serial.print(attempt);
		Serial.println(F(")"));
		delay(120);
	}

	return false;
}

bool blePrepareForPrint(uint32_t timeoutMs) {
	bleSlaveManagerBegin();
	g_bleSlave.prepareForPrintInProgress = true;
	g_bleSlave.reconnectRequested = true;
	const uint32_t defaultTimeoutMs = BLE_CONNECT_TIMEOUT_MS + BLE_HANDSHAKE_TIMEOUT_MS + 2000;
	const uint32_t effectiveTimeoutMs = timeoutMs == 0 ? defaultTimeoutMs : max(timeoutMs, static_cast<uint32_t>(1500));

	if (bleIsReady()) {
		g_bleSlave.prepareForPrintInProgress = false;
		return true;
	}

	const uint32_t startMs = millis();
	while ((millis() - startMs) < effectiveTimeoutMs) {
		if (bleIsReady()) {
			g_bleSlave.prepareForPrintInProgress = false;
			return true;
		}

		const bool retryDelayElapsed = (millis() - g_bleSlave.lastAttemptMs) >= BLE_RETRY_DELAY_MS;
		if (!g_bleSlave.connectionAttemptInProgress && retryDelayElapsed) {
			const bool connectedNow = bleAttemptConnectionNow();
			if (connectedNow && bleIsReady()) {
				return true;
			}
		}

		delay(25);
	}

	if (bleIsReady()) {
		g_bleSlave.prepareForPrintInProgress = false;
		return true;
	}

	if (bleIsConnected()) {
		Serial.println(F("[BLE] Prepare timeout reached but transport is connected; skipping forced reset"));
		g_bleSlave.prepareForPrintInProgress = false;
		return false;
	}

	Serial.println(F("[BLE] Prepare timeout (no link); resetting BLE stack for next attempt"));
	bleResetClientStack(F("[BLE] BLE stack reset after prepare timeout"));
	g_bleSlave.prepareForPrintInProgress = false;

	return false;
}

bool bleIsBusyForPrint() {
	return g_bleSlave.prepareForPrintInProgress || g_bleSlave.connectionAttemptInProgress;
}

void bleShutdownAfterPrint() {
	if (!g_bleSlave.stackInitialized) {
		return;
	}

	Serial.println(F("[BLE] Releasing BLE stack after print"));
	g_bleSlave.reconnectRequested = false;
	g_bleSlave.connectionAttemptInProgress = false;
	g_bleSlave.intentionalShutdown = true;

	delay(150);

	if (pBleClient != nullptr && pBleClient->isConnected()) {
		pBleClient->disconnect();
		delay(150);
	}

	bleDisposeFoundDevice();
	bleClearConnectionState();
	pBleCharacteristic = nullptr;
	g_bleSlave.stackInitialized = true;
	g_bleSlave.notifyRegistered = false;
	g_bleSlave.handshakeComplete = false;
	g_bleSlave.awaitingPong = false;
	g_bleSlave.intentionalShutdown = false;
	g_bleSlave.lastAttemptMs = millis();
	g_bleSlave.consecutiveConnectFailures = 0;
	g_bleSlave.lastRxMs = 0;
	g_bleSlave.lastPingMs = 0;
	g_bleIncomingBuffer = "";
	Serial.println(F("[BLE] BLE link closed; NimBLE stack kept initialized"));
}

bool bleWaitForReady(uint32_t timeoutMs) {
	const uint32_t startMs = millis();
	while ((millis() - startMs) < timeoutMs) {
		if (bleIsReady()) {
			return true;
		}

		if (!bleIsConnected()) {
			return false;
		}

		delay(25);
	}

	return bleIsReady();
}

bool bleCheckPaperPresent(uint32_t timeoutMs) {
	if (!bleIsReady()) {
		return false;
	}

	for (int attempt = 0; attempt < 2; ++attempt) {
		if (bleLock()) {
			g_bleSlave.paperStatus = -1;
			bleUnlock();
		}

		if (!bleSend(F("PAPER_STATUS"))) {
			Serial.println(F("[BLE] Failed to request paper status"));
			return false;
		}

		const uint32_t startMs = millis();
		while ((millis() - startMs) < timeoutMs) {
			int8_t status = -1;
			if (bleLock()) {
				status = g_bleSlave.paperStatus;
				bleUnlock();
			}

			if (status == 1) {
				Serial.println(F("[BLE] Paper check: PRESENT"));
				return true;
			}

			if (status == 0) {
				if (attempt == 0) {
					// Some printers briefly report OUT while waking; confirm once more.
					Serial.println(F("[BLE] Paper check: OUT (first read), confirming..."));
					break;
				}
				Serial.println(F("[BLE] Paper check: OUT (confirmed)"));
				return false;
			}

			delay(20);
		}

		if (attempt == 0) {
			Serial.println(F("[BLE] Paper check retrying..."));
			delay(220);
		}
	}

	Serial.println(F("[BLE] Paper check timeout/unknown"));
	return false;
}

bool bleRequestSlaveStatus(uint32_t timeoutMs) {
	if (!bleIsReady()) {
		return false;
	}

	uint32_t startSeq = 0;
	int16_t startSoc = -1;
	int8_t startChg = -1;
	int8_t startPaper = -1;
	if (bleLock()) {
		startSeq = g_bleSlave.statusSeq;
		startSoc = g_bleSlave.batterySocTenths;
		startChg = g_bleSlave.chargingStatus;
		startPaper = g_bleSlave.paperStatus;
		bleUnlock();
	}

	if (!bleSend(F("STATUS"))) {
		Serial.println(F("[BLE] Failed to request STATUS"));
		return false;
	}

	const uint32_t startMs = millis();
	while ((millis() - startMs) < timeoutMs) {
		uint32_t seqNow = startSeq;
		int16_t socNow = -1;
		int8_t chgNow = -1;
		int8_t paperNow = -1;
		if (bleLock()) {
			seqNow = g_bleSlave.statusSeq;
			socNow = g_bleSlave.batterySocTenths;
			chgNow = g_bleSlave.chargingStatus;
			paperNow = g_bleSlave.paperStatus;
			bleUnlock();
		}

		const bool changed = seqNow != startSeq;
		const bool gotSoc = socNow >= 0 && socNow != startSoc;
		const bool gotChg = chgNow != -1 && chgNow != startChg;
		const bool gotPaper = paperNow != -1 && paperNow != startPaper;

		if (changed && (gotSoc || gotChg || gotPaper)) {
			return true;
		}

		delay(20);
	}

	return false;
}

int bleSlaveBatteryPercent() {
	int16_t socTenths = -1;
	if (bleLock()) {
		socTenths = g_bleSlave.batterySocTenths;
		bleUnlock();
	}

	if (socTenths < 0) {
		return -1;
	}

	int pct = static_cast<int>((socTenths + 5) / 10);
	if (pct < 0) pct = 0;
	if (pct > 100) pct = 100;
	return pct;
}

String bleSlaveBatteryLevelText() {
	const int pct = bleSlaveBatteryPercent();
	if (pct < 0) {
		return String(F("unknown"));
	}

	if (pct <= 25) {
		return String(F("LOW"));
	}

	if (pct >= 90) {
		return String(F("FULL"));
	}

	return String(F("GOOD"));
}

String bleSlaveChargingStatusText() {
	int8_t chg = -1;
	if (bleLock()) {
		chg = g_bleSlave.chargingStatus;
		bleUnlock();
	}

	if (chg == 1) {
		return String(F("charging"));
	}

	if (chg == 0) {
		return String(F("not charging"));
	}

	return String(F("unknown"));
}

String bleSlavePaperStatusText() {
	int8_t paper = -1;
	if (bleLock()) {
		paper = g_bleSlave.paperStatus;
		bleUnlock();
	}

	if (paper == 1) {
		return String(F("present"));
	}

	if (paper == 0) {
		return String(F("out"));
	}

	return String(F("unknown"));
}

String bleConnectionStatusText() {
	if (bleIsReady()) {
		return String(F("ready"));
	}

	return bleIsConnected() ? String(F("connected")) : String(F("disconnected"));
}

#endif  // BLE_SLAVE_MANAGER_H
