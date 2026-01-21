import { ref } from 'vue'

export function useUsage() {
  const usageReadings = ref([])
  const usageLoading = ref(false)
  const usageError = ref('')

  const fetchUsageReadings = async (customerId) => {
    usageLoading.value = true
    usageError.value = ''
    usageReadings.value = []

    try {
      const res = await fetch(`/customers/${customerId}/readings`, {
        headers: { 'Accept': 'application/json' },
        credentials: 'same-origin',
      })

      if (!res.ok) {
        if (res.status === 401) {
          throw new Error('Unauthorized. Please login again.')
        }
        throw new Error('Failed to load readings')
      }

      const json = await res.json()
      usageReadings.value = Array.isArray(json?.data) ? json.data : []
    } catch (e) {
      usageError.value = e?.message || 'Failed to load readings'
    } finally {
      usageLoading.value = false
    }
  }

  const markBillPaid = async (billId) => {
    try {
      await window.axios.post(`/bills/${billId}/mark-paid`)
    } catch (error) {
      if (error.response?.status === 401) throw new Error('Unauthorized. Please login again.')
      throw new Error('Failed to mark paid: ' + (error.response?.data?.message || error.message))
    }
  }

  const voidBill = async (billId, reason = '') => {
    try {
      await window.axios.post(`/bills/${billId}/void`, { reason })
    } catch (error) {
      if (error.response?.status === 401) throw new Error('Unauthorized. Please login again.')
      throw new Error('Failed to void bill: ' + (error.response?.data?.message || error.message))
    }
  }

  const formatReadingAt = (value) => {
    if (!value) return '-'
    const d = new Date(value)
    if (Number.isNaN(d.getTime())) return String(value)
    return d.toLocaleString()
  }

  const formatMoney = (value) => {
    const n = Number(value)
    if (Number.isNaN(n)) return '0.00'
    return n.toFixed(2)
  }

  const isBillDue = (reading) => {
    if (!reading?.due_date) return false
    const due = new Date(reading.due_date)
    if (Number.isNaN(due.getTime())) return false
    const today = new Date()
    today.setHours(0, 0, 0, 0)
    due.setHours(0, 0, 0, 0)
    return due < today
  }

  return {
    usageReadings,
    usageLoading,
    usageError,
    fetchUsageReadings,
    markBillPaid,
    voidBill,
    formatReadingAt,
    formatMoney,
    isBillDue
  }
}