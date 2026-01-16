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
    const csrf = document.querySelector('meta[name="csrf-token"]')?.getAttribute('content')

    const res = await fetch(`/bills/${billId}/mark-paid`, {
      method: 'POST',
      headers: {
        'Accept': 'application/json',
        ...(csrf ? { 'X-CSRF-TOKEN': csrf } : {}),
      },
      credentials: 'same-origin',
    })

    if (!res.ok) {
      if (res.status === 401) throw new Error('Unauthorized. Please login again.')
      const text = await res.text()
      throw new Error('Failed to mark paid: ' + text)
    }
  }

  const voidBill = async (billId, reason = '') => {
    const csrf = document.querySelector('meta[name="csrf-token"]')?.getAttribute('content')

    const res = await fetch(`/bills/${billId}/void`, {
      method: 'POST',
      headers: {
        'Accept': 'application/json',
        'Content-Type': 'application/json',
        ...(csrf ? { 'X-CSRF-TOKEN': csrf } : {}),
      },
      credentials: 'same-origin',
      body: JSON.stringify({ reason })
    })

    if (!res.ok) {
      if (res.status === 401) throw new Error('Unauthorized. Please login again.')
      const text = await res.text()
      throw new Error('Failed to void bill: ' + text)
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