import { parseSyncChunks } from '../../utils/parseSyncChunks.js'
import { createExportWatchdog } from '../../utils/exportWatchdog.js'
import { ref } from 'vue'
import { serialService } from '../../services/serialService'
import { databaseService } from '../../services/databaseService'

export function useSyncReadings() {
  const exportInProgress = ref(false)
  const exportLines = ref([])
  const exportResolve = ref(null)
  const exportReject = ref(null)
  const exportWatchdog = createExportWatchdog(error => finishExport(error))
  const exportBeginMarker = ref(null)
  const exportEndMarker = ref(null)
  const exportLinePrefix = ref(null)
  const exportCollecting = ref(false)
  const lastAck = ref(null)

  const sendLine = async (line) => {
    const text = String(line).replace(/\r|\n/g, '')
    await serialService.sendLine(text)
  }

  const parseCreatedAtToEpoch = (createdAt) => {
    const value = String(createdAt || '').trim()
    if (!value) return 0
    // Device sends local datetime without timezone: YYYY-MM-DD HH:mm:ss
    const normalized = value.replace(' ', 'T')
    const ts = Date.parse(normalized)
    if (Number.isNaN(ts)) return 0
    return Math.floor(ts / 1000)
  }

  const exportReadingsFromDevice = async (includeSynced = false) => {
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
      exportWatchdog.start(60000, 'Timed out waiting for device export progress')
    })

    promise.catch(() => {})
    try { await sendLine(includeSynced ? 'EXPORT_ALL_READINGS' : 'EXPORT_READINGS') } catch (error) { finishExport(error) }

    const lines = await promise
    return parseReadingsExportLines(lines)
  }

  const finishExport = (error, lines = null) => {
    exportWatchdog.stop()
    exportCollecting.value = false
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

  const parseReadingsExportLines = lines => parseSyncChunks(lines, exportBeginMarker.value, 'READINGS_CHUNK|').map(reading => ({
    ...reading,
    reading_id: Number(reading.reading_id || 0),
    customer_id: Number(reading.customer_id || 0),
    previous_reading: Number(reading.previous_reading || 0),
    current_reading: Number(reading.current_reading || 0),
    usage_m3: Number(reading.usage_m3 || 0),
    reading_at: Number(reading.reading_at || 0) >= 1577836800 ? Number(reading.reading_at) : parseCreatedAtToEpoch(reading.created_at),
  }))

  const inspectReadingsFromDevice = async () => {
    const readings = await exportReadingsFromDevice(true)
    return databaseService.inspectDeviceReadings(readings)
  }

  const inspectReadingsForAccount = async (accountNo) => {
    const account = String(accountNo || '').trim().toUpperCase()
    if (!/^[A-Z0-9-]{1,64}$/.test(account)) {
      throw new Error('Enter a valid account number, for example B-001')
    }
    if (exportInProgress.value) throw new Error('Export already in progress')

    exportInProgress.value = true
    exportLines.value = []
    exportBeginMarker.value = 'BEGIN_READINGS_QUERY_JSON'
    exportEndMarker.value = 'END_READINGS_QUERY_JSON'
    exportLinePrefix.value = 'READINGS_CHUNK|'
    const promise = new Promise((resolve, reject) => {
      exportResolve.value = resolve
      exportReject.value = reject
      exportWatchdog.start(15000, 'Timed out waiting for device query progress')
    })
    promise.catch(() => {})
    try { await sendLine('QUERY_READINGS|' + account) } catch (error) { finishExport(error) }
    return databaseService.inspectDeviceReadings(parseReadingsExportLines(await promise))
  }

  const handleDeviceLine = (line) => {
    if (!line) return

    if (!exportInProgress.value) return

    // Human-readable diagnostics sent by QUERY_READINGS. They are shown in
    // the serial log but must not be appended to a JSON export chunk.
    if (line.startsWith('QUERY_RESULT|')) return

    // Begin marker: start collecting
    if (exportBeginMarker.value && line.startsWith(exportBeginMarker.value + '|')) {
      exportWatchdog.progress()
      exportLines.value = [line]
      exportCollecting.value = true
      return
    }

    // End marker: stop collecting and finish

    if (exportEndMarker.value && line === exportEndMarker.value) {
      exportCollecting.value = false
      const lines = exportLines.value.slice()
      exportLines.value = []
      finishExport(null, lines)
      return
    }

    // Capture every incoming line while export is in progress (includes READINGS_CHUNK, heap prints, fragments)
    exportLines.value.push(line)
    if (exportCollecting.value && line.startsWith('READINGS_CHUNK|')) exportWatchdog.progress()

    if (line.startsWith('ERR|')) {
      exportCollecting.value = false
      finishExport(new Error('Device error: ' + line))
    }
  }

  return {
    inspectReadingsFromDevice,
    inspectReadingsForAccount,
    exportReadingsFromDevice,
    handleDeviceLine
  }
}
