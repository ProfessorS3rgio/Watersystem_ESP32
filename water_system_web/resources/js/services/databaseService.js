// Database service for handling API calls to the backend
let activeSyncRunId = null

export const databaseService = {
  async previewSyncBatch(payload) {
    return (await window.axios.post('/sync-reviews', payload)).data.review
  },
  async approveSyncBatch(id, deviceUid, acceptWarnings) {
    return (await window.axios.post(`/sync-reviews/${id}/approve`, { device_uid: deviceUid, accept_warnings: acceptWarnings })).data.receipt
  },
  async cancelSyncReview(id) { await window.axios.post(`/sync-reviews/${id}/cancel`) },
  async acknowledgeSyncReview(id) { await window.axios.post(`/sync-reviews/${id}/acknowledged`) },
  async syncReviewHistory() { return (await window.axios.get('/sync-reviews')).data.reviews },
  async startSyncRun() {
    const res = await window.axios.post('/sync-runs')
    activeSyncRunId = res.data?.run_id || null
    if (!activeSyncRunId) throw new Error('Sync run did not return an ID')
    window.axios.defaults.headers.common['X-Sync-Run-ID'] = activeSyncRunId
    return activeSyncRunId
  },

  async finishSyncRun(status = 'completed', details = {}) {
    if (!activeSyncRunId) return
    const runId = activeSyncRunId
    try {
      await window.axios.post(`/sync-runs/${runId}/finish`, {
        status,
        summary: details.summary || null,
        error: details.error || null,
      })
    } finally {
      activeSyncRunId = null
      delete window.axios.defaults.headers.common['X-Sync-Run-ID']
    }
  },

  async createRestorePoint(label = 'Before device sync') {
    try {
      const res = await window.axios.post('/backup/restore-points', { label, source: 'sync' })
      return res.data
    } catch (error) {
      if (error.response?.status === 401) throw new Error('Unauthorized (login required)')
      throw new Error('Restore point failed: ' + (error.response?.data?.message || error.message))
    }
  },

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

  async fetchBarangaysFromDatabase() {
    const res = await fetch('/barangays', {
      headers: { 'Accept': 'application/json' },
      credentials: 'same-origin',
    })
    if (!res.ok) {
      if (res.status === 401) throw new Error('Unauthorized (login required)')
      throw new Error('Failed to fetch barangays from database')
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

  async inspectDeviceReadings(readings) {
    const res = await window.axios.post('/readings/inspect', { readings })
    return res.data
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
  },

  async markReadingsSynced(readingIds) {
    try {
      const res = await window.axios.post('/readings/mark-synced', { reading_ids: readingIds })
      return Number(res.data?.updated || 0)
    } catch (error) {
      if (error.response?.status === 401) throw new Error('Unauthorized (login required)')
      throw new Error('Failed to mark readings as synced: ' + (error.response?.data?.message || error.message))
    }
  },

  async markBillsSynced(billIds) {
    try {
      const res = await window.axios.post('/bills/mark-synced', { bill_ids: billIds })
      return Number(res.data?.updated || 0)
    } catch (error) {
      if (error.response?.status === 401) throw new Error('Unauthorized (login required)')
      throw new Error('Failed to mark bills as synced: ' + (error.response?.data?.message || error.message))
    }
  },

  async markBillTransactionsSynced(billTransactionIds) {
    try {
      const res = await window.axios.post('/bill-transactions/mark-synced', { bill_transaction_ids: billTransactionIds })
      return Number(res.data?.updated || 0)
    } catch (error) {
      if (error.response?.status === 401) throw new Error('Unauthorized (login required)')
      throw new Error('Failed to mark bill transactions as synced: ' + (error.response?.data?.message || error.message))
    }
  },

  async markDeductionsSynced(deductionIds) {
    try {
      const res = await window.axios.post('/deductions/mark-synced', { deduction_ids: deductionIds })
      return Number(res.data?.updated || 0)
    } catch (error) {
      if (error.response?.status === 401) throw new Error('Unauthorized (login required)')
      throw new Error('Failed to mark deductions as synced: ' + (error.response?.data?.message || error.message))
    }
  },

  async markCustomerTypesSynced(customerTypeIds) {
    try {
      const res = await window.axios.post('/customer-types/mark-synced', { customer_type_ids: customerTypeIds })
      return Number(res.data?.updated || 0)
    } catch (error) {
      if (error.response?.status === 401) throw new Error('Unauthorized (login required)')
      throw new Error('Failed to mark customer types as synced: ' + (error.response?.data?.message || error.message))
    }
  },

  async markBarangaysSynced(brgyIds) {
    try {
      const res = await window.axios.post('/barangays/mark-synced', { brgy_ids: brgyIds })
      return Number(res.data?.updated || 0)
    } catch (error) {
      if (error.response?.status === 401) throw new Error('Unauthorized (login required)')
      throw new Error('Failed to mark barangays as synced: ' + (error.response?.data?.message || error.message))
    }
  },

  async markSettingsSynced(settingIds) {
    try {
      const res = await window.axios.post('/settings/mark-synced', { setting_ids: settingIds })
      return Number(res.data?.updated || 0)
    } catch (error) {
      if (error.response?.status === 401) throw new Error('Unauthorized (login required)')
      throw new Error('Failed to mark settings as synced: ' + (error.response?.data?.message || error.message))
    }
  }
}
