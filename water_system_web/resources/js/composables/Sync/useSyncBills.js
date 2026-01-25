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

    await sendLine('EXPORT_BILLS')

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

  const parseBillsExportLines = (lines) => {
    const bills = []
    let chunkBuffer = ''
    for (const line of lines) {
      if (line.startsWith('BILLS_CHUNK|')) {
        const parts = line.split('|', 3)
        if (parts.length >= 3) {
          chunkBuffer = parts[2] || ''
        }
        continue
      }

      if (chunkBuffer) {
        if (line.startsWith('Heap free') || line === 'END_BILLS_JSON' || line === 'BEGIN_BILLS_JSON') {
          // skip meta
        } else {
          chunkBuffer += String(line).trim()
        }
      }

      if (chunkBuffer) {
        try {
          const chunkBills = JSON.parse(chunkBuffer)
          for (const bill of chunkBills) {
            bills.push({
              bill_id: Number(bill.bill_id || 0),
              reference_number: bill.reference_number || '',
              customer_id: Number(bill.customer_id || 0),
              reading_id: Number(bill.reading_id || 0),
              device_uid: bill.device_uid || '',
              bill_date: bill.bill_date || '',
              rate_per_m3: Number(bill.rate_per_m3 || 0),
              charges: Number(bill.charges || 0),
              penalty: Number(bill.penalty || 0),
              total_due: Number(bill.total_due || 0),
              status: bill.status || 'pending',
            })
          }
          chunkBuffer = '' // Clear after successful parse
        } catch (e) {
          // Not complete yet, continue accumulating
        }
      }
    }
    return bills
  }

  const syncBillsFromDevice = async () => {
    const deviceBills = await exportBillsFromDevice()

    if (deviceBills.length === 0) {
      return
    }

    console.log('Bills exported from device:', deviceBills)

    const processed = await databaseService.upsertBillsToDatabase(deviceBills)
    console.log('Bills synced to database, processed:', processed)
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
    exportBillsFromDevice,
    syncBillsFromDevice,
    handleDeviceLine
  }
}