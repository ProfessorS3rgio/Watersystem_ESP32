import { parseSyncChunks } from '../../utils/parseSyncChunks.js'
import { createExportWatchdog } from '../../utils/exportWatchdog.js'
import { ref } from 'vue'
import { serialService } from '../../services/serialService'

export function useSyncBills() {
  const exportInProgress = ref(false)
  const exportLines = ref([])
  const exportResolve = ref(null)
  const exportReject = ref(null)
  const watchdog = createExportWatchdog(error => finishExport(error))
  const exportBeginMarker = ref(null)
  const exportEndMarker = ref(null)
  const exportLinePrefix = ref(null)
  const lastAck = ref(null)

  const sendLine = async (line) => {
    const text = String(line).replace(/\r|\n/g, '')
    await serialService.sendLine(text)
  }

  const exportBillsFromDevice = async () => {
    if (exportInProgress.value) {
      throw new Error('Export already in progress')
    }

    exportInProgress.value = true
    exportLines.value = []
    lastAck.value = null
    exportBeginMarker.value = 'BEGIN_BILLS_JSON'
    exportEndMarker.value = 'END_BILLS_JSON'
    exportLinePrefix.value = 'BILLS_CHUNK|'

    const promise = new Promise((resolve, reject) => {
      exportResolve.value = resolve
      exportReject.value = reject
      watchdog.start(60000, 'Timed out waiting for bill export progress')
    })

    promise.catch(() => {})
    try { await sendLine('EXPORT_BILLS') } catch (error) { finishExport(error) }

    const lines = await promise
    return parseBillsExportLines(lines)
  }

  const finishExport = (error, lines = null) => {
    watchdog.stop()
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

  const parseBillsExportLines = lines => parseSyncChunks(lines, 'BEGIN_BILLS_JSON', 'BILLS_CHUNK|')

  const handleDeviceLine = (line) => {
    if (!exportInProgress.value) return

    if (line.startsWith(exportBeginMarker.value)) {
      watchdog.progress()
      exportLines.value = [line]
      return
    }

    if (line.startsWith('ERR|')) { finishExport(new Error(line)); return }
    if (line === exportEndMarker.value) {
      finishExport(null)
      return
    }

    exportLines.value.push(line)
    if (line.startsWith('BILLS_CHUNK|')) watchdog.progress()
  }

  return {
    exportBillsFromDevice,
    handleDeviceLine
  }
}
