#ifndef BLE_SLAVE_MANAGER_H
#define BLE_SLAVE_MANAGER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEClient.h>
#include <BLEScan.h>
#include <BLEUtils.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

BLEClient* pBleClient = nullptr;
BLERemoteCharacteristic* pBleCharacteristic = nullptr;

void bleSlaveManagerBegin();
void bleRequestReconnect();
bool bleIsConnected();
bool bleIsReady();
bool bleSend(const String &cmd);
bool bleWaitForReady(uint32_t timeoutMs = 3000);
String bleConnectionStatusText();

namespace {
constexpr char BLE_MASTER_DEVICE_NAME[] = "WaterSystem";
constexpr uint32_t BLE_SCAN_SECONDS = 5;
constexpr uint32_t BLE_RETRY_DELAY_MS = 5000;
constexpr uint32_t BLE_STATUS_POLL_MS = 1000;
constexpr uint32_t BLE_HEARTBEAT_INTERVAL_MS = 5000;
constexpr uint32_t BLE_HEARTBEAT_TIMEOUT_MS = 15000;
constexpr size_t BLE_CHUNK_SIZE = 20;

BLEUUID g_serviceUuid("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
BLEUUID g_characteristicUuid("beb5483e-36e1-4688-b7f5-ea07361b26a8");
BLEAdvertisedDevice* g_foundDevice = nullptr;

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
	0,
	0,
	0
};

String g_bleIncomingBuffer;

bool bleLock(TickType_t timeout = pdMS_TO_TICKS(250)) {
	return g_bleSlave.mutex != nullptr && xSemaphoreTake(g_bleSlave.mutex, timeout) == pdTRUE;
}

void bleUnlock() {
	if (g_bleSlave.mutex != nullptr) {
		xSemaphoreGive(g_bleSlave.mutex);
	}
}

void bleDisposeFoundDevice() {
	if (g_foundDevice != nullptr) {
		delete g_foundDevice;
		g_foundDevice = nullptr;
	}
}

void bleClearConnectionState() {
	if (bleLock()) {
		pBleCharacteristic = nullptr;
		g_bleSlave.notifyRegistered = false;
		g_bleSlave.handshakeComplete = false;
		g_bleSlave.awaitingPong = false;
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

void bleNotifyCallback(BLERemoteCharacteristic* characteristic, uint8_t* data, size_t length, bool isNotify) {
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

class BleAdvertisedCallbacks final : public BLEAdvertisedDeviceCallbacks {
 public:
	void onResult(BLEAdvertisedDevice advertisedDevice) override {
		if (!advertisedDevice.haveServiceUUID()) {
			return;
		}

		if (!advertisedDevice.getServiceUUID().equals(g_serviceUuid)) {
			return;
		}

		bleDisposeFoundDevice();
		g_foundDevice = new BLEAdvertisedDevice(advertisedDevice);
		BLEDevice::getScan()->stop();
	}
};

class BleClientCallbacks final : public BLEClientCallbacks {
 public:
	void onConnect(BLEClient* client) override {
		(void)client;
		g_bleSlave.reconnectRequested = false;
		Serial.println(F("[BLE] Connected to slave"));
	}

	void onDisconnect(BLEClient* client) override {
		(void)client;
		bleClearConnectionState();
		g_bleSlave.reconnectRequested = true;
		Serial.println(F("[BLE] Slave disconnected; reconnect scheduled"));
	}
};

BleAdvertisedCallbacks g_bleAdvertisedCallbacks;
BleClientCallbacks g_bleClientCallbacks;

bool bleScanForSlave() {
	bleDisposeFoundDevice();

	BLEScan* scan = BLEDevice::getScan();
	if (scan == nullptr) {
		Serial.println(F("[BLE] Scan instance unavailable"));
		return false;
	}

	scan->clearResults();
	scan->setAdvertisedDeviceCallbacks(&g_bleAdvertisedCallbacks, false);
	scan->setActiveScan(true);
	scan->setInterval(160);
	scan->setWindow(80);

	Serial.println(F("[BLE] Scanning for slave..."));
	scan->start(BLE_SCAN_SECONDS, false);
	scan->stop();
	scan->clearResults();

	if (g_foundDevice == nullptr) {
		Serial.println(F("[BLE] Slave not found"));
		return false;
	}

	Serial.print(F("[BLE] Found slave: "));
	Serial.println(g_foundDevice->getAddress().toString().c_str());
	return true;
}

bool bleConnectToSlave() {
	if (g_foundDevice == nullptr) {
		return false;
	}

	if (pBleClient == nullptr) {
		pBleClient = BLEDevice::createClient();
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

	Serial.println(F("[BLE] Connecting to slave..."));
	if (!pBleClient->connect(g_foundDevice)) {
		Serial.println(F("[BLE] Connect failed"));
		bleDisposeFoundDevice();
		return false;
	}

	BLERemoteService* remoteService = pBleClient->getService(g_serviceUuid);
	if (remoteService == nullptr) {
		Serial.println(F("[BLE] Remote service not found"));
		pBleClient->disconnect();
		bleDisposeFoundDevice();
		return false;
	}

	BLERemoteCharacteristic* remoteCharacteristic = remoteService->getCharacteristic(g_characteristicUuid);
	if (remoteCharacteristic == nullptr) {
		Serial.println(F("[BLE] Remote characteristic not found"));
		pBleClient->disconnect();
		bleDisposeFoundDevice();
		return false;
	}

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
		remoteCharacteristic->registerForNotify(bleNotifyCallback);
		g_bleSlave.notifyRegistered = true;
		Serial.println(F("[BLE] Notifications enabled"));
	}

	g_bleSlave.reconnectRequested = false;
	Serial.println(F("[BLE] Transport connected; waiting for slave handshake"));
	bleSend(String(F("HELLO|")) + BLE_MASTER_DEVICE_NAME);
	if (bleWaitForReady(3000)) {
		Serial.println(F("[BLE] Slave link ready for printer forwarding"));
	} else {
		Serial.println(F("[BLE] Handshake pending; background heartbeat will continue"));
	}
	bleDisposeFoundDevice();
	return true;
}

bool bleEnsureInitialized() {
	if (g_bleSlave.stackInitialized) {
		return true;
	}

	BLEDevice::init(BLE_MASTER_DEVICE_NAME);
	g_bleSlave.stackInitialized = true;
	Serial.println(F("[BLE] Client stack initialized"));
	return true;
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

		g_bleSlave.connectionAttemptInProgress = true;
		g_bleSlave.lastAttemptMs = nowMs;

		if (bleEnsureInitialized()) {
			if (bleScanForSlave()) {
				bleConnectToSlave();
			}
		}

		g_bleSlave.connectionAttemptInProgress = false;
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

	if (g_bleSlave.taskHandle == nullptr) {
		xTaskCreatePinnedToCore(
			bleSlaveTask,
			"BleSlaveTask",
			6144,
			nullptr,
			1,
			&g_bleSlave.taskHandle,
			1
		);
	}

	Serial.println(F("[BLE] Background reconnect task started"));
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
		pBleCharacteristic->writeValue(reinterpret_cast<uint8_t*>(const_cast<char*>(data + offset)), chunk);
		offset += chunk;
		delay(10);
	}

	bleUnlock();

	if (!success) {
		bleRequestReconnect();
	}

	return success;
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
