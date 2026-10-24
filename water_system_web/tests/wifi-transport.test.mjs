import assert from 'node:assert/strict'
import test from 'node:test'
import { createWifiTransport } from '../resources/js/services/wifiTransport.js'
import { parseSyncChunks } from '../resources/js/utils/parseSyncChunks.js'

globalThis.document = { querySelector: () => ({ getAttribute: () => 'test-csrf' }) }

test('wireless transport pairs and forwards complete response lines', async () => {
  const calls = [], received = [], statuses = []
  let link
  link = createWifiTransport({
    receive: text => received.push(text), statusChanged: () => statuses.push(link.connected),
    request: async (url, options) => {
      calls.push([url, JSON.parse(options.body)])
      return url.endsWith('/connect') ? Response.json({ device_id: 2 }) : new Response('BEGIN_READINGS_JSON|0\nEND_READINGS_JSON\n')
    }
  })
  await link.connect('ABCD1234')
  await link.sendLine('EXPORT_READINGS')
  assert.equal(link.connected, true)
  assert.match(link.label, /ESP32 2/)
  assert.deepEqual(calls[1], ['/device-link/wifi/command', { command: 'EXPORT_READINGS' }])
  assert.equal(received[0], 'BEGIN_READINGS_JSON|0\nEND_READINGS_JSON\n')
  await link.disconnect()
  assert.deepEqual(statuses, [true, false])
})

test('connection loss clears wireless connection state', async () => {
  const link = createWifiTransport({ receive() {}, statusChanged() {}, request: async url => url.endsWith('/connect') ? Response.json({ device_id: 1 }) : Response.json({ message: 'Connection lost' }, { status: 503 }) })
  await link.connect('ABCD1234')
  await assert.rejects(link.sendLine('EXPORT_BILLS'), /Connection lost/)
  assert.equal(link.connected, false)
})

test('export parsing preserves pipe characters in JSON and validates all chunks', () => {
  const lines = ['BEGIN|2', 'CHUNK|0|[{"notes":"cash | office"}]', 'CHUNK|1|[{"amount":20}]']
  assert.deepEqual(parseSyncChunks(lines, 'BEGIN', 'CHUNK|'), [{ notes: 'cash | office' }, { amount: 20 }])
  assert.throws(() => parseSyncChunks(lines.slice(0, 2), 'BEGIN', 'CHUNK|'), /Incomplete/)
  assert.throws(() => parseSyncChunks([...lines, lines[1]], 'BEGIN', 'CHUNK|'), /repeated/)
  assert.throws(() => parseSyncChunks(['BEGIN|1', 'CHUNK|0|[{'], 'BEGIN', 'CHUNK|'))
})
