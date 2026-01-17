// Database service for handling API calls to the backend
export const databaseService = {
  async fetchCustomersFromDatabase() {
    const res = await fetch('/customers', {
      headers: { 'Accept': 'application/json' },
      credentials: 'same-origin',
    })
    if (!res.ok) {
      if (res.status === 401) throw new Error('Unauthorized (login required)')
      throw new Error('Failed to fetch customers from database')
    }
    const json = await res.json()
    return Array.isArray(json?.data) ? json.data : []
  },

  async fetchSettingsFromDatabase() {
    const res = await fetch('/settings', {
      headers: { 'Accept': 'application/json' },
      credentials: 'same-origin',
    })
    if (!res.ok) {
      if (res.status === 401) throw new Error('Unauthorized (login required)')
      throw new Error('Failed to fetch settings from database')
    }
    const json = await res.json()
    return json?.data || {}
  },

  async upsertCustomersToDatabase(customers) {
    const csrf = document.querySelector('meta[name="csrf-token"]')?.getAttribute('content')
    const res = await fetch('/customers/sync', {
      method: 'POST',
      headers: {
        'Accept': 'application/json',
        'Content-Type': 'application/json',
        ...(csrf ? { 'X-CSRF-TOKEN': csrf } : {}),
      },
      credentials: 'same-origin',
      body: JSON.stringify({ customers }),
    })

    if (!res.ok) {
      if (res.status === 401) throw new Error('Unauthorized (login required)')
      const text = await res.text()
      throw new Error('Database sync failed: ' + text)
    }
    const json = await res.json()
    return Number(json?.processed || 0)
  },

  async upsertReadingsToDatabase(readings) {
    const csrf = document.querySelector('meta[name="csrf-token"]')?.getAttribute('content')
    const res = await fetch('/readings/sync', {
      method: 'POST',
      headers: {
        'Accept': 'application/json',
        'Content-Type': 'application/json',
        ...(csrf ? { 'X-CSRF-TOKEN': csrf } : {}),
      },
      credentials: 'same-origin',
      body: JSON.stringify({ readings }),
    })

    if (!res.ok) {
      if (res.status === 401) throw new Error('Unauthorized (login required)')
      const text = await res.text()
      throw new Error('Readings sync failed: ' + text)
    }

    const json = await res.json()
    return Number(json?.processed || 0)
  }
}