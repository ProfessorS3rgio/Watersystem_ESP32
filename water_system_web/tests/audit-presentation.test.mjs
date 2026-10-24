import assert from 'node:assert/strict'
import test from 'node:test'
import { auditChanges, auditDate, auditValue, auditType } from '../resources/js/utils/auditPresentation.js'

test('audit changes distinguish zero, false, creation, and deletion', () => {
  assert.deepEqual(auditChanges({ before: { amount: 0, enabled: false }, after: { amount: 10, enabled: true } }).map(row => row.key), ['amount', 'enabled'])
  assert.equal(auditChanges({ before: null, after: { status: 'active' } })[0].before, undefined)
  assert.equal(auditChanges({ before: { account_no: 'M-001' }, after: null })[0].after, undefined)
  assert.equal(auditValue(false), 'No')
  assert.equal(auditValue(0), '0')
})
test('nested snapshots remain inspectable and unchanged fields are omitted', () => {
  assert.equal(auditChanges({ before: { record: { id: 1 } }, after: { record: { id: 1 } } }).length, 0)
  assert.match(auditValue({ account: 'M-001' }), /M-001/)
  assert.equal(auditType('App\\Models\\Customer'), 'Customer')
})
test('timestamps are shown in Philippine time', () => {
  assert.match(auditDate('2026-10-23T16:30:00Z'), /Oct 24, 2026/)
  assert.equal(auditDate('invalid'), '—')
})
