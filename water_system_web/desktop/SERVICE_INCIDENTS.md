# DMBC service incidents

Available in the web app and Windows desktop version 0.2.0. Open **Operations → Service incidents**, or use **Report issue** on a member row to start a report with that account selected.

## Workflow

1. Record the member, issue type, short title, description and precise location/landmark. Choose a priority and identify whether the issue is before the meter, at its connection, after the meter, or still needs inspection. A crew name is optional.
2. Move a reported issue to **In progress** when inspection or repair starts. Use progress notes to record findings, materials needed or follow-up work.
3. Mark it **Resolved** with repair and verification notes. It may be resolved directly from Reported when the work is already complete.
4. Mark it **Closed** after confirming the outcome. Reopen a resolved/closed report into In progress with a reason if the problem returns. Cancel an incorrect or duplicate report with a reason; cancelled reports can be returned to Reported.

Closed, resolved and cancelled report details cannot be rewritten without reopening. Notes can still be appended. Reopening clears the current resolution/timestamps while preserving the original outcome in history, and a new repair resolution is required later.

The list supports live search by reference, account, member, location, barangay, title or crew; status, priority and issue filters; customer-specific views; and pagination. Urgent reports appear first. Selecting an account shows a warning when it already has open reports, allowing staff to review them before recording another issue.

Reports do not change meter readings, bills, penalties or connection status. Crew assignment is an internal record and does not send messages. This version records office reports; it does not add reporting screens or incident transfer commands to ESP32 firmware. Photos, GPS collection and customer self-service submission are not part of this version.

## Data and reliability

- `service_incidents` holds current report state, a human-readable reference, customer snapshots, priority, assignment, repair outcome and optimistic version.
- `incident_events` holds append-only activity records, actor snapshots, status changes, notes and field differences. Events are loaded in stable descending-ID pages.
- `IncidentService` owns workflow rules and transactions. A report update, its history entry and system audit record commit together; an audit failure rolls back all of them.
- Creation uses a unique client submission UUID and request fingerprint. Retrying the same request returns the original report; a different payload cannot reuse that submission key.
- Updates require the version last read by the editor. Stale updates receive a conflict response and retain the user's draft until they explicitly reload.
- Customer names, account numbers, addresses and barangay are captured when reporting. A deleted account does not erase its incident or timeline.
- Database backup archives include incidents and events. Restoring a billing/device snapshot preserves current incident history, even when the snapshot predates the report. Account links are restored only when the same account identity is present; otherwise they are detached with a history entry and the original customer context is retained.
- Reports and history are authenticated staff features. There are no report or event deletion endpoints and no history-edit endpoint. Model guards reject ordinary ORM history updates/deletions; these are application protections, not database tamper-proofing.

## Verification

The full PHP suite passed 39 tests (251 assertions), including 12 incident tests covering workflow validation, retry handling, conflicts, audit rollback, snapshots, customer deletion, stable history pages and isolation from financial records. Existing JavaScript checks passed. The packaged-backend smoke test exercised the incident API through the actual frontend request helper, including CSRF/session handling, repair/close/reopen, duplicate retries, stale conflicts and restore/restart persistence. All sample incident writes used temporary test profiles.

The migration only adds incident tables. The Windows launcher creates a pre-upgrade SQLite backup before applying pending migrations. Rollout checks fingerprint the existing desktop business tables and verify that accounts, bills, readings, transactions, settings and login/key material remain unchanged by the upgrade.
