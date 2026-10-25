# Review incoming device data

DMBC 0.3.0 reads pending readings, bills, and transactions together, then opens a teal review dialog before importing them. The master needs firmware **v1.1.0** with `verified_sync_ack=1`; older firmware can still connect and export, but cannot approve incoming batches safely. Upload `Watersystem_ESP32/master/master.ino` using the existing Huge APP partition scheme. No SD formatting or database erase is needed.

## Operator workflow

1. Connect the master over USB or Wi-Fi and start sync. DMBC creates its existing restore point, reads the pending groups, and stages a review. Staging is separate from the reading, bill, and payment tables.
2. Review ready records, matching records already saved, warnings, and blocking issues. Search by customer, account, bill reference, or reading ID. Large batches are paginated.
3. Resolve blocking issues by cancelling and correcting the source data/customer assignment, then start another sync. Conflicting readings or financial records are never silently overwritten.
4. Accept any warnings explicitly, then choose **Approve & save batch**. DMBC re-reads the master to detect changes and the server rechecks the website state. Website changes require **Refresh checks** and another approval; device changes require cancelling and reading a fresh batch.
5. Readings, bills, transactions, required device registration, and the approval receipt save in one transaction. Matching rows are skipped. Historical records can be linked to their device IDs without duplicating their contents.
6. Only after that transaction commits does DMBC confirm each group to the master. The firmware compares the last exported group with the current pending records under a write lock. New or edited records cause an error and leave that entire group pending. DMBC waits for the actual device acknowledgment.
7. Reference data and member uploads proceed after the incoming batch is confirmed. Recent verification receipts show the approver, counts, warnings accepted, and whether the master confirmed the saved data.

Cancel before approval leaves incoming business records and device sync flags untouched. If the connection drops after saving, the receipt remains **Saved · device confirmation pending**. Reconnect and sync again: exact matches are recognised instead of duplicated. An approval request can be retried against the same review ID without applying the import twice. If a save response was lost, DMBC must confirm cancellation with the server before dismissing that review.

## Checks and limits

- Customer existence and barangay assignment; stable device identity.
- Valid meter values, usage arithmetic, dates, repeated identities, duplicate transitions, continuity, and high usage warnings (over 50 m³ or three times recent average, whichever is larger).
- Bill/reading links, existing record conflicts, duplicate billing months, nonnegative amounts, total arithmetic, status/payment agreement, and overpayments.
- Transaction identity, bill reference, source, cash/change arithmetic, duplicate events, and conflicts with existing payment history.
- Paid bills with missing historical payment data require an explicit warning acceptance. Adding missing history to a bill already paid does not increase its balance again.
- Whole-batch approval only; no partial approval. Up to 5,000 rows per group; reviews expire after 30 minutes. Any signed-in operator can approve their own review under the application's current access model. This does not add new user roles.

Reviews retain the validation details and approval receipt. Staged payloads are encrypted with the existing application key, cleared on approval/cancellation, and removed from expired reviews when that operator stages another batch. Pending previews should be restarted after moving a database to another application key. Backup snapshots preserve review history across restores, like the audit history; a restored dataset is rechecked before any pending approval.

The previous direct reading/bill/transaction import endpoints now return HTTP 409 directing clients to the review flow. Browser and desktop clients use the same server-side validation and atomic import.

## Verification performed

Automated coverage checks read-only previews, cancellation, warnings, stale reviews, ownership, expiry, duplicate/conflicting records, idempotent approvals, transaction rollback, device acknowledgment order, disconnected acknowledgments, and the firmware's new/edited-record guards. Physical device sync over USB and Wi-Fi still needs a check after uploading the firmware.
