import { ref, computed } from 'vue'

export function useCustomers() {
  const customers = ref([])
  const isLoading = ref(false)
  const errorMessage = ref('')
  const search = ref('')
  const customerTypes = ref([])
  const barangays = ref([])
  const deductions = ref([])

  const activeCount = computed(() => customers.value.filter(c => c.status === 'active').length)
  const inactiveCount = computed(() => customers.value.filter(c => c.status === 'disconnected').length)
  const newCount = computed(() => customers.value.filter(c => Number(c.previous_reading) === 0).length)

  const filteredCustomers = computed(() => {
    const q = (search.value || '').trim().toLowerCase()
    if (!q) return customers.value
    return customers.value.filter(c => {
      return (
        String(c.account_no || '').toLowerCase().includes(q) ||
        String(c.customer_name || '').toLowerCase().includes(q) ||
        String(c.address || '').toLowerCase().includes(q)
      )
    })
  })

  const fetchCustomers = async () => {
    isLoading.value = true
    errorMessage.value = ''
    try {
      const res = await fetch('/customers', {
        headers: { 'Accept': 'application/json' },
        credentials: 'same-origin',
      })

      if (!res.ok) {
        if (res.status === 401) {
          errorMessage.value = 'Unauthorized. Please login again.'
          return
        }
        throw new Error('Failed to load customers')
      }

      // Ensure server returned JSON (otherwise it's likely an HTML login/error page)
      const contentType = res.headers.get('content-type') || ''
      if (!contentType.includes('application/json')) {
        const text = await res.text().catch(() => null)
        errorMessage.value = 'Unexpected response from server. Likely unauthenticated or an HTML error page returned.'
        console.error('Non-JSON response for /customers:', res.status, contentType, text)
        return
      }

      let json
      try {
        json = await res.json()
      } catch (e) {
        errorMessage.value = 'Failed to parse server response as JSON'
        console.error('JSON parse error for /customers:', e)
        return
      }

      customers.value = Array.isArray(json?.data) ? json.data : []
    } catch (e) {
      errorMessage.value = e?.message || 'Failed to load customers'
    } finally {
      isLoading.value = false
    }
  }

  const updateCustomer = async (customerId, customerData) => {
    const csrf = document.querySelector('meta[name="csrf-token"]')?.getAttribute('content')

    const res = await fetch(`/customers/${customerId}`, {
      method: 'PUT',
      headers: {
        'Accept': 'application/json',
        'Content-Type': 'application/json',
        ...(csrf ? { 'X-CSRF-TOKEN': csrf } : {}),
      },
      credentials: 'same-origin',
      body: JSON.stringify(customerData)
    })

    if (res.status === 422) {
      const json = await res.json()
      throw new Error('Validation error: ' + Object.values(json.errors).flat().join(', '))
    }

    if (!res.ok) {
      throw new Error('Failed to update customer')
    }

    await fetchCustomers()
  }

  const createCustomer = async (customerData) => {
    const csrf = document.querySelector('meta[name="csrf-token"]')?.getAttribute('content')

    const res = await fetch('/customers', {
      method: 'POST',
      headers: {
        'Accept': 'application/json',
        'Content-Type': 'application/json',
        ...(csrf ? { 'X-CSRF-TOKEN': csrf } : {}),
      },
      credentials: 'same-origin',
      body: JSON.stringify(customerData)
    })

    if (res.status === 422) {
      const json = await res.json()
      throw new Error('Validation error: ' + Object.values(json.errors).flat().join(', '))
    }

    if (!res.ok) {
      throw new Error('Failed to save customer')
    }

    await fetchCustomers()
  }

  const deleteCustomer = async (customerId) => {
    const csrf = document.querySelector('meta[name="csrf-token"]')?.getAttribute('content')

    const res = await fetch(`/customers/${customerId}`, {
      method: 'DELETE',
      headers: {
        'Accept': 'application/json',
        ...(csrf ? { 'X-CSRF-TOKEN': csrf } : {}),
      },
      credentials: 'same-origin',
    })

    if (!res.ok) {
      throw new Error('Failed to delete customer')
    }

    await fetchCustomers()
  }

  const fetchCustomerTypes = async () => {
    try {
      const res = await fetch('/customer-types', {
        headers: { 'Accept': 'application/json' },
        credentials: 'same-origin',
      })
      if (res.ok) {
        const json = await res.json()
        customerTypes.value = Array.isArray(json?.data) ? json.data : []
      }
    } catch (e) {
      console.error('Failed to fetch customer types', e)
    }
  }

  const fetchBarangays = async () => {
    try {
      const res = await fetch('/barangays', {
        headers: { 'Accept': 'application/json' },
        credentials: 'same-origin',
      })
      if (res.ok) {
        const json = await res.json()
        barangays.value = Array.isArray(json?.data) ? json.data : []
      }
    } catch (e) {
      console.error('Failed to fetch barangays', e)
    }
  }

  const fetchDeductions = async () => {
    try {
      const res = await fetch('/deductions', {
        headers: { 'Accept': 'application/json' },
        credentials: 'same-origin',
      })
      if (res.ok) {
        const json = await res.json()
        deductions.value = Array.isArray(json?.data) ? json.data : []
      }
    } catch (e) {
      console.error('Failed to fetch deductions', e)
    }
  }

  return {
    customers,
    isLoading,
    errorMessage,
    search,
    activeCount,
    inactiveCount,
    newCount,
    filteredCustomers,
    customerTypes,
    barangays,
    deductions,
    fetchCustomers,
    fetchCustomerTypes,
    fetchBarangays,
    fetchDeductions,
    updateCustomer,
    createCustomer,
    deleteCustomer
  }
}