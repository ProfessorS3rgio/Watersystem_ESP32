import { ref, computed } from 'vue'
import { serialService } from '../../services/serialService'
import { databaseService } from '../../services/databaseService'
import { syncSnapshot, acknowledgeVerifiedGroup } from '../../utils/verifiedSync.js'

export function useSyncData() {
  const isSyncing = ref(false)
  const syncStage = ref(-1)
  const syncStatus = ref('idle')
  const syncError = ref('')
  const syncStartTime = ref(null)
  const syncLogs = ref([])

  const isJsonLog = (message) => {
    if (!message.startsWith('Device ← ')) return false
    const dataPart = message.substring(10).trim()
    return (
      dataPart.includes('"bill_id"') ||
      dataPart.includes('"reading_id"') ||
      dataPart.includes('"customer_id"') ||
      dataPart.includes('"bill_transaction_id"') ||
      dataPart.startsWith('BILL_TRANSACTIONS_CHUNK|')
    )
  }

  const filteredSyncLogs = computed(() => syncLogs.value.filter(log => !isJsonLog(log.message)))

  const addLog = (message) => {
    const time = new Date().toLocaleTimeString()
    syncLogs.value.push({ time, message })

    // Auto-scroll to bottom (this will be handled by the component)
  }

  const formatElapsedTime = (ms) => {
    const seconds = Math.floor(ms / 1000)
    const minutes = Math.floor(seconds / 60)
    const remainingSeconds = seconds % 60
    if (minutes > 0) {
      return `${minutes}m ${remainingSeconds}s`
    } else {
      return `${remainingSeconds}s`
    }
  }

  const separateCustomersBySyncStatus = (customers) => {
    const newCustomers = customers.filter(c => !c.Synced && c.last_sync === null)
    const updatedCustomers = customers.filter(c => !c.Synced && c.last_sync !== null)
    return { newCustomers, updatedCustomers }
  }

  const syncData = async ({
    isConnected,
    exportDeviceInfoFromDevice,
    pushCustomerTypesToDevice,
    pushDeductionsToDevice,
    pushBarangaysToDevice,
    pushSettingsToDevice,
    exportReadingsFromDevice,
    exportBillsFromDevice,
    exportBillTransactionsFromDevice,
    reviewIncomingBatch,
    pushCustomersToDevice,
    refreshDeviceInfo,
    sendLineDevice,
    onSyncSuccess
  }) => {
    if (isSyncing.value) return
    const st = serialService.getState()
    if (!isConnected || !st.writer) {
      addLog('Not connected. Connect a device first.')
      return
    }

    isSyncing.value = true
    syncStage.value = 0
    syncStatus.value = 'running'
    syncError.value = ''
    syncStartTime.value = Date.now()
    let syncRunStarted = false
    let syncRunStatus = 'completed'
    let verificationReceipt = null

    try {
      await databaseService.startSyncRun()
      syncRunStarted = true
      addLog('Creating restore point for current web data...')
      const restorePoint = await databaseService.createRestorePoint()
      addLog(`✓ Restore point created (${restorePoint.row_count} rows)`)
      addLog('Starting customer sync...')

      // Get device info first to know which barangay this device belongs to
      addLog('Getting device information...')
      const deviceInfo = await exportDeviceInfoFromDevice()
      const deviceBrgyId = deviceInfo?.brgy_id || 1
      const deviceId = deviceInfo?.device_id || 1
      const lastSyncEpoch = deviceInfo?.last_sync_epoch || 0
      addLog(`Device ID: ${deviceId}, Barangay ID: ${deviceBrgyId}, Last Sync: ${lastSyncEpoch}`)

      // Gather every incoming group before pushing or importing anything.
      syncStage.value = 1
      addLog('Reading pending records for verification…')
      const batch = {
        device: deviceInfo,
        readings: await exportReadingsFromDevice(),
        bills: await exportBillsFromDevice(),
        transactions: await exportBillTransactionsFromDevice(),
      }
      if (batch.readings.length + batch.bills.length + batch.transactions.length > 0) {
        addLog('Waiting for batch review. No incoming records have been saved.')
        const recheck = async () => {
          const currentDevice = await exportDeviceInfoFromDevice()
          if (currentDevice.device_uid !== deviceInfo.device_uid || Number(currentDevice.brgy_id) !== Number(deviceInfo.brgy_id)) throw new Error('Connected master changed. Cancel and start a new sync.')
          if (Number(currentDevice.verified_sync_ack) !== 1) throw new Error('The master needs firmware v1.1.0 before this batch can be approved.')
          const current = { readings: await exportReadingsFromDevice(), bills: await exportBillsFromDevice(), transactions: await exportBillTransactionsFromDevice() }
          for (const kind of ['readings', 'bills', 'transactions']) if (syncSnapshot(current[kind]) !== syncSnapshot(batch[kind])) throw new Error('Device records changed during review. Cancel and start a new sync to include them.')
        }
        verificationReceipt = await reviewIncomingBatch(batch, recheck)
        addLog(`✓ Approved batch saved. Receipt: ${verificationReceipt.review_id}`)
        // Each ACK is preceded by another read, then verified under a write
        // lock by the master firmware. A changed pending set is never marked.
        for (const [kind, read, command] of [
          ['readings', exportReadingsFromDevice, 'READINGS_SYNCED'],
          ['bills', exportBillsFromDevice, 'BILLS_SYNCED'],
          ['transactions', exportBillTransactionsFromDevice, 'BILL_TRANSACTIONS_SYNCED'],
        ]) {
          if (!batch[kind].length) continue
          if (syncSnapshot(await read()) !== syncSnapshot(batch[kind])) throw new Error('Website batch was saved, but the device changed before confirmation. Sync again; saved records will be recognised.')
          await acknowledgeVerifiedGroup(serialService, command)
        }
        await databaseService.acknowledgeSyncReview(verificationReceipt.review_id)
        addLog('✓ Master confirmed the saved batch.')
      } else addLog('No incoming readings, bills, or payments need review.')

      // Sync device info to server immediately
      try {
        console.log('Syncing device info to server:', deviceInfo)
        const serverDeviceInfo = {
          device_id: deviceInfo.device_id,
          brgy_id: deviceInfo.brgy_id,
          device_mac: deviceInfo.device_uid,
          device_uid: deviceInfo.device_uid,
          firmware_version: deviceInfo.firmware_version,
          device_name: `ESP32 Device ${deviceInfo.device_id}`,
          print_count: deviceInfo.print_count || 0,
          customer_count: deviceInfo.customer_count || 0,
        }
        console.log('Sending serverDeviceInfo:', serverDeviceInfo)
        await databaseService.syncDeviceInfoToDatabase(serverDeviceInfo)
        console.log('Device info synced successfully')
      } catch (error) {
        console.error('Failed to sync device info to server:', error)
        // Continue with sync even if device info sync fails
      }

      // Get customers from web database for this barangay, then filter by sync state.
      // This catches rows that were explicitly marked Synced = 0 even if their
      // updated_at timestamp did not move.
      addLog('Fetching customers from database...')
      const dbCustomers = await databaseService.fetchCustomersFromDatabase({ brgy_id: deviceBrgyId })
      let pendingCustomers = dbCustomers.filter(c => !c.Synced || c.last_sync === null)
      addLog(`Database has ${pendingCustomers.length} unsynced customers`)

      // If device has no customers, perform full sync
      if (deviceInfo.customer_count === 0) {
        addLog('Device has no customers, performing full customer sync...')
        pendingCustomers = dbCustomers
        addLog(`Full sync: pushing ${pendingCustomers.length} total customers to ESP32`)
      }

      // Sync customer types from DB to device first (needed for customer references)
      syncStage.value = 2
      addLog('Fetching customer types from database...')
      const dbCustomerTypes = await databaseService.fetchCustomerTypesFromDatabase()
      const unsyncedCustomerTypes = dbCustomerTypes.filter(ct => !ct.Synced || ct.last_sync === null)
      if (unsyncedCustomerTypes.length > 0) {
        addLog(`Database has ${unsyncedCustomerTypes.length} unsynced customer types`)
        addLog(`Pushing ${unsyncedCustomerTypes.length} customer types to ESP32...`)
        await pushCustomerTypesToDevice(unsyncedCustomerTypes)
        addLog('✓ Customer types sync completed')

        // Mark customer types as synced in web database
        addLog('Marking customer types as synced in web database...')
        const customerTypeIds = unsyncedCustomerTypes.map(ct => ct.type_id || ct.id)
        await databaseService.markCustomerTypesSynced(customerTypeIds)
        addLog(`✓ Marked ${customerTypeIds.length} customer types as synced`)
      } else {
        addLog('Customer types are already synced, skipping...')
      }

      // Sync deductions from DB to device
      addLog('Fetching deductions from database...')
      const dbDeductions = await databaseService.fetchDeductionsFromDatabase()
      const unsyncedDeductions = dbDeductions.filter(d => !d.Synced || d.last_sync === null)
      if (unsyncedDeductions.length > 0) {
        addLog(`Database has ${unsyncedDeductions.length} unsynced deductions`)
        addLog(`Pushing ${unsyncedDeductions.length} deductions to ESP32...`)
        await pushDeductionsToDevice(unsyncedDeductions)
        addLog('✓ Deductions sync completed')

        // Mark deductions as synced in web database
        addLog('Marking deductions as synced in web database...')
        const deductionIds = unsyncedDeductions.map(d => d.deduction_id || d.id)
        await databaseService.markDeductionsSynced(deductionIds)
        addLog(`✓ Marked ${deductionIds.length} deductions as synced`)
      } else {
        addLog('Deductions are already synced, skipping...')
      }

      // Sync barangays from DB to device
      addLog('Fetching barangays from database...')
      const dbBarangays = await databaseService.fetchBarangaysFromDatabase()
      const unsyncedBarangays = dbBarangays.filter(b => !b.Synced || b.last_sync === null)
      if (unsyncedBarangays.length > 0) {
        addLog(`Database has ${unsyncedBarangays.length} unsynced barangays`)
        addLog(`Pushing ${unsyncedBarangays.length} barangays to ESP32...`)
        await pushBarangaysToDevice(unsyncedBarangays)
        addLog('✓ Barangays sync completed')

        // Mark barangays as synced in web database
        addLog('Marking barangays as synced in web database...')
        const brgyIds = unsyncedBarangays.map(b => b.brgy_id || b.id)
        await databaseService.markBarangaysSynced(brgyIds)
        addLog(`✓ Marked ${brgyIds.length} barangays as synced`)
      } else {
        addLog('Barangays are already synced, skipping...')
      }

      // Sync settings from DB to device
      addLog('Fetching settings from database...')
      const dbSettings = await databaseService.fetchSettingsFromDatabase()
      if (dbSettings && (!dbSettings.Synced || dbSettings.last_sync === null)) {
        addLog(`Database has 1 unsynced setting`)
        addLog(`Pushing 1 setting to ESP32...`)
        await pushSettingsToDevice([dbSettings])
        addLog('✓ Settings sync completed')

        // Mark settings as synced in web database
        addLog('Marking settings as synced in web database...')
        const settingIds = [dbSettings.id]
        await databaseService.markSettingsSynced(settingIds)
        addLog(`✓ Marked 1 setting as synced`)

        // Reload SD card to apply new settings
        addLog('Reloading SD card on ESP32 to apply new settings...')
        await sendLineDevice('RELOAD_SD')
        // Wait a moment for reload
        await new Promise(r => setTimeout(r, 1000))
        await refreshDeviceInfo()
      } else {
        addLog('Settings are already synced, skipping...')
      }

      // Incoming records were already verified and saved together above.

      // Separate customers into new and updated
      syncStage.value = 3
      const { newCustomers, updatedCustomers } = separateCustomersBySyncStatus(pendingCustomers)

      addLog(`New customers: ${newCustomers.length}, Updated customers: ${updatedCustomers.length}`)

      // Push filtered DB customers to device (adds new, updates existing) - last to ensure dependencies
      if (newCustomers.length > 0) {
        addLog(`Pushing ${newCustomers.length} new customers to ESP32...`)
      }
      if (updatedCustomers.length > 0) {
        addLog(`Pushing ${updatedCustomers.length} updated customers to ESP32...`)
      }
      await pushCustomersToDevice(newCustomers, updatedCustomers)
      addLog(`Sent ${newCustomers.length} new customers and ${updatedCustomers.length} updated customers to device`)
      addLog('✓ Customer sync completed')

      // Mark customers as synced in web database
      if (pendingCustomers.length > 0) {
        addLog('Marking customers as synced in web database...')
        const accountNumbers = pendingCustomers.map(c => c.account_no)
        await databaseService.markCustomersSynced(accountNumbers)
        addLog(`✓ Marked ${accountNumbers.length} customers as synced`)
      }

      // Always update last sync on the device at the end of a successful run.
      syncStage.value = 4
      // (If there were no new readings, READINGS_SYNCED isn't sent, so device last_sync_epoch would stay 0.)
      const epochNow = Math.floor(Date.now() / 1000)
      await sendLineDevice('SET_LAST_SYNC|' + String(epochNow))

      await refreshDeviceInfo()

      // Show success dialog
      const elapsed = Date.now() - syncStartTime.value
      addLog('Sync completed in ' + formatElapsedTime(elapsed))
      syncStatus.value = 'completed'
      if (onSyncSuccess) {
        onSyncSuccess()
      }
    } catch (error) {
      syncRunStatus = error?.code === 'SYNC_CANCELLED' ? 'cancelled' : 'failed'
      syncStatus.value = syncRunStatus
      syncError.value = error?.response?.data?.message || error?.message || String(error)
      addLog(syncError.value)
    } finally {
      try {
        if (syncRunStarted) {
          await databaseService.finishSyncRun(syncRunStatus, {
            summary: { elapsed_ms: Date.now() - syncStartTime.value, verification: verificationReceipt },
            error: syncRunStatus === 'failed' ? syncError.value : null,
          })
        }
      } catch (error) {
        addLog('Could not save sync run history: ' + (error?.message || String(error)))
      } finally {
        isSyncing.value = false
        syncStartTime.value = null
      }
    }
  }

  return {
    isSyncing,
    syncStage,
    syncStatus,
    syncError,
    syncLogs,
    filteredSyncLogs,
    syncData,
    addLog,
    formatElapsedTime
  }
}
