# DMBC Windows 0.1.1 verification

Checked on 25 October 2026 on the development Windows PC.

## Fixes made during the check

- Normalize Windows verbatim resource paths before passing them to PHP. This fixes the installed app silently ignoring php.ini and failing to load SQLite.
- Check required PHP extensions on startup and show a Windows error dialog for startup failures.
- Use JSON authentication responses and validate that a real user was returned. Incorrect credentials no longer produce an HTML parsing error or falsely authenticate a redirect.
- Display the generated desktop account information instead of the old web credentials.
- Update the page's CSRF token after successful sign-in. Refresh an expired token once when signing in again after logout.
- Provide the missing HTML login redirect route.
- Describe backups in terms that also apply to the desktop SQLite installation.

## Automated checks

- Full PHP suite: 27 tests, 119 assertions passed.
- JavaScript suite: 25 checks passed, including desktop USB transport, authentication, receipts, settings drafts and simulated sync workflows.
- Rust launcher regression: Windows verbatim paths are converted to PHP-compatible paths with an absolute extension directory.
- Packaged-backend smoke: real Vue login form method, incorrect/correct credentials, logout/re-login, all primary read endpoints, settings save, customer creation, snapshot creation/restore, ZIP export and restart persistence passed. These writes used a unique temporary profile; they did not modify the association's records.

## Installed application checks

- Signed in with the generated administrator through the installed DMBC form.
- Loaded the dashboard, community members, billing overview, billing settings, backup screen, sync workspace and audit history.
- Downloaded a backup to Windows Downloads and verified the ZIP contains JSON data, a manifest and a SQLite database with the correct file signature.
- Confirmed the backup event is listed in audit history and opens in the event detail modal.
- Invoked native USB enumeration from the sync screen. Windows returned no serial ports, and DMBC displayed the expected actionable message. No device was opened or synchronized.
- Upgraded the installed application to 0.1.1 using the generated installer, then relaunched it. The database, encryption key, generated login details and signed-in session were preserved. Normal window closure stopped its PHP backend.

The earlier `desktop/.test-data/initial-login.txt` file belongs to an isolated prototype database and does not authenticate the installed application. Use the file under `%LOCALAPPDATA%\ph.dmbc.water.development` for installed-app credentials. `desktop/check-account.php <profile-directory>` verifies that file against its database without printing password material or modifying records.

## Checks requiring connected equipment or another PC

Physical ESP32 USB/Wi-Fi transfers, receipt printer output and installation on a separate clean Windows PC remain unverified. Simulated transfers and receipt rendering tests do not replace those equipment checks. The development installer remains unsigned.

The existing MySQL database was not changed. The desktop profile remains separate under `%LOCALAPPDATA%\ph.dmbc.water.development`.
