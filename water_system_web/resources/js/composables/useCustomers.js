import { ref, computed } from 'vue'

export function useCustomers() {
  const customers = ref([])
  const isLoading = ref(false)
  const errorMessage = ref('')
  const search = ref('')

  const activeCount = computed(() => customers.value.filter(c => !!c.is_active).length)
  const inactiveCount = computed(() => customers.value.filter(c => !c.is_active).length)
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

      const json = await res.json()
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
      if (res.status === 401) throw new Error('Unauthorized. Please login again.')
      const text = await res.text()
      throw new Error('Failed to delete customer: ' + text)
    }

    await fetchCustomers()
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
    fetchCustomers,
    updateCustomer,
    createCustomer,
    deleteCustomer
  }
}