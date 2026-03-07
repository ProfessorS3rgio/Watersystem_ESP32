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
bool blePrepareForPrint(uint32_t timeoutMs = 8000);
void bleShutdownAfterPrint();
bool bleWaitForReady(uint32_t timeoutMs = 3000);
String bleConnectionStatusText();

namespace {
constexpr char BLE_MASTER_DEVICE_NAME[] = "WaterSystem";
constexpr bool BLE_DIRECT_CONNECT_ENABLED = true;
constexpr char BLE_SLAVE_KNOWN_MAC[] = "d4:e9:f4:8d:5f:82";
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
	TaskHandle_t taskHandle;
	volatile bool initialized;
	volatile bool stackInitialized;
	volatile bool reconnectRequested;
	volatile bool connectionAttemptInProgress;
	volatile bool notifyRegistered;
	volatile bool handshakeComplete;
	volatile bool awaitingPong;
	volatile bool intentionalShutdown;
	volatile uint8_t consecutiveConnectFailures;
	uint32_t lastAttemptMs;
	uint32_t lastRxMs;
	uint32_t lastPingMs;
};

BleSlaveManagerState g_bleSlave = {
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
	0,
	0,
	0,
	0
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

void bleUnlock() {
	if (g_bleSlave.mutex != nullptr) {
		xSemaphoreGive(g_bleSlave.mutex);
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
		bleUnlock();
	}
}

void bleHandleIncomingLine(const String& line) {
	if (line.length() == 0) {
		return;
	}

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
	NimBLEAddress knownAddress(std::string(BLE_SLAVE_KNOWN_MAC), BLE_ADDR_PUBLIC);
	const bool connected = pBleClient->connect(knownAddress);

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
	if (pBleClient != nullptr) {
		NimBLEDevice::deleteClient(pBleClient);
		pBleClient = nullptr;
	}

	if (g_bleSlave.stackInitialized) {
		NimBLEDevice::deinit(false);
		g_bleSlave.stackInitialized = false;
	}

	g_bleSlave.intentionalShutdown = false;
	g_bleSlave.connectionAttemptInProgress = false;
	g_bleSlave.lastAttemptMs = millis();
	g_bleSlave.consecutiveConnectFailures = 0;
	g_bleIncomingBuffer = "";
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
	if (!bleLock()) {
		return false;
	}

	const bool connected = pBleClient != nullptr
			&& pBleCharacteristic != nullptr
			&& pBleClient->isConnected();

	if (!connected) {
		bleUnlock();
		return false;
	}

	String payload = cmd;
	if (!payload.endsWith("\n")) {
		payload += "\n";
	}

	const char* data = payload.c_str();
	const size_t len = payload.length();
	size_t offset = 0;
	bool success = true;

	while (offset < len) {
		if (pBleClient == nullptr || pBleCharacteristic == nullptr || !pBleClient->isConnected()) {
			success = false;
			break;
		}

		const size_t chunk = min(BLE_CHUNK_SIZE, len - offset);
		const bool writeOk = pBleCharacteristic->writeValue(
			reinterpret_cast<const uint8_t*>(data + offset),
			chunk,
			false
		);
		if (!writeOk) {
			success = false;
			break;
		}
		offset += chunk;
		delay(10);
	}

	bleUnlock();

	if (!success) {
		bleRequestReconnect();
	}

	return success;
}

bool blePrepareForPrint(uint32_t timeoutMs) {
	bleSlaveManagerBegin();
	g_bleSlave.reconnectRequested = true;
	const uint32_t defaultTimeoutMs = BLE_CONNECT_TIMEOUT_MS + BLE_HANDSHAKE_TIMEOUT_MS + 2000;
	const uint32_t effectiveTimeoutMs = timeoutMs == 0 ? defaultTimeoutMs : max(timeoutMs, static_cast<uint32_t>(1500));

	if (bleIsReady()) {
		return true;
	}

	const uint32_t startMs = millis();
	while ((millis() - startMs) < effectiveTimeoutMs) {
		if (bleIsReady()) {
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
		return true;
	}

	if (bleIsConnected()) {
		Serial.println(F("[BLE] Prepare timeout reached but transport is connected; skipping forced reset"));
		return false;
	}

	Serial.println(F("[BLE] Prepare timeout (no link); resetting BLE stack for next attempt"));
	bleResetClientStack(F("[BLE] BLE stack reset after prepare timeout"));

	return false;
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

String bleConnectionStatusText() {
	if (bleIsReady()) {
		return String(F("ready"));
	}

	return bleIsConnected() ? String(F("connected")) : String(F("disconnected"));
}

#endif  // BLE_SLAVE_MANAGER_H
