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
  const exportCollecting = ref(false)
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
    exportBeginMarker.value = 'BEGIN_READINGS_JSON'
    exportEndMarker.value = 'END_READINGS_JSON'
    exportLinePrefix.value = 'READINGS_CHUNK|'

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
    exportInProgress.value = false
    if (error) {
      if (exportReject.value) {
        exportReject.value(error)
      }
    } else {
      if (exportResolve.value) {
        exportResolve.value(lines || exportLines.value)
      }
    }
    exportResolve.value = null
    exportReject.value = null
  }

  const parseReadingsExportLines = (lines) => {
    const readings = []
    let chunkBuffer = ''
    for (const line of lines) {
      const t = String(line).trim()

      if (t.startsWith('READINGS_CHUNK|')) {
        const parts = t.split('|', 3)
        if (parts.length >= 3) {
          chunkBuffer = parts[2] || ''
        }
        continue
      }

      // If there's no explicit chunk prefix, start accumulation when a line
      // looks like the start of JSON or contains reading keys (heuristic).
      if (!chunkBuffer) {
        // Start accumulating on any non-meta, non-empty line. Device may
        // emit fragments starting mid-object (e.g. 'stomer_id":273').
        if (t && !t.startsWith('Heap free') && t !== 'END_READINGS_JSON' && t !== 'BEGIN_READINGS_JSON') {
          chunkBuffer = t
        } else {
          // nothing to accumulate yet
          continue
        }
      } else {
        if (t.startsWith('Heap free') || t === 'END_READINGS_JSON' || t === 'BEGIN_READINGS_JSON') {
          // skip meta
        } else {
          chunkBuffer += t
        }
      }

      if (chunkBuffer) {
        try {
          const chunkReadings = JSON.parse(chunkBuffer)
          for (const reading of chunkReadings) {
            readings.push({
              reading_id: Number(reading.reading_id || 0),
              customer_id: Number(reading.customer_id || 0),
              device_uid: reading.device_uid || '',
              previous_reading: Number(reading.previous_reading || 0),
              current_reading: Number(reading.current_reading || 0),
              usage_m3: Number(reading.usage_m3 || 0),
              reading_at: Number(reading.reading_at || 0)
            })
          }
          chunkBuffer = '' // Clear after successful parse
        } catch (e) {
          // Not complete yet, continue accumulating
        }
      }
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

    console.log('Readings exported from device (count):', deviceReadings.length)
    let processed = 0
    try {
      processed = await databaseService.upsertReadingsToDatabase(deviceReadings)
      console.log('Readings synced to database, processed:', processed)
      if (Number(processed) !== Number(deviceReadings.length)) {
        throw new Error('Upserted readings count does not match device export count')
      }
    } catch (err) {
      console.error('Failed to upsert readings to database:', err)
      throw err
    }

    // Mark device readings as synced (so next export is incremental)
    await sendLine('READINGS_SYNCED')

    return processed
  }

  const handleDeviceLine = (line) => {
    if (!line) return

    if (!exportInProgress.value) return

    // Begin marker: start collecting
    if (exportBeginMarker.value && line.startsWith(exportBeginMarker.value)) {
      exportLines.value = []
      exportCollecting.value = true
      return
    }

    // End marker: stop collecting and finish

    if (exportEndMarker.value && line.startsWith(exportEndMarker.value)) {
      exportCollecting.value = false
      const lines = exportLines.value.slice()
      exportLines.value = []
      finishExport(null, lines)
      return
    }

    // Capture every incoming line while export is in progress (includes READINGS_CHUNK, heap prints, fragments)
    exportLines.value.push(line)

    if (line.startsWith('ERR|')) {
      exportCollecting.value = false
      finishExport(new Error('Device error: ' + line))
    }
  }

  return {
    syncReadingsFromDevice,
    exportReadingsFromDevice,
    handleDeviceLine
  }
}