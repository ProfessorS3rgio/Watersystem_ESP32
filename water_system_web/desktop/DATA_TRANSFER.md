# MySQL to installed desktop SQLite

Completed on 25 October 2026. The existing MySQL database was read from a consistent, read-only snapshot and was not modified.

The installed desktop now contains 276 customers, 544 readings, 470 bills, 335 payment/void transaction records, 4 devices, 12 restore points and 10 sync runs. All source account, rate, deduction, sequence, policy and audit records were copied. The original MySQL user remains unchanged; the existing desktop administrator was retained with a separate user ID so its login still works.

The installed application was reopened and signed in successfully after the switch. Its live dashboard displays 276 registered members and 4 devices, with populated collection figures. MySQL auto-increment counters were checked against SQLite's next-ID counters; no corrections were needed.

There are 143 audit records: 141 from MySQL, one preserved desktop backup event and one import event. The preserved desktop event retains its original ID in metadata and points to the preserved desktop account. Old login sessions were backed up and invalidated to prevent a previous desktop user ID from authenticating as an imported MySQL user.

## Backups

Stored outside the repository at:

`%LOCALAPPDATA%\ph.dmbc.water.development\transfers\mysql-20261025-061751-fb3419`

- `mysql-original.sql`: complete MySQL schema and data snapshot, including operational tables.
- `mysql-original.json`: structured copy with per-table row checksums.
- `desktop-before-import.sqlite` and `desktop-replaced.sqlite`: the previous desktop database.
- `desktop-before-application.key` and `desktop-before-initial-login.txt`: preserved desktop key and login file.
- `desktop-old-sessions`: prior desktop sessions.
- `manifest.json` / `completed.json`: counts, financial totals, checksums and transfer completion metadata.

These backups contain private association records and account hashes. They are not bundled in the installer or stored in the repository.

## Validation

All copied row values were compared with the MySQL snapshot using canonical per-table checksums. Bill and payment monetary totals match the source. SQLite integrity and foreign-key checks passed. The source was rechecked immediately before replacement to ensure it had not changed since the snapshot.

A dry run against isolated copies passed 16 assertions covering account and audit identity preservation, session invalidation, foreign-key failure protection and refusal to discard desktop-only business records. The installed database was then checked again, including all original source rows, financial totals and preservation of the desktop login/key.

Laravel migrations remain native to SQLite; MySQL migration history, caches, active sessions, password-reset tokens and empty job queues were not copied into the active desktop database. They remain in the full MySQL backups. This avoids importing stale login sessions or the wrong database migration history.

This is a one-time data copy. Future MySQL edits and desktop SQLite edits do not automatically synchronize.

## Source files

Keep `src-tauri` source, configuration, Cargo lockfile, permissions and icons in Git. The `.gitignore` excludes its `target`, `resources` and `gen` directories, as well as local SQLite files and developer transfer/test data directories.

The developer utilities `transfer-mysql.php`, `test-transfer.php`, `verify-transfer.php` and `check-account.php` support the recorded staging, dry run, validation and login checks. The transfer tool refuses replacement if the desktop already has customers, readings, bills, transactions, devices, sync runs or restore points; future transfers with changes on both sides require an explicit merge.
