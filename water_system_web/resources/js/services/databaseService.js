// Database service for handling API calls to the backend
export const databaseService = {
  async fetchCustomersFromDatabase(options = {}) {
    const url = new URL('/customers', window.location.origin)
    if (options.updated_after) url.searchParams.set('updated_after', options.updated_after)
    if (options.brgy_id) url.searchParams.set('brgy_id', options.brgy_id)
    const res = await fetch(url, {
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
  },

  async upsertBillsToDatabase(bills) {
    try {
      const res = await window.axios.post('/bills/sync', { bills })
      return Number(res.data?.processed || 0)
    } catch (error) {
      if (error.response?.status === 401) throw new Error('Unauthorized (login required)')
      throw new Error('Bills sync failed: ' + (error.response?.data?.message || error.message))
    }
  },

  async syncDeviceInfoToDatabase(deviceInfo) {
    try {
      const res = await window.axios.post('/devices/sync', { device_info: deviceInfo })
      return res.data
    } catch (error) {
      if (error.response?.status === 401) throw new Error('Unauthorized (login required)')
      throw new Error('Device info sync failed: ' + (error.response?.data?.message || error.message))
    }
  },

  async markCustomersSynced(accountNumbers) {
    try {
      const res = await window.axios.post('/customers/mark-synced', { account_numbers: accountNumbers })
      return Number(res.data?.updated || 0)
    } catch (error) {
      if (error.response?.status === 401) throw new Error('Unauthorized (login required)')
      throw new Error('Failed to mark customers as synced: ' + (error.response?.data?.message || error.message))
    }
  }
}