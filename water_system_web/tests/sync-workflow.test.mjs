import assert from 'node:assert/strict'
import { createServer } from 'vite'

// Exercise the orchestration without a serial port, network calls, or database writes.
const server = await createServer({ configFile: false, optimizeDeps: { noDiscovery: true, entries: [] }, server: { middlewareMode: true, hmr: false }, appType: 'custom' })
try {
  const { useSyncData } = await server.ssrLoadModule('/resources/js/composables/Sync/useSyncData.js')
  const { databaseService: db } = await server.ssrLoadModule('/resources/js/services/databaseService.js')
  const { serialService: serial } = await server.ssrLoadModule('/resources/js/services/serialService.js')
  serial.getState = () => ({ writer: {} })
  db.startSyncRun = async () => {}
  db.createRestorePoint = async () => ({ row_count: 0 })
  db.syncDeviceInfoToDatabase = async () => {}
  db.fetchCustomersFromDatabase = async () => []
  db.fetchCustomerTypesFromDatabase = async () => []
  db.fetchDeductionsFromDatabase = async () => []
  db.fetchBarangaysFromDatabase = async () => []
  db.fetchSettingsFromDatabase = async () => null
  db.finishSyncRun = async () => {}

  const flow = useSyncData()
  const observed = []
  const options = {
    isConnected: true,
    exportDeviceInfoFromDevice: async () => { observed.push(flow.syncStage.value); return { device_id: 1, brgy_id: 1, customer_count: 1, verified_sync_ack:1 } },
    exportReadingsFromDevice: async () => { observed.push(flow.syncStage.value); return [] },
    exportBillsFromDevice: async () => [],
    exportBillTransactionsFromDevice: async () => [],
    pushCustomersToDevice: async () => { observed.push(flow.syncStage.value) },
    refreshDeviceInfo: async () => { observed.push(flow.syncStage.value) },
    sendLineDevice: async () => {},
  }
  await flow.syncData(options)
  assert.deepEqual(observed, [0, 1, 3, 4], 'displayed stages follow actual transfer order')
  assert.equal(flow.syncStatus.value, 'completed')
  assert.equal(flow.isSyncing.value, false)

  db.finishSyncRun = async () => { throw new Error('history unavailable') }
  await flow.syncData({ ...options, exportReadingsFromDevice: async () => { throw new Error('USB disconnected') } })
  assert.equal(flow.syncStatus.value, 'failed')
  assert.equal(flow.syncError.value, 'USB disconnected')
  assert.equal(flow.isSyncing.value, false, 'history failures must release the busy state')

  let release
  const gate = new Promise(resolve => { release = resolve })
  let starts = 0
  db.startSyncRun = async () => { starts++; await gate }
  db.finishSyncRun = async () => {}
  const running = flow.syncData(options)
  await flow.syncData(options)
  assert.equal(starts, 1, 'duplicate starts must not create overlapping sync sessions')
  release()
  await running
  assert.equal(flow.isSyncing.value, false)
  const events = [], listeners = new Set()
  let approved = false
  serial.onLine = callback => { listeners.add(callback); return () => listeners.delete(callback) }
  serial.onStatus = callback => { callback({ isConnected: true }); return () => {} }
  serial.sendLine = async command => {
    assert.equal(approved,true,'never acknowledge before the batch commits')
    events.push(command)
    for (const listener of listeners) listener('ACK|' + command)
  }
  db.acknowledgeSyncReview = async () => { events.push('receipt-confirmed') }
  let allowApproval
  const approval = new Promise(resolve => { allowApproval = resolve })
  const pendingOptions = { ...options,
    exportReadingsFromDevice: async () => [{ reading_id: 1 }],
    exportBillsFromDevice: async () => [{ bill_id: 2 }],
    exportBillTransactionsFromDevice: async () => [{ bill_transaction_id: 3 }],
    reviewIncomingBatch: async (batch, recheck) => {
      assert.equal(batch.transactions.length,1)
      await approval
      await recheck()
      approved = true
      return { review_id: 'test-receipt' }
    },
  }
  const pendingRun = flow.syncData(pendingOptions)
  await new Promise(resolve => setTimeout(resolve,20))
  assert.deepEqual(events,[],'review pauses the entire incoming import')
  allowApproval()
  await pendingRun
  assert.deepEqual(events,['READINGS_SYNCED','BILLS_SYNCED','BILL_TRANSACTIONS_SYNCED','receipt-confirmed'])
  assert.equal(flow.syncStatus.value,'completed')
  approved = false; events.length = 0
  await flow.syncData({ ...pendingOptions, reviewIncomingBatch: async () => { throw Object.assign(new Error('Cancelled'),{ code:'SYNC_CANCELLED' }) } })
  assert.equal(flow.syncStatus.value,'cancelled')
  assert.deepEqual(events,[])
  approved = true
  serial.sendLine = async () => { throw new Error('USB removed after save') }
  await flow.syncData({ ...pendingOptions, reviewIncomingBatch: async () => ({review_id:'saved-receipt'}) })
  assert.equal(flow.syncStatus.value,'failed')
  assert.match(flow.syncError.value,/USB removed after save/)
  assert.equal(listeners.size,0,'failed acknowledgments clean up their listeners')
  console.log('Sync workflow checks passed: stages, failure cleanup, duplicate-start protection.')
} finally {
  await server.close()
}
