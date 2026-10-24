// The local Laravel server reaches the hotspot; the browser stays on its own origin.
export function createWifiTransport({ receive, statusChanged, request = fetch }) {
  let connected = false
  let deviceId = null
  let sending = false

  async function post(path, body = {}) {
    const response = await request('/device-link/wifi/' + path, {
      method: 'POST', credentials: 'same-origin',
      headers: { Accept: 'application/json', 'Content-Type': 'application/json', 'X-CSRF-TOKEN': document.querySelector('meta[name="csrf-token"]')?.getAttribute('content') || '' },
      body: JSON.stringify(body),
      signal: AbortSignal.timeout(60000)
    })
    if (!response.ok) {
      const data = await response.json().catch(() => ({}))
      const error = new Error(data.message || 'Wireless device request failed.')
      error.status = response.status
      throw error
    }
    return response
  }

  return {
    get connected() { return connected },
    get label() { return 'Wi-Fi · ESP32 ' + (deviceId ?? '') + ' · 192.168.4.1' },
    async connect(token) {
      const response = await post('connect', { token })
      deviceId = (await response.json()).device_id
      connected = true
      statusChanged()
    },
    async sendLine(command) {
      if (!connected) throw new Error('Pair the Wi-Fi device first.')
      if (sending) throw new Error('A wireless command is already in progress.')
      sending = true
      try {
        const response = await post('command', { command })
        const text = await response.text()
        // Keep ACKs and export markers on the same receive path as USB serial.
        receive(text.endsWith('\n') ? text : text + '\n')
      } catch (error) {
        if (![422, 429].includes(error.status)) { connected = false; statusChanged() }
        throw error
      } finally { sending = false }
    },
    async disconnect() {
      connected = false
      deviceId = null
      statusChanged()
      await post('disconnect').catch(() => {})
    }
  }
}
