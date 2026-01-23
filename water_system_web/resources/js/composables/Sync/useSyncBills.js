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
    exportLinePrefix.value = '{'

    const promise = new Promise((resolve, reject) => {
      exportResolve.value = resolve
      exportReject.value = reject
      exportTimeoutId.value = setTimeout(() => {
        finishExport(new Error('Timed out waiting for device export'))
      }, 12000)
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
    for (const line of lines) {
      if (!line.startsWith('{')) continue
      try {
        const bill = JSON.parse(line)
        bills.push({
          bill_id: Number(bill.bill_id || 0),
          reference_number: bill.reference_number || '',
          customer_id: Number(bill.customer_id || 0),
          reading_id: Number(bill.reading_id || 0),
          bill_no: bill.bill_no || '',
          bill_date: bill.bill_date || '',
          rate_per_m3: Number(bill.rate_per_m3 || 0),
          charges: Number(bill.charges || 0),
          penalty: Number(bill.penalty || 0),
          total_due: Number(bill.total_due || 0),
          status: bill.status || 'pending',
          created_at: bill.created_at || '',
          updated_at: bill.updated_at || '',
        })
      } catch (e) {
        console.error('Failed to parse bill JSON:', line, e)
      }
    }
    return bills
  }

  const syncBillsFromDevice = async () => {
    const deviceBills = await exportBillsFromDevice()

    if (deviceBills.length === 0) {
      return
    }

    const processed = await databaseService.upsertBillsToDatabase(deviceBills)
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

    if (line.startsWith(exportLinePrefix.value)) {
      exportLines.value.push(line)
    }
  }

  return {
    exportBillsFromDevice,
    syncBillsFromDevice,
    handleDeviceLine
  }
}