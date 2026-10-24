import assert from 'node:assert/strict'
import { createServer } from 'vite'

globalThis.document = { createElement: () => ({}), querySelector: () => ({ getAttribute: () => 'csrf-test' }) }
const commands = []
let committed = false
let failImport = false
let failExport = false
globalThis.fetch = async (url, options) => {
  if (url.endsWith('/connect')) return Response.json({ device_id: 1 })
  const { command } = JSON.parse(options.body)
  commands.push(command)
  if (command.startsWith('UPSERT_SETTINGS|')) return new Response('ACK|UPSERT|Settings\n')
  if (command === 'EXPORT_BILLS') {
    if (failExport) return Response.json({ message: 'Device disconnected' }, { status: 503 })
    return new Response('BEGIN_BILLS_JSON|1\nBILLS_CHUNK|0|[{"bill_id":1,"reference_number":"TEST|1","total_due":10}]\nEND_BILLS_JSON\n')
  }
  if (command === 'BILLS_SYNCED') {
    assert.equal(committed, true, 'device records must not be acknowledged before website commit')
    return new Response('ACK|BILLS_SYNCED\n')
  }
  if (command.startsWith('SET_TIME|')) return new Response('ACK|SET_TIME\n')
  if (command === 'EXPORT_READINGS') return new Response('BEGIN_READINGS_JSON|1\nREADINGS_CHUNK|0|[{"reading_id":1,"customer_account_number":"M-001","reading_at":1700000000}]\nEND_READINGS_JSON\n')
  if (command === 'READINGS_SYNCED') return new Response('ACK|READINGS_SYNCED\n')
  if (command === 'EXPORT_BILL_TRANSACTIONS') return new Response('BEGIN_BILL_TRANSACTIONS_JSON|1\nBILL_TRANSACTIONS_CHUNK|0|[{"bill_transaction_id":1,"notes":"cash | office"}]\nEND_BILL_TRANSACTIONS_JSON\n')
  if (command === 'BILL_TRANSACTIONS_SYNCED') return new Response('ACK|BILL_TRANSACTIONS_SYNCED\n')
  throw new Error('Unexpected test command: ' + command)
}

const server = await createServer({ configFile: false, server: { middlewareMode: true }, appType: 'custom' })
try {
  const { serialService } = await server.ssrLoadModule('/resources/js/services/serialService.js')
  const { databaseService } = await server.ssrLoadModule('/resources/js/services/databaseService.js')
  const { useSyncSettings } = await server.ssrLoadModule('/resources/js/composables/Sync/useSyncSettings.js')
  const { useSyncBills } = await server.ssrLoadModule('/resources/js/composables/Sync/useSyncBills.js')
  const { useSyncReadings } = await server.ssrLoadModule('/resources/js/composables/Sync/useSyncReadings.js')
  const { useSyncBillTransactions } = await server.ssrLoadModule('/resources/js/composables/Sync/useSyncBillTransactions.js')
  const settings = useSyncSettings()
  const bills = useSyncBills()
  const readings = useSyncReadings()
  const transactions = useSyncBillTransactions()
  serialService.onLine(line => { settings.handleAck(line); bills.handleDeviceLine(line); readings.handleDeviceLine(line); transactions.handleExportLine(line) })
  databaseService.upsertReadingsToDatabase = async rows => ({ processed: rows.length, skipped: 0 })
  databaseService.syncBillTransactionsToDatabase = async rows => { assert.equal(rows[0].notes, 'cash | office'); return rows.length }
  databaseService.upsertBillsToDatabase = async rows => {
    if (failImport) throw new Error('Import failed')
    assert.equal(rows[0].reference_number, 'TEST|1')
    committed = true
    return rows.length
  }
  databaseService.markBillsSynced = async () => 1
  await serialService.connectWifi('ABCD1234')
  assert.equal(serialService.getState().transport, 'wifi')
  assert.ok(serialService.getState().writer, 'existing sync handlers see a connected transport')
  await settings.pushSettingsToDevice([{ id: 1, bill_due_days: 5, disconnection_days: 8 }])
  await readings.syncReadingsFromDevice()
  assert.equal(commands.at(-1), 'READINGS_SYNCED')
  await transactions.exportBillTransactionsFromDevice()
  assert.equal(commands.at(-1), 'BILL_TRANSACTIONS_SYNCED')
  databaseService.syncBillTransactionsToDatabase = async () => 0
  const transactionAcks = commands.filter(c => c === 'BILL_TRANSACTIONS_SYNCED').length
  await assert.rejects(transactions.exportBillTransactionsFromDevice(), /left unsynced/)
  assert.equal(commands.filter(c => c === 'BILL_TRANSACTIONS_SYNCED').length, transactionAcks)
  await bills.syncBillsFromDevice()
  assert.equal(commands.at(-1), 'BILLS_SYNCED')
  const acknowledgments = commands.filter(c => c === 'BILLS_SYNCED').length
  failImport = true
  committed = false
  await assert.rejects(bills.syncBillsFromDevice(), /Import failed/)
  assert.equal(commands.filter(c => c === 'BILLS_SYNCED').length, acknowledgments)
  failExport = true
  await assert.rejects(bills.syncBillsFromDevice(), /Device disconnected/)
  assert.equal(serialService.getState().isConnected, false)
  console.log('Wi-Fi integration checks passed: immediate ACKs, commit-before-acknowledge, failed import, disconnected exports.')
} finally {
  await server.close()
}
