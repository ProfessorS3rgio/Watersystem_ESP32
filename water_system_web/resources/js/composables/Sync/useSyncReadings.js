import { ref } from 'vue'
import { serialService } from '../../services/serialService'
import { databaseService } from '../../services/databaseService'

export function useSyncReadings() {
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

  const exportReadingsFromDevice = async () => {
    if (exportInProgress.value) {
      throw new Error('Export already in progress')
    }

    exportInProgress.value = true
    exportLines.value = []
    lastAck.value = null
    exportBeginMarker.value = 'BEGIN_READINGS'
    exportEndMarker.value = 'END_READINGS'
    exportLinePrefix.value = 'READ|'

    const promise = new Promise((resolve, reject) => {
      exportResolve.value = resolve
      exportReject.value = reject
      exportTimeoutId.value = setTimeout(() => {
        finishExport(new Error('Timed out waiting for device export'))
      }, 12000)
    })

    await sendLine('EXPORT_READINGS')

    const lines = await promise
    return parseReadingsExportLines(lines)
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

  const parseReadingsExportLines = (lines) => {
    const readings = []
    for (const line of lines) {
      if (!line.startsWith('READ|')) continue
      const parts = line.split('|')
      // READ|customer_id|device_id|previous_reading|current_reading|usage_m3|reading_at_epoch
      if (parts.length < 7) continue
      readings.push({
        customer_id: Number(parts[1] || 0),
        device_id: Number(parts[2] || 1),
        previous_reading: Number(parts[3] || 0),
        current_reading: Number(parts[4] || 0),
        usage_m3: Number(parts[5] || 0),
        reading_at: Number(parts[6] || 0),
      })
    }
    return readings
  }

  const syncReadingsFromDevice = async () => {
    // Ensure device has a usable clock (epoch seconds)
    const epochNow = Math.floor(Date.now() / 1000)
    await sendLine('SET_TIME|' + String(epochNow))

    const deviceReadings = await exportReadingsFromDevice()

    if (deviceReadings.length === 0) {
      return
    }

    const processed = await databaseService.upsertReadingsToDatabase(deviceReadings)

    // Mark device readings as synced (so next export is incremental)
    await sendLine('READINGS_SYNCED')

    return processed
  }

  const handleDeviceLine = (line) => {
    if (!line) return

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
    syncReadingsFromDevice,
    exportReadingsFromDevice,
    handleDeviceLine
  }
}