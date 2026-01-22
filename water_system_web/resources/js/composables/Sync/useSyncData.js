import { ref } from 'vue'
import { serialService } from '../../services/serialService'
import { databaseService } from '../../services/databaseService'

export function useSyncData() {
  const isSyncing = ref(false)
  const syncStartTime = ref(null)
  const syncLogs = ref([])

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
    syncReadingsFromDevice,
    pushCustomersToDevice,
    refreshDeviceInfo,
    sendLineDevice,
    onSyncSuccess
  }) => {
    const st = serialService.getState()
    if (!isConnected || !st.writer) {
      addLog('Not connected. Connect a device first.')
      return
    }

    isSyncing.value = true
    syncStartTime.value = Date.now()

    try {
      addLog('Starting customer sync...')

      // Get device info first to know which barangay this device belongs to
      addLog('Getting device information...')
      const deviceInfo = await exportDeviceInfoFromDevice()
      const deviceBrgyId = deviceInfo?.brgy_id || 1
      const deviceId = deviceInfo?.device_id || 1
      const lastSyncEpoch = deviceInfo?.last_sync_epoch || 0
      addLog(`Device ID: ${deviceId}, Barangay ID: ${deviceBrgyId}, Last Sync: ${lastSyncEpoch}`)

      // Get customers from web database, filtered by barangay and updated after last sync
      addLog('Fetching customers from database...')
      let dbCustomers = await databaseService.fetchCustomersFromDatabase({ brgy_id: deviceBrgyId, updated_after: lastSyncEpoch })
      addLog(`Database has ${dbCustomers.length} customers updated since last sync`)

      // If device has no customers, perform full sync
      if (deviceInfo.customer_count === 0) {
        addLog('Device has no customers, performing full customer sync...')
        dbCustomers = await databaseService.fetchCustomersFromDatabase({ brgy_id: deviceBrgyId })
        addLog(`Full sync: pushing ${dbCustomers.length} total customers to ESP32`)
      }

      // No need to filter further, API already filtered
      const filteredDbCustomers = dbCustomers

      // Sync customer types from DB to device first (needed for customer references)
      addLog('Fetching customer types from database...')
      const dbCustomerTypes = await databaseService.fetchCustomerTypesFromDatabase()
      addLog(`Database has ${dbCustomerTypes.length} customer types`)
      addLog(`Pushing ${dbCustomerTypes.length} customer types to ESP32...`)
      await pushCustomerTypesToDevice(dbCustomerTypes)
      addLog('✓ Customer types sync completed')

      // Sync deductions from DB to device
      addLog('Fetching deductions from database...')
      const dbDeductions = await databaseService.fetchDeductionsFromDatabase()
      addLog(`Database has ${dbDeductions.length} deductions`)
      addLog(`Pushing ${dbDeductions.length} deductions to ESP32...`)
      await pushDeductionsToDevice(dbDeductions)
      addLog('✓ Deductions sync completed')

      // Sync readings (device -> DB)
      await syncReadingsFromDevice()

      // Separate customers into new and updated
      const { newCustomers, updatedCustomers } = separateCustomersBySyncStatus(filteredDbCustomers)

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
      if (filteredDbCustomers.length > 0) {
        addLog('Marking customers as synced in web database...')
        const accountNumbers = filteredDbCustomers.map(c => c.account_no)
        await databaseService.markCustomersSynced(accountNumbers)
        addLog(`✓ Marked ${accountNumbers.length} customers as synced`)
      }

      // Always update last sync on the device at the end of a successful run.
      // (If there were no new readings, READINGS_SYNCED isn't sent, so device last_sync_epoch would stay 0.)
      const epochNow = Math.floor(Date.now() / 1000)
      await sendLineDevice('SET_LAST_SYNC|' + String(epochNow))

      await refreshDeviceInfo()

      // Reload SD card to apply new settings
      addLog('Reloading SD card on ESP32...')
      await sendLineDevice('RELOAD_SD')
      // Wait a moment for reload
      await new Promise(r => setTimeout(r, 1000))
      await refreshDeviceInfo()

      // Show success dialog
      const elapsed = Date.now() - syncStartTime.value
      addLog('Sync completed in ' + formatElapsedTime(elapsed))
      if (onSyncSuccess) {
        onSyncSuccess()
      }
    } catch (error) {
      addLog('Sync failed: ' + (error?.message || String(error)))
    } finally {
      isSyncing.value = false
      syncStartTime.value = null
    }
  }

  return {
    isSyncing,
    syncLogs,
    syncData,
    addLog,
    formatElapsedTime
  }
}