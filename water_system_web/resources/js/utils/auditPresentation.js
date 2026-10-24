export const auditLabel = value => String(value || '').replace(/_/g, ' ').replace(/\b\w/g, letter => letter.toUpperCase())
export const auditType = value => String(value || '').split('\\').pop() || 'Record'
export function auditDate(value) {
  const date = new Date(value)
  return value && !Number.isNaN(date.getTime()) ? date.toLocaleString('en-PH', { timeZone: 'Asia/Manila', year: 'numeric', month: 'short', day: '2-digit', hour: '2-digit', minute: '2-digit', second: '2-digit' }) : '—'
}
export function auditValue(value) {
  if (value === undefined || value === null) return '—'
  if (typeof value === 'object') return JSON.stringify(value, null, 2)
  return typeof value === 'boolean' ? (value ? 'Yes' : 'No') : String(value)
}
export function auditChanges(log) {
  const before = log.before || {}, after = log.after || {}
  return [...new Set([...Object.keys(before), ...Object.keys(after)])].filter(key => JSON.stringify(before[key]) !== JSON.stringify(after[key])).map(key => ({ key, before: before[key], after: after[key] }))
}
export function auditSummary(log) {
  const data = log.metadata || {}
  if (log.imported_row_count) return `${log.imported_row_count} imported rows · ${data.source_file || 'Meter readings workbook'}`
  if (data.received !== undefined) return `${data.received} records received${data.sync_run_id ? ' · linked sync run' : ''}`
  if (data.amount !== undefined) return `Payment: ₱${Number(data.amount).toLocaleString('en-PH', { minimumFractionDigits: 2 })}`
  return log.subject_name || log.subject_label || (data.sync_run_id ? 'Linked to a device sync run' : 'Open event to inspect recorded details')
}
