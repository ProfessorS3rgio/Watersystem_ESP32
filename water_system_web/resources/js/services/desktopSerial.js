import { invoke, Channel } from '@tauri-apps/api/core';

export const isDesktop = () => Boolean(globalThis.__TAURI_INTERNALS__);
let connected = false;
let label = '';
let channel;

export function createDesktopSerial({ receive, statusChanged }) {
  return {
    get connected() { return connected; },
    get label() { return label; },
    async connect({ baudRate = 115200 } = {}) {
      if (connected) return;
      const ports = await invoke('serial_ports');
      if (!ports.length) throw new Error('No serial ports found. Connect the ESP32 USB cable and check its Windows driver.');
      const name = await new Promise((resolve, reject) => {
        globalThis.dispatchEvent(new CustomEvent('dmbc:choose-port', { detail: { ports, resolve, reject } }));
      });
      const decoder = new TextDecoder();
      channel = new Channel();
      channel.onmessage = bytes => {
        if (!bytes.length) {
          connected = false;
          receive('Device disconnected (USB removed)\n');
          statusChanged();
          void invoke('serial_disconnect').catch(() => {});
          return;
        }
        receive(decoder.decode(new Uint8Array(bytes), { stream: true }));
      };
      await invoke('serial_connect', { name, baudRate, channel });
      connected = true;
      label = `${name} · DMBC USB`;
      statusChanged();
    },
    async disconnect() {
      await invoke('serial_disconnect');
      connected = false; label = ''; channel = null; statusChanged();
    },
    async sendLine(line) {
      if (!connected) throw new Error('USB device is disconnected');
      await invoke('serial_write', { line });
    },
  };
}
