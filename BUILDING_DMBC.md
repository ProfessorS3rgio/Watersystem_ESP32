# Rebuilding DMBC on Windows

There are two separate builds: the Windows application and the ESP32 master firmware. Editing this repository does not update an installed application or a device automatically.

## Windows application

Open PowerShell and run:

```powershell
cd "C:\Users\ProfessorS3rgio\Documents\Arduino\Watersystem\water_system_web"
npm run desktop:build
```

This prepares the Laravel backend, builds the Vue interface, packages PHP, and creates the Windows installer. The development computer needs Node/npm, Composer/PHP with the required extensions, Rust, and the Visual Studio C++ build tools. They have already been used on this computer. After installing tools, open a new terminal so PATH updates take effect. On a fresh checkout, install dependencies with `composer install` and `npm install` first.

Find the installer in:

```text
water_system_web\src-tauri\target\release\bundle\nsis\
```

Its filename includes the version in `src-tauri/tauri.conf.json`, currently `DMBC Water_0.2.1_x64-setup.exe`. Close DMBC, then run the newly generated installer to update the installed application. Its existing SQLite data lives separately under `%LOCALAPPDATA%\ph.dmbc.water.development`. Keep the application identifier unchanged when upgrading this development profile. Make a backup from DMBC before installing an update.

`npm run build` only builds the web interface; it does not create an installer. For desktop development, use `npm run desktop:dev`. This setup stages the application at startup, so stop it with Ctrl+C and run it again after changes. It uses the development profile, so changes made while testing can affect that profile's data.

## ESP32 master firmware

The reading export heap fix requires uploading the updated **master firmware**. Rebuilding the Windows application alone cannot replace the code running on the device.

In Arduino IDE:

1. Open `Watersystem_ESP32\master\master.ino`.
2. Select **ESP32 Dev Module** and **Huge APP (3MB No OTA/1MB SPIFFS)** under Partition Scheme, matching the current CLI build.
3. Select the master device's actual COM port.
4. Click Verify to compile. Disconnect DMBC's USB connection and close any Serial Monitor before clicking Upload.

Alternatively, compile with PowerShell:

```powershell
cd "C:\Users\ProfessorS3rgio\Documents\Arduino\Watersystem\Watersystem_ESP32\master"
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app --jobs 2 .
arduino-cli board list
```

After identifying the correct port, replace `COMx` below with that port and upload:

```powershell
arduino-cli upload --fqbn esp32:esp32:esp32:PartitionScheme=huge_app --port COMx .
```

Normal firmware uploads do not require formatting the SD card or erasing the database. Preserve the existing partition scheme and leave full flash erase disabled.

## Check the reading export fix

1. Reconnect the master to DMBC after uploading.
2. Run a normal sync, then inspect readings twice without restarting the device.
3. Confirm inspection finishes with `END_READINGS_JSON` and the full expected reading count, without `ERR|JSON_CAPACITY`.
4. Query an account with several saved readings and confirm its history matches.
5. Repeat through Wi-Fi if used. Check nonempty bill/payment exports when available.

Inspection remains read-only. Outgoing exports stream SQLite rows directly, use 20-row bulk frames, finalize their statements, and release unused SQLite cache memory at the end. They no longer build a whole JSON chunk in an ArduinoJson document. Account queries use one row per frame to keep their diagnostic lines outside the JSON. The application now waits for reading export progress instead of imposing a fixed one-minute total duration.

Compilation and automated checks cannot prove the device's actual heap behavior. Repeat the sync/inspection sequence on the physical master to confirm it.
