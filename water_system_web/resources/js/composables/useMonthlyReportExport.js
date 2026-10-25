import { ref } from 'vue'
import { invoke } from '@tauri-apps/api/core'

function browserDownload(blob, filename) {
  const url = URL.createObjectURL(blob)
  const link = document.createElement('a')
  link.href = url
  link.download = filename
  document.body.appendChild(link)
  link.click()
  link.remove()
  setTimeout(() => URL.revokeObjectURL(url), 30000)
}

export function useMonthlyReportExport({ request = (...args) => fetch(...args), desktop = Boolean(globalThis.__TAURI_INTERNALS__), save = args => invoke('save_billing_report', args), download = browserDownload } = {}) {
  const exportingReport = ref(false)
  const reportExportStage = ref('')
  const reportExportError = ref('')
  const reportExportResult = ref('')

  async function downloadMonthlyReport({ month, barangayId }) {
    if (exportingReport.value) return
    exportingReport.value = true
    reportExportError.value = ''
    reportExportResult.value = ''
    reportExportStage.value = 'Preparing your Excel report…'
    try {
      const params = new URLSearchParams({ month })
      if (barangayId) params.set('brgy_id', barangayId)
      const response = await request(`/bills/monthly-report?${params}`, {
        credentials: 'same-origin', headers: { Accept: 'application/json' },
      })
      if (!response.ok) {
        const error = await response.json().catch(() => ({}))
        throw new Error(error.message || 'Could not export the report. Please try again.')
      }
      const type = response.headers.get('content-type') || ''
      if (!/spreadsheetml|octet-stream|application\/zip/i.test(type)) {
        throw new Error('The report could not be downloaded. Check that you are still signed in and try again.')
      }
      const disposition = response.headers.get('content-disposition') || ''
      const filename = disposition.match(/filename="([^"]+)"/i)?.[1]
        || disposition.match(/filename=([^;\s]+)/i)?.[1]
        || `monthly-billing-report-${barangayId ? 'Barangay' : 'All-Barangays'}-${month}.xlsx`
      const blob = await response.blob()
      if (desktop) {
        reportExportStage.value = 'Saving your report and opening its folder…'
        const [path, revealed] = await save({ filename, bytes: Array.from(new Uint8Array(await blob.arrayBuffer())) })
        reportExportResult.value = revealed ? `Report saved. File Explorer opened with your file selected: ${path}`
          : `Report saved to ${path}. File Explorer could not open; you can find the file in Downloads.`
      } else {
        await download(blob, filename)
        reportExportResult.value = 'Report downloaded. Check your browser’s downloads for the file location.'
      }
    } catch (error) {
      reportExportError.value = error instanceof Error ? error.message : String(error)
    } finally {
      exportingReport.value = false
      reportExportStage.value = ''
    }
  }

  return { exportingReport, reportExportStage, reportExportError, reportExportResult, downloadMonthlyReport }
}
