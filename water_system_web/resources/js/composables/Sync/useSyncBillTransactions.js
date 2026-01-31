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
    const text = String(line).replace(/\r|\n/g, '')
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

  const parseBillTransactionsExportLines = (lines) => {
    const billTransactions = []
    let chunkBuffer = ''
    for (const line of lines) {
      if (line.startsWith('BILL_TRANSACTIONS_CHUNK|')) {
        const parts = line.split('|', 3)
        if (parts.length >= 3) {
          chunkBuffer = parts[2] || ''
        }
        continue
      }

      if (chunkBuffer) {
        if (line.startsWith('Heap free') || line === 'END_BILL_TRANSACTIONS_JSON' || line === 'BEGIN_BILL_TRANSACTIONS_JSON') {
          // skip meta
        } else {
          chunkBuffer += String(line).trim()
        }
      }

      if (chunkBuffer) {
        try {
          const chunkTransactions = JSON.parse(chunkBuffer)
          for (const transaction of chunkTransactions) {
            billTransactions.push({
              bill_transaction_id: Number(transaction.bill_transaction_id || 0),
              bill_id: Number(transaction.bill_id || 0),
              bill_reference_number: transaction.bill_reference_number || '',
              type: transaction.type || 'payment',
              source: transaction.source || 'Device',
              amount: Number(transaction.amount || 0),
              cash_received: Number(transaction.cash_received || 0),
              change: Number(transaction.change || 0),
              transaction_date: transaction.transaction_date || '',
              payment_method: transaction.payment_method || 'cash',
              processed_by_device_uid: transaction.processed_by_device_uid || null,
              notes: transaction.notes || null,
              created_at: transaction.created_at || null,
              updated_at: transaction.updated_at || null,
            })
          }
          chunkBuffer = '' // Clear after successful parse
        } catch (e) {
          // Not complete yet, continue accumulating
        }
      }
    }
    return billTransactions
  }

  const syncBillTransactionsFromDevice = async () => {
    const deviceBillTransactions = await exportBillTransactionsFromDevice()

    if (deviceBillTransactions.length === 0) {
      return
    }

    console.log('Bill transactions exported from device:', deviceBillTransactions)

    const processed = await databaseService.upsertBillTransactionsToDatabase(deviceBillTransactions)
    console.log('Bill transactions synced to database, processed:', processed)
    return processed
  }

  const handleDeviceLine = (line) => {
    if (!exportInProgress.value) return

    if (line === exportBeginMarker.value) {
      exportLines.value = []
      return
    }

    if (line === exportEndMarker.value) {
      finishExport(null)
      return
    }

    exportLines.value.push(line)
  }

  return {
    exportBillTransactionsFromDevice,
    syncBillTransactionsFromDevice,
    handleDeviceLine
  }
}