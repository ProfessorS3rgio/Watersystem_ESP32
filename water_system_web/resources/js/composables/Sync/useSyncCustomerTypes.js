import { serialService } from '../../services/serialService'

export function useSyncCustomerTypes() {
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

  const pushCustomerTypesToDevice = async (dbCustomerTypes) => {
    // Send each upsert line-by-line (adds new customer types, updates existing ones)
    for (let i = 0; i < dbCustomerTypes.length; i++) {
      const ct = dbCustomerTypes[i]
      const createdAt = ct.created_at ? Math.floor(new Date(ct.created_at).getTime() / 1000) : 0
      const updatedAt = ct.updated_at ? Math.floor(new Date(ct.updated_at).getTime() / 1000) : 0
      const line = [
        'UPSERT_CUSTOMER_TYPE',
        String(Number(ct.id || ct.type_id || 1)),
        escapeDeviceField(ct.type_name || ct.name || ''),
        String(Number(ct.rate_per_m3 || 0)),
        String(Number(ct.min_m3 || 0)),
        String(Number(ct.min_charge || 0)),
        String(Number(ct.penalty || 0)),
        String(createdAt),
        String(updatedAt)
      ].join('|')
      await sendLine(line)
      // tiny pacing helps avoid overwhelming the serial buffer
      await new Promise(r => setTimeout(r, 10))
    }
  }

  return {
    pushCustomerTypesToDevice
  }
}