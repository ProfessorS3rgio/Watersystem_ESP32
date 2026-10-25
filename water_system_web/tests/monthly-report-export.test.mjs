import test from 'node:test'
import assert from 'node:assert/strict'
import { useMonthlyReportExport } from '../resources/js/composables/useMonthlyReportExport.js'

const workbook = () => new Response(new Uint8Array([80,75,3,4,1,2]), { headers: {
  'content-type': 'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet',
  'content-disposition': 'attachment; filename="monthly-billing-report-All-Barangays-2026-10.xlsx"',
} })
const report = { month: '2026-10', barangayId: '' }

test('desktop export stays busy until the file is saved; repeated clicks cannot duplicate it', async () => {
  let finishRequest, finishSave, calls = 0, saved
  const job = useMonthlyReportExport({ desktop: true,
    request: (url, options) => {
      calls++
      assert.equal(url, '/bills/monthly-report?month=2026-10')
      assert.equal(options.credentials,'same-origin')
      return new Promise(resolve => { finishRequest = resolve })
    },
    save: args => { saved = args; return new Promise(resolve => { finishSave = resolve }) },
  })
  const pending = job.downloadMonthlyReport(report)
  assert.equal(job.exportingReport.value,true)
  await job.downloadMonthlyReport(report)
  assert.equal(calls,1)
  finishRequest(workbook())
  while (!finishSave) await new Promise(resolve => setImmediate(resolve))
  assert.equal(job.exportingReport.value,true)
  assert.deepEqual(saved.bytes,[80,75,3,4,1,2])
  assert.equal(saved.filename,'monthly-billing-report-All-Barangays-2026-10.xlsx')
  finishSave(['C:\\Users\\Test\\Downloads\\report.xlsx',true])
  await pending
  assert.equal(job.exportingReport.value,false)
  assert.match(job.reportExportResult.value,/File Explorer opened/)
})

test('failed generation and expired sessions clear loading without saving a bogus workbook', async () => {
  for (const response of [Response.json({ message: 'Invalid month' }, { status: 422 }), new Response('<html>Login</html>', { headers: { 'content-type': 'text/html' } })]) {
    const job = useMonthlyReportExport({ desktop: true, request: async () => response, save: () => assert.fail('must not save an error response') })
    await job.downloadMonthlyReport(report)
    assert.equal(job.exportingReport.value,false)
    assert.ok(job.reportExportError.value)
    assert.equal(job.reportExportResult.value,'')
  }
})

test('save failure allows retry; Explorer failure still reports the saved file location', async () => {
  let attempt = 0
  const job = useMonthlyReportExport({ desktop: true, request: async () => workbook(), save: async () => {
    if (++attempt === 1) throw new Error('Downloads is not writable')
    return ['C:\\Downloads\\report.xlsx',false]
  } })
  await job.downloadMonthlyReport(report)
  assert.equal(job.reportExportError.value,'Downloads is not writable')
  assert.equal(job.exportingReport.value,false)
  await job.downloadMonthlyReport(report)
  assert.equal(job.reportExportError.value,'')
  assert.match(job.reportExportResult.value,/Report saved to C:/)
  assert.match(job.reportExportResult.value,/File Explorer could not open/)
})

test('browser export retains normal downloads and includes the selected barangay', async () => {
  let filename
  const job = useMonthlyReportExport({ desktop: false,
    request: async url => { assert.match(url,/brgy_id=3/); return workbook() },
    download: async (blob, name) => { assert.equal(blob.size,6); filename = name },
    save: () => assert.fail('browser must not call native commands'),
  })
  await job.downloadMonthlyReport({ ...report, barangayId: '3' })
  assert.ok(filename.endsWith('.xlsx'))
  assert.match(job.reportExportResult.value,/browser/)
  assert.equal(job.exportingReport.value,false)
})
