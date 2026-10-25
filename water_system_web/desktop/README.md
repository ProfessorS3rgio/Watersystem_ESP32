# DMBC Water for Windows — development build

This branch packages the existing Laravel/Vue application in a Tauri desktop window. The Windows installer includes PHP, Laravel, compiled UI assets and the DMBC logo. End users do not run XAMPP, Composer, Node or a database server.

## Try the installer

Build output:

`src-tauri/target/release/bundle/nsis/DMBC Water_0.1.1_x64-setup.exe`

Install for your Windows user, then open **DMBC Water**. The application prepares an empty SQLite database and reference settings automatically. Existing MySQL records are not imported or changed. No sample customers are seeded.

On first launch, open `%LOCALAPPDATA%\ph.dmbc.water.development\initial-login.txt` to get the generated administrator password. The email is `admin@dmbc.local`. Repeat launches retain this account and its password.

Data lives in `%LOCALAPPDATA%\ph.dmbc.water.development`:

- `dmbc.sqlite`: desktop database. While open, SQLite may also create `-wal` and `-shm` files. Use Settings → Backup to get a consistent database archive.
- `application.key`: persistent Laravel encryption key; preserve it with your data.
- `storage`: sessions, restore-related files and application logs.
- `backend.log` / `startup-error.log`: startup diagnostics.
- `before-upgrade-*.sqlite`: automatic database copies made before pending migrations.

Updates do not replace this profile. This development identifier intentionally separates it from a future production installation.

## Build locally

Requires Node/npm, PHP 8.2+ with the extensions checked by `check-runtime.php`, installed Composer dependencies, Rust MSVC, Visual Studio C++ Build Tools and WebView2. The installer uses Tauri's WebView2 setup support; internet may be required if WebView2 is absent.

From `water_system_web`:

```powershell
npm install
npm run desktop:build
```

For quicker test builds use `npm run desktop:build:dev`. Output is under `src-tauri/target/debug/bundle/nsis`. `npm run desktop:dev` starts the native application after staging a compiled UI. Restart the app after source changes; the first version does not provide frontend hot reload.

The preparation script uses the PHP installation on PATH. Override `DMBC_PHP_SOURCE` with a trusted Windows PHP directory if needed. Only application sources, dependencies and public assets are staged. `.env`, databases, uploaded data and development profiles are excluded. Staged resources and build outputs are Git-ignored; the source configuration and Cargo lockfile should be committed.

Validation:

```powershell
npm run desktop:prepare
npm run desktop:test
php artisan test --filter='DesktopSqliteTest|AuditHistoryTest|SettingsSaveTest|WirelessSyncTest'
```

The smoke check starts the packaged PHP backend against a unique temporary profile without opening a browser. It exercises the actual Vue login form method with incorrect and correct credentials, logout/re-login, settings saves, customer creation, snapshot restore, restart persistence, local access controls, primary read endpoints and archive export. The feature checks cover real SQLite migrations, partial/full payments, relational snapshot restore, audit retention and archive integrity.

## Device connections and remaining acceptance checks

USB uses native serial access with an explicit Windows port chooser. The browser version retains Web Serial. Wi-Fi uses the existing local Laravel relay to the ESP32 hotspot. Hardware synchronization must still be tested on the real master; compilation and simulated transport tests do not verify physical connectivity.

Check installation on another Windows PC, first login, closing/reopening, USB/Wi-Fi sync, receipt printing and downloads. The first installer is unsigned; Windows may show an unknown-publisher prompt. There is no automatic update service or MySQL-to-SQLite import in this build. The bundled PHP development server is limited to `127.0.0.1` and intended for this development trial; a production release needs a reviewed runtime/serving strategy and clean-machine testing.

The native window is restricted to its own local origin. Local backend access requires a per-launch cookie, and serial commands check the active window and exact local origin. A single-instance guard prevents two DMBC windows from starting separate backends for this profile. PHP is stopped on normal application exit.

## Startup diagnostics

The launcher checks the bundled PHP extensions before accessing SQLite and shows a Windows error dialog if startup fails. See `startup-error.log` in the profile for the underlying error; preserve the profile when reinstalling.

Windows resource paths may have a `\\?\` prefix. The launcher normalizes those paths before passing them to PHP and supplies an absolute extension directory. Without normalization, PHP can silently ignore its INI file and report `could not find driver` even when the SQLite DLL is installed. The launcher regression test covers this path format:

```powershell
cargo test --manifest-path src-tauri/Cargo.toml --bin dmbc-desktop
```

References: [Tauri Windows installers](https://v2.tauri.app/distribute/windows-installer/), [Tauri prerequisites](https://v2.tauri.app/start/prerequisites/).
