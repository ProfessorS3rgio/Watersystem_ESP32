import { parseSyncChunks } from '../../utils/parseSyncChunks.js'
import { createExportWatchdog } from '../../utils/exportWatchdog.js'
import { ref } from 'vue'
import { serialService } from '../../services/serialService'

export function useSyncBillTransactions() {
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
    const text = String(line).replace(/\r|\n/g, ' ')
    await serialService.sendLine(text)
  }

  const exportBillTransactionsFromDevice = async () => {
    if (exportInProgress.value) {
      throw new Error('Export already in progress')
    }

    exportInProgress.value = true
    exportLines.value = []
    lastAck.value = null
    exportBeginMarker.value = 'BEGIN_BILL_TRANSACTIONS_JSON'
    exportEndMarker.value = 'END_BILL_TRANSACTIONS_JSON'
    exportLinePrefix.value = 'BILL_TRANSACTIONS_CHUNK|'

    const promise = new Promise((resolve, reject) => {
      exportResolve.value = resolve
      exportReject.value = reject
      watchdog.start(60000, 'Timed out waiting for transaction export progress')
    })

    promise.catch(() => {})
    try { await sendLine('EXPORT_BILL_TRANSACTIONS') } catch (error) { finishExport(error) }

    const lines = await promise
    return parseBillTransactionsExportLines(lines)
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
        exportResolve.value(lines)
      }
    }
    exportResolve.value = null
    exportReject.value = null
  }

  const parseBillTransactionsExportLines = lines => parseSyncChunks(lines, 'BEGIN_BILL_TRANSACTIONS_JSON', 'BILL_TRANSACTIONS_CHUNK|')

  const handleExportLine = (line) => {
    if (!exportInProgress.value) return

    const t = String(line || '')

    // Device sends BEGIN marker with chunk count, e.g. BEGIN_BILL_TRANSACTIONS_JSON|2
    if (exportBeginMarker.value && t.startsWith(exportBeginMarker.value)) {
      watchdog.progress()
      exportLines.value = []
      exportLines.value.push(t)
      return
    }

    if (exportEndMarker.value && t.startsWith(exportEndMarker.value)) {
      finishExport(null, exportLines.value.slice())
      return
    }

    exportLines.value.push(t)
    if (t.startsWith('BILL_TRANSACTIONS_CHUNK|')) watchdog.progress()

    if (t.startsWith('ERR|')) {
      finishExport(new Error('Device error: ' + t))
    }
  }

  const handleAck = (line) => {
    if (line.startsWith('ACK|BILL_TRANSACTIONS_SYNCED')) {
      lastAck.value = line
    }
  }

  return {
    exportBillTransactionsFromDevice,
    handleExportLine,
    handleAck
  }
}
