# DMBC wireless sync

Wireless sync uses the master's own open Wi-Fi access point. No router, internet connection, hotspot password, or laptop serial driver is needed. The Laravel application and database must run on the laptop that joins the hotspot. A cloud-hosted Laravel server cannot reach this private device address.

## Using it

1. Compile and upload the updated master firmware using the ESP32 Huge APP partition scheme. Install the web changes on the laptop as well.
2. On the master, open Menu → System Tools → **6. Wireless Sync**.
3. In Windows Wi-Fi settings, join **DMBC-SYNC-<device ID>**. It is an open network; Windows may report “No internet,” which is expected.
4. Keep the locally running DMBC web application open at its usual localhost address. Open **Device sync → Sync workspace → Wi-Fi hotspot**.
5. Enter the eight-character pairing code displayed on the master, choose **Pair over Wi-Fi**, then **Start sync**.
6. Wait for completion. Press **C** or **#** on the master to exit and stop the hotspot. It also turns off after ten minutes without a device API request.

The pairing code is for the DMBC application; it is not a Wi-Fi network password. It changes each time Wireless Sync mode starts. This is an open local network using HTTP, so the pairing code does not provide encrypted traffic.

## Behavior and architecture

- Normal keypad billing, printing, and USB command processing pause while Wireless Sync mode is active. The BLE printer connection is released before enabling Wi-Fi.
- USB remains supported. Disconnect one transport before selecting the other.
- The browser calls authenticated, CSRF-protected Laravel routes under `/device-link/wifi/`. Laravel relays allowlisted commands to the fixed address `http://192.168.4.1`. The browser does not connect directly to the device, and there is no CORS setup.
- The device exposes `GET /sync/hello` and `POST /sync/command`, both requiring `X-DMBC-Token`. It accepts one hotspot station at a time.
- Existing command handlers write protocol responses through `syncOutput()`. USB is the default; wireless requests temporarily select a buffered HTTP response writer. Exports stream through a 1 KB buffer and use smaller JSON chunks to limit Wi-Fi heap use.
- Complete chunk counts and JSON are checked before importing. Bills and payment transactions are marked synced on the device only after successful website import. Failed or interrupted exports remain available for retry.
- Device restart, SD formatting, and arbitrary debug commands require USB. Wireless supports the normal sync workflow, inspection, and date normalization.

## Validation

Automated web checks use mocked devices and HTTP responses. Before field use, test one master with actual SD records: pair, sync, verify website and device counts, repeat sync without duplicates, interrupt a transfer and retry, inspect an account, exit wireless mode, and confirm USB sync and BLE printing still work. No automated test flashes or erases a device.
