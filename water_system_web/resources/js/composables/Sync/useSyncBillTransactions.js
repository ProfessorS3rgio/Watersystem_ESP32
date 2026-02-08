import { ref } from 'vue'
import { serialService } from '../../services/serialService'
import { databaseService } from '../../services/databaseService'

export function useSyncBillTransactions() {
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
      exportTimeoutId.value = setTimeout(() => {
        finishExport(new Error('Timed out waiting for device export'))
      }, 60000)
    })

    await sendLine('EXPORT_BILL_TRANSACTIONS')

    const lines = await promise
    return parseBillTransactionsExportLines(lines)
  }

  const finishExport = (error, lines = null) => {
    if (exportTimeoutId.value) {
      clearTimeout(exportTimeoutId.value)
    }
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

  const parseBillTransactionsExportLines = async (lines) => {
    let expectedChunks = null

    for (const line of lines) {
      const t = String(line || '').trim()
      if (t.startsWith(exportBeginMarker.value + '|')) {
        const parts = t.split('|')
        if (parts.length >= 2) {
          const n = parseInt(parts[1])
          if (Number.isFinite(n)) expectedChunks = n
        }
      }
    }

    const transactions = []
    let chunkBuffer = ''

    for (const line of lines) {
      const t = String(line || '').trim()

      // Start a new chunk buffer when we see the prefix.
      if (t.startsWith(exportLinePrefix.value)) {
        const parts = t.split('|', 3)
        if (parts.length >= 3) {
          chunkBuffer = parts[2] || ''
        }
        continue
      }

      // Accumulate fragments if they happen (should be rare after serialService fix).
      if (chunkBuffer) {
        if (t.startsWith('Heap free') || t.startsWith(exportBeginMarker.value) || t.startsWith(exportEndMarker.value)) {
          // skip meta
        } else {
          chunkBuffer += t
        }
      }

      if (chunkBuffer) {
        try {
          const chunkArr = JSON.parse(chunkBuffer)
          if (Array.isArray(chunkArr)) {
            for (const row of chunkArr) transactions.push(row)
          }
          chunkBuffer = ''
        } catch (_) {
          // Not complete yet
        }
      }
    }

    if (expectedChunks && expectedChunks > 0 && transactions.length === 0) {
      throw new Error('Bill transactions export received, but no valid JSON chunks were parsed')
    }

    if (transactions.length > 0) {
      const processed = await databaseService.syncBillTransactionsToDatabase(transactions)
      return Number(processed || 0)
    }

    return 0
  }

  const handleExportLine = (line) => {
    if (!exportInProgress.value) return

    const t = String(line || '')

    // Device sends BEGIN marker with chunk count, e.g. BEGIN_BILL_TRANSACTIONS_JSON|2
    if (exportBeginMarker.value && t.startsWith(exportBeginMarker.value)) {
      exportLines.value = []
      exportLines.value.push(t)
      return
    }

    if (exportEndMarker.value && t.startsWith(exportEndMarker.value)) {
      const lines = exportLines.value.slice()
      exportLines.value = []
      finishExport(null, lines)
      return
    }

    exportLines.value.push(t)

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