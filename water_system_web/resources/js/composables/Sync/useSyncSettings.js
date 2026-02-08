import { ref } from 'vue'
import { serialService } from '../../services/serialService'

export function useSyncSettings() {
  const expectedAck = ref(null)
  const ackResolve = ref(null)
  const ackReject = ref(null)
  const ackTimeoutId = ref(null)

  const sendLine = async (line) => {
    const text = String(line).replace(/\r|\n/g, ' ')
    await serialService.sendLine(text)
  }

  const waitForAck = () => {
    return new Promise((resolve, reject) => {
      ackResolve.value = resolve
      ackReject.value = reject
      // Timeout after 30 seconds
      if (ackTimeoutId.value) {
        clearTimeout(ackTimeoutId.value)
        ackTimeoutId.value = null
      }
      ackTimeoutId.value = setTimeout(() => {
        if (ackResolve.value) {
          ackResolve.value = null
          ackReject.value = null
          expectedAck.value = null
          ackTimeoutId.value = null
          reject(new Error('Timeout waiting for settings ACK'))
        }
      }, 30000)
    })
  }

  const handleAck = (line) => {
    if (line.startsWith('ACK|UPSERT|Settings')) {
      if (ackResolve.value) {
        ackResolve.value(line)
        ackResolve.value = null
        ackReject.value = null
        expectedAck.value = null
        if (ackTimeoutId.value) {
          clearTimeout(ackTimeoutId.value)
          ackTimeoutId.value = null
        }
      }
    } else if (line.startsWith('ERR|')) {
      if (ackReject.value) {
        ackReject.value(new Error('Device error: ' + line))
        ackResolve.value = null
        ackReject.value = null
        expectedAck.value = null
        if (ackTimeoutId.value) {
          clearTimeout(ackTimeoutId.value)
          ackTimeoutId.value = null
        }
      }
    }
  }

  const pushSettingsToDevice = async (dbSettings) => {
    // Send settings to device
    for (let i = 0; i < dbSettings.length; i++) {
      const s = dbSettings[i]
      const line = [
        'UPSERT_SETTINGS',
        String(Number(s.id || 1)),
        String(Number(s.bill_due_days || 5)),
        String(Number(s.disconnection_days || 8))
      ].join('|')
      await sendLine(line)
      // Wait for ACK or ERR
      await waitForAck()
      // tiny pacing helps avoid overwhelming the serial buffer
      await new Promise(r => setTimeout(r, 10))
    }
  }

  return {
    pushSettingsToDevice,
    handleAck
  }
}