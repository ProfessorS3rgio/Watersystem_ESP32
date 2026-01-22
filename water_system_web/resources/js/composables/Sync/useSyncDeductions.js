import { serialService } from '../../services/serialService'

export function useSyncDeductions() {
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

  const pushDeductionsToDevice = async (dbDeductions) => {
    // Send each upsert line-by-line (adds new deductions, updates existing ones)
    for (let i = 0; i < dbDeductions.length; i++) {
      const d = dbDeductions[i]
      const createdAt = d.created_at ? Math.floor(new Date(d.created_at).getTime() / 1000) : 0
      const updatedAt = d.updated_at ? Math.floor(new Date(d.updated_at).getTime() / 1000) : 0
      const line = [
        'UPSERT_DEDUCTION',
        String(Number(d.id || d.deduction_id || 1)),
        escapeDeviceField(d.name || ''),
        escapeDeviceField(d.type || ''),
        String(Number(d.value || 0)),
        String(createdAt),
        String(updatedAt)
      ].join('|')
      await sendLine(line)
      // tiny pacing helps avoid overwhelming the serial buffer
      await new Promise(r => setTimeout(r, 10))
    }
  }

  return {
    pushDeductionsToDevice
  }
}