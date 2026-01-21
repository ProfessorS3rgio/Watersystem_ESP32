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

  async fetchCustomerTypesFromDatabase() {
    const res = await fetch('/customer-types', {
      headers: { 'Accept': 'application/json' },
      credentials: 'same-origin',
    })
    if (!res.ok) {
      if (res.status === 401) throw new Error('Unauthorized (login required)')
      throw new Error('Failed to fetch customer types from database')
    }
    const json = await res.json()
    return Array.isArray(json?.data) ? json.data : []
  },

  async fetchDeductionsFromDatabase() {
    const res = await fetch('/deductions', {
      headers: { 'Accept': 'application/json' },
      credentials: 'same-origin',
    })
    if (!res.ok) {
      if (res.status === 401) throw new Error('Unauthorized (login required)')
      throw new Error('Failed to fetch deductions from database')
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
    try {
      const res = await window.axios.post('/customers/sync', { customers })
      return Number(res.data?.processed || 0)
    } catch (error) {
      if (error.response?.status === 401) throw new Error('Unauthorized (login required)')
      throw new Error('Database sync failed: ' + (error.response?.data?.message || error.message))
    }
  },

  async upsertReadingsToDatabase(readings) {
    try {
      const res = await window.axios.post('/readings/sync', { readings })
      return Number(res.data?.processed || 0)
    } catch (error) {
      if (error.response?.status === 401) throw new Error('Unauthorized (login required)')
      throw new Error('Readings sync failed: ' + (error.response?.data?.message || error.message))
    }
  }
}