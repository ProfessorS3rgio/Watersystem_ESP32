import assert from 'node:assert/strict'
import { createServer } from 'vite'

// Exercise the orchestration without a serial port, network calls, or database writes.
const server = await createServer({ configFile: false, server: { middlewareMode: true }, appType: 'custom' })
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
    exportDeviceInfoFromDevice: async () => { observed.push(flow.syncStage.value); return { device_id: 1, brgy_id: 1, customer_count: 1 } },
    syncReadingsFromDevice: async () => { observed.push(flow.syncStage.value) },
    syncBillsFromDevice: async () => {},
    exportBillTransactionsFromDevice: async () => 0,
    pushCustomersToDevice: async () => { observed.push(flow.syncStage.value) },
    refreshDeviceInfo: async () => { observed.push(flow.syncStage.value) },
    sendLineDevice: async () => {},
  }
  await flow.syncData(options)
  assert.deepEqual(observed, [0, 2, 3, 4], 'displayed stages follow actual transfer order')
  assert.equal(flow.syncStatus.value, 'completed')
  assert.equal(flow.isSyncing.value, false)

  db.finishSyncRun = async () => { throw new Error('history unavailable') }
  await flow.syncData({ ...options, syncReadingsFromDevice: async () => { throw new Error('USB disconnected') } })
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
  console.log('Sync workflow checks passed: stages, failure cleanup, duplicate-start protection.')
} finally {
  await server.close()
}
