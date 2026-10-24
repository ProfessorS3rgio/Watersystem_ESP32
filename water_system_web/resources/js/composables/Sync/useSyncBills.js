import { parseSyncChunks } from '../../utils/parseSyncChunks.js'
import { ref } from 'vue'
import { serialService } from '../../services/serialService'
import { databaseService } from '../../services/databaseService'

export function useSyncBills() {
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
      exportTimeoutId.value = setTimeout(() => {
        finishExport(new Error('Timed out waiting for device export'))
      }, 60000)
    })

    promise.catch(() => {})
    try { await sendLine('EXPORT_BILLS') } catch (error) { finishExport(error) }

    const lines = await promise
    return parseBillsExportLines(lines)
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

  const parseBillsExportLines = lines => parseSyncChunks(lines, 'BEGIN_BILLS_JSON', 'BILLS_CHUNK|')

  const syncBillsFromDevice = async () => {
    const deviceBills = await exportBillsFromDevice()

    if (deviceBills.length === 0) {
      return
    }

    console.log('Bills exported from device:', deviceBills)

    const processed = await databaseService.upsertBillsToDatabase(deviceBills)
    console.log('Bills synced to database, processed:', processed)

    if (Number(processed) !== deviceBills.length) {
      throw new Error('Some bills could not be imported; device records were left unsynced')
    }

    await sendLine('BILLS_SYNCED')
    return processed
  }

  const handleDeviceLine = (line) => {
    if (!exportInProgress.value) return

    if (line.startsWith(exportBeginMarker.value)) {
      exportLines.value = [line]
      return
    }

    if (line.startsWith('ERR|')) { finishExport(new Error(line)); return }
    if (line === exportEndMarker.value) {
      finishExport(null)
      return
    }

    exportLines.value.push(line)
  }

  return {
    exportBillsFromDevice,
    syncBillsFromDevice,
    handleDeviceLine
  }
}
