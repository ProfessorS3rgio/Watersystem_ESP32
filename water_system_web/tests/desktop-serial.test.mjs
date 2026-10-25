import assert from 'node:assert/strict';
import test from 'node:test';
import { mockIPC, clearMocks } from '@tauri-apps/api/mocks';
import { createDesktopSerial } from '../resources/js/services/desktopSerial.js';

test('desktop serial preserves split UTF-8 data, sends commands and reports USB removal', async () => {
  globalThis.window = globalThis;
  let channel;
  const writes = [];
  let disconnects = 0;
  let available = [];
  mockIPC((command, args) => {
    if (command === 'serial_ports') return available;
    if (command === 'serial_connect') { channel = args.channel; assert.equal(args.name, 'COM7'); assert.equal(args.baudRate, 115200); return; }
    if (command === 'serial_write') { writes.push(args.line); return; }
    if (command === 'serial_disconnect') { disconnects++; return; }
    throw new Error(`Unexpected command ${command}`);
  });
  globalThis.dispatchEvent = event => { event.detail.resolve('COM7'); return true; };
  const chunks = [];
  let changes = 0;
  const desktop = createDesktopSerial({ receive: chunk => chunks.push(chunk), statusChanged: () => changes++ });
  try {
    await assert.rejects(desktop.connect(), /No serial ports/);
    available = ['COM7'];
    await desktop.connect();
    assert.equal(desktop.connected, true);
    assert.equal(desktop.label, 'COM7 · DMBC USB');
    const encoded = new TextEncoder().encode('ACK|Doña Josefa\n');
    const split = encoded.indexOf(0xc3) + 1;
    channel.onmessage([...encoded.subarray(0, split)]);
    channel.onmessage([...encoded.subarray(split)]);
    assert.equal(chunks.join(''), 'ACK|Doña Josefa\n');
    await desktop.sendLine('EXPORT_BILLS');
    assert.deepEqual(writes, ['EXPORT_BILLS']);
    channel.onmessage([]);
    assert.equal(desktop.connected, false);
    assert.match(chunks.at(-1), /USB removed/);
    assert.equal(disconnects, 1);
    assert.equal(changes, 2);
    await assert.rejects(desktop.sendLine('EXPORT_BILLS'), /disconnected/);
    await desktop.disconnect();
  } finally { clearMocks(); delete globalThis.window; delete globalThis.dispatchEvent; }
});
