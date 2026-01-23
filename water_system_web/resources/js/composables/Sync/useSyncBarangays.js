import { serialService } from '../../services/serialService'

export function useSyncBarangays() {
  const sendLine = async (line) => {
    const text = String(line).replace(/\r|\n/g, '')
    await serialService.sendLine(text)
  }

  const escapeDeviceField = (value) => {
    return String(value ?? '')
      .replace(/\r|\n/g, ' ')
      .replace(/\|/g, ' ')
      .trim()
  }

  const pushBarangaysToDevice = async (dbBarangays) => {
    // Send each upsert line-by-line (adds new barangays, updates existing ones)
    for (let i = 0; i < dbBarangays.length; i++) {
      const b = dbBarangays[i]
      const createdAt = b.created_at ? Math.floor(new Date(b.created_at).getTime() / 1000) : 0
      const updatedAt = b.updated_at ? Math.floor(new Date(b.updated_at).getTime() / 1000) : 0
      const line = [
        'UPSERT_BARANGAY',
        String(Number(b.brgy_id || b.id || 1)),
        escapeDeviceField(b.barangay || ''),
        escapeDeviceField(b.prefix || ''),
        String(Number(b.next_number || 1)),
        String(createdAt),
        String(updatedAt)
      ].join('|')
      await sendLine(line)
      // tiny pacing helps avoid overwhelming the serial buffer
      await new Promise(r => setTimeout(r, 10))
    }
  }

  return {
    pushBarangaysToDevice
  }
}