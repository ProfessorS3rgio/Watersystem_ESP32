import { ref } from 'vue'
import { serialService } from '../../services/serialService'

export function useSyncDevice() {
  const deviceId = ref(null)
  const brgyId = ref(null)
  const firmwareVersion = ref('-')
  const lastSync = ref('Never')
  const pendingRecords = ref(0)
  const printCount = ref(0)
  const customerCount = ref(0)
  const sdTotalBytes = ref(0)
  const sdUsedBytes = ref(0)
  const sdFreeBytes = ref(0)
  const cpuFreqMhz = ref(0)
  const heapFreeBytes = ref(0)

  const exportInProgress = ref(false)
  const exportLines = ref([])
  const exportResolve = ref(null)
  const exportReject = ref(null)
  const exportTimeoutId = ref(null)
  const exportBeginMarker = ref(null)
  const exportEndMarker = ref(null)
  const exportLinePrefix = ref(null)
  const lastAck = ref(null)

  const sendLine = async (line) => {
    const text = String(line).replace(/\r|\n/g, '')
    await serialService.sendLine(text)
  }

  const exportDeviceInfoFromDevice = async () => {
    if (exportInProgress.value) {
      throw new Error('Export already in progress')
    }

    exportInProgress.value = true
    exportLines.value = []
    lastAck.value = null
    exportBeginMarker.value = 'BEGIN_DEVICE_INFO'
    exportEndMarker.value = 'END_DEVICE_INFO'
    exportLinePrefix.value = 'INFO|'

    const promise = new Promise((resolve, reject) => {
      exportResolve.value = resolve
      exportReject.value = reject
      exportTimeoutId.value = setTimeout(() => {
        finishExport(new Error('Timed out waiting for device export'))
      }, 8000)
    })

    await sendLine('EXPORT_DEVICE_INFO')

    const lines = await promise
    return parseDeviceInfoLines(lines)
  }

  const finishExport = (error, lines = null) => {
    if (exportTimeoutId.value) {
      clearTimeout(exportTimeoutId.value)
      exportTimeoutId.value = null
    }

    const resolve = exportResolve.value
    const reject = exportReject.value
    exportResolve.value = null
    exportReject.value = null
    exportInProgress.value = false

    if (error) {
      if (reject) reject(error)
      return
    }
    if (resolve) resolve(lines || [])
  }

  const parseDeviceInfoLines = (lines) => {
    const out = {}
    for (const line of lines) {
      if (!line.startsWith('INFO|')) continue
      const parts = line.split('|')
      // INFO|key|value
      if (parts.length < 3) continue
      const key = parts[1]
      const value = parts.slice(2).join('|')
      if (
        key === 'pending_readings' ||
        key === 'print_count' ||
        key === 'last_sync_epoch' ||
        key === 'device_id' ||
        key === 'brgy_id' ||
        key === 'customer_count' ||
        key === 'sd_present' ||
        key === 'sd_total_bytes' ||
        key === 'sd_used_bytes' ||
        key === 'sd_free_bytes' ||
        key === 'cpu_freq_mhz' ||
        key === 'heap_free_bytes' ||
        key === 'heap_min_free_bytes' ||
        key === 'heap_max_alloc_bytes'
      ) {
        out[key] = Number(value || 0)
      } else {
        out[key] = value
      }
    }
    return out
  }

  const syncDeviceInfoToServer = async (deviceInfo) => {
    try {
      console.log('Syncing device info to server:', deviceInfo)
      // Prepare device info for server
      const serverDeviceInfo = {
        device_id: deviceInfo.device_id,
        brgy_id: deviceInfo.brgy_id,
        device_mac: deviceInfo.device_uid, // ESP32 MAC address
        device_uid: deviceInfo.device_uid,
        firmware_version: deviceInfo.firmware_version,
        device_name: `ESP32 Device ${deviceInfo.device_id}`,
        print_count: deviceInfo.print_count || 0,
        customer_count: deviceInfo.customer_count || 0,
      }

      console.log('Sending serverDeviceInfo:', serverDeviceInfo)
      await databaseService.syncDeviceInfoToDatabase(serverDeviceInfo)
      console.log('Device info synced successfully')
      return true
    } catch (error) {
      console.error('Failed to sync device info to server:', error)
      throw error
    }
  }

  const refreshDeviceInfo = async () => {
    try {
      const info = await exportDeviceInfoFromDevice()
      if (typeof info?.device_id === 'number') deviceId.value = info.device_id
      if (typeof info?.brgy_id === 'number') brgyId.value = info.brgy_id
      if (info?.firmware_version) firmwareVersion.value = info.firmware_version
      if (typeof info?.pending_readings === 'number') pendingRecords.value = info.pending_readings

      const last = Number(info?.last_sync_epoch || 0)
      lastSync.value = last > 0 ? new Date(last * 1000).toLocaleString() : 'Never'

      if (typeof info?.print_count === 'number') printCount.value = info.print_count
      if (typeof info?.customer_count === 'number') customerCount.value = info.customer_count

      if (typeof info?.sd_total_bytes === 'number') sdTotalBytes.value = info.sd_total_bytes
      if (typeof info?.sd_used_bytes === 'number') sdUsedBytes.value = info.sd_used_bytes
      if (typeof info?.sd_free_bytes === 'number') sdFreeBytes.value = info.sd_free_bytes

      if (typeof info?.cpu_freq_mhz === 'number') cpuFreqMhz.value = info.cpu_freq_mhz
      if (typeof info?.heap_free_bytes === 'number') heapFreeBytes.value = info.heap_free_bytes

      // Sync device info to server
      await syncDeviceInfoToServer(info)
    } catch (e) {
      console.error('Device info unavailable:', e?.message || String(e))
    }
  }

  const handleDeviceLine = (line) => {
    if (!line) return

    // Track acks (useful for debugging)
    if (line.startsWith('ACK|')) {
      lastAck.value = line
    }

    if (!exportInProgress.value) return

    if (exportBeginMarker.value && line === exportBeginMarker.value) {
      exportLines.value = []
      return
    }

    if (exportEndMarker.value && line === exportEndMarker.value) {
      const lines = exportLines.value.slice()
      exportLines.value = []
      finishExport(null, lines)
      return
    }

    if (exportLinePrefix.value && line.startsWith(exportLinePrefix.value)) {
      exportLines.value.push(line)
    }

    if (line.startsWith('ERR|')) {
      finishExport(new Error('Device error: ' + line))
    }
  }

  return {
    deviceId,
    brgyId,
    firmwareVersion,
    lastSync,
    pendingRecords,
    printCount,
    customerCount,
    sdTotalBytes,
    sdUsedBytes,
    sdFreeBytes,
    cpuFreqMhz,
    heapFreeBytes,
    exportDeviceInfoFromDevice,
    refreshDeviceInfo,
    syncDeviceInfoToServer,
    handleDeviceLine,
    sendLine
  }
}