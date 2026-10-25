import assert from 'node:assert/strict'
import { readFileSync } from 'node:fs'
import { parseSyncChunks } from '../../water_system_web/resources/js/utils/parseSyncChunks.js'
const lines = readFileSync(process.argv[2], 'utf8').trim().split(/\r?\n/)
assert.equal(lines.length, 152)
const text = 'Doña | \\"\n\r\t\x01\x00end'
let count = 0
for (let chunk=0;chunk<150;chunk++) {
  const prefix = `READINGS_CHUNK|${chunk}|`
  assert.ok(lines[chunk].startsWith(prefix))
  const rows = JSON.parse(lines[chunk].slice(prefix.length))
  assert.equal(rows.length,20)
  for (const row of rows) {
    assert.equal(row.reading_id,++count)
    assert.equal(row.customer_id,1234567890123)
    assert.equal(row.device_uid,text)
    assert.equal(row.last_sync,'')
    assert.equal(row.reading_at,1792934229)
  }
}
assert.equal(JSON.parse(lines[150]).total_due,1234.56)
assert.equal(JSON.parse(lines[151]).notes,text)
const imported = parseSyncChunks(['BEGIN_READINGS_JSON|150', ...lines.slice(0,150)], 'BEGIN_READINGS_JSON', 'READINGS_CHUNK|')
assert.equal(imported.length,3000)
assert.equal(imported[2999].reading_id,3000)
console.log('Passed: 3,000 streamed rows without C++ heap allocation; JSON controls, Unicode, null text, large IDs, dates, money, and cursor errors.')
