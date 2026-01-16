let port = null;
let reader = null;
let writer = null;
let textDecoder = null;
let textEncoder = null;
let isConnected = false;
let isReading = false;
let rxBuffer = '';

const lineListeners = new Set();
const statusListeners = new Set();

function emitStatus() {
  const snapshot = {
    isConnected,
    port,
    connectedPortLabel: getConnectedPortLabel(),
  };
  for (const cb of statusListeners) cb(snapshot);
}

function emitLine(line) {
  for (const cb of lineListeners) cb(line);
}

function getConnectedPortLabel() {
  if (!port) return '';
  try {
    const info = typeof port.getInfo === 'function' ? port.getInfo() : null;
    if (info && (info.usbVendorId || info.usbProductId)) {
      const vid = info.usbVendorId ? info.usbVendorId.toString(16).padStart(4, '0') : '----';
      const pid = info.usbProductId ? info.usbProductId.toString(16).padStart(4, '0') : '----';
      return `USB ${vid}:${pid} (Web Serial)`;
    }
  } catch (_) {
    // ignore
  }
  return 'Serial Port (Web Serial)';
}

function handleChunk(chunk) {
  rxBuffer += chunk;
  const lines = rxBuffer.split(/\r\n|\n|\r/);
  rxBuffer = lines.pop() ?? '';

  for (const line of lines) {
    const trimmed = line.trimEnd();
    if (trimmed.length > 0) emitLine(trimmed);
  }

  // flush if device prints without newlines
  if (rxBuffer.length > 200) {
    const flushed = rxBuffer.trimEnd();
    rxBuffer = '';
    if (flushed.length > 0) emitLine(flushed);
  }
}

async function startReadLoop() {
  if (!reader || isReading) return;
  isReading = true;

  try {
    while (reader && isConnected) {
      const { value, done } = await reader.read();
      if (done) break;
      if (value && value.length) {
        const chunk = textDecoder ? textDecoder.decode(value, { stream: true }) : new TextDecoder().decode(value);
        if (chunk.length > 0) handleChunk(chunk);
      }
    }
  } catch (e) {
    emitLine(`Read stopped: ${e?.message || String(e)}`);
  } finally {
    isReading = false;
  }
}

async function ensureStreams() {
  if (!port) throw new Error('No serial port selected');
  if (!port.readable || !port.writable) {
    throw new Error('Serial port is not readable/writable');
  }

  textDecoder = new TextDecoder();
  textEncoder = new TextEncoder();

  if (!reader) reader = port.readable.getReader();
  if (!writer) writer = port.writable.getWriter();
}

async function connect({ baudRate = 115200 } = {}) {
  if (!('serial' in navigator)) {
    throw new Error('Web Serial API not supported (use Chrome/Edge)');
  }

  if (port && isConnected) {
    emitStatus();
    return;
  }

  port = await navigator.serial.requestPort();
  await port.open({ baudRate });

  isConnected = true;
  rxBuffer = '';
  reader = null;
  writer = null;

  await ensureStreams();
  emitStatus();
  void startReadLoop();
}

async function autoReconnect({ baudRate = 115200 } = {}) {
  if (!('serial' in navigator)) return false;
  if (isConnected && port) return true;

  const ports = await navigator.serial.getPorts();
  if (!ports || ports.length === 0) return false;

  port = ports[0];
  await port.open({ baudRate });

  isConnected = true;
  rxBuffer = '';
  reader = null;
  writer = null;

  await ensureStreams();
  emitStatus();
  void startReadLoop();
  return true;
}

async function disconnect() {
  isConnected = false;
  emitStatus();

  try {
    if (reader) {
      await reader.cancel();
      reader.releaseLock();
    }
  } catch (_) {}

  try {
    if (writer) {
      await writer.close();
      writer.releaseLock();
    }
  } catch (_) {}

  try {
    if (port) {
      await port.close();
    }
  } catch (_) {}

  port = null;
  reader = null;
  writer = null;
  textDecoder = null;
  textEncoder = null;
  rxBuffer = '';
}

async function sendLine(line) {
  if (!writer) throw new Error('Serial writer not available');
  const text = String(line).replace(/\r|\n/g, '');
  const payload = textEncoder ? textEncoder.encode(text + '\n') : new TextEncoder().encode(text + '\n');
  await writer.write(payload);
}

function onLine(cb) {
  lineListeners.add(cb);
  return () => lineListeners.delete(cb);
}

function onStatus(cb) {
  statusListeners.add(cb);
  cb({ isConnected, port, connectedPortLabel: getConnectedPortLabel() });
  return () => statusListeners.delete(cb);
}

function getState() {
  return {
    isConnected,
    port,
    reader,
    writer,
    connectedPortLabel: getConnectedPortLabel(),
  };
}

function installSerialDisconnectListener() {
  if (!('serial' in navigator)) return;

  navigator.serial.addEventListener('disconnect', (event) => {
    const disconnectedPort = event && (event.port || event.target);
    if (port && disconnectedPort === port) {
      emitLine('Device disconnected (USB removed)');
      void disconnect();
    }
  });
}

export const serialService = {
  connect,
  autoReconnect,
  disconnect,
  sendLine,
  onLine,
  onStatus,
  getState,
  installSerialDisconnectListener,
};
