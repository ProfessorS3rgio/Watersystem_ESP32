import { serialService } from '../../services/serialService'

export function useSyncSettings() {
  const sendLine = async (line) => {
    const text = String(line).replace(/\r|\n/g, ' ')
    await serialService.sendLine(text)
  }

  const pushSettingsToDevice = async (dbSettings) => {
    // Send settings to device
    for (let i = 0; i < dbSettings.length; i++) {
      const s = dbSettings[i]
      const createdAt = s.created_at ? Math.floor(new Date(s.created_at).getTime() / 1000) : 0
      const updatedAt = s.updated_at ? Math.floor(new Date(s.updated_at).getTime() / 1000) : 0
      const line = [
        'UPSERT_SETTINGS',
        String(Number(s.id || 1)),
        String(Number(s.bill_due_days || 5)),
        String(Number(s.disconnection_days || 8)),
        String(createdAt),
        String(updatedAt)
      ].join('|')
      await sendLine(line)
      // tiny pacing helps avoid overwhelming the serial buffer
      await new Promise(r => setTimeout(r, 10))
    }
  }

  return {
    pushSettingsToDevice
  }
}