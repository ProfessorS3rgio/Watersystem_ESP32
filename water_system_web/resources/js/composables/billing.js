import { ref, computed, onMounted } from 'vue'

export function useBilling() {
    // Tab management
    const activeTab = ref('customer')

    // Search and customer selection
    const searchAccountNo = ref('')
    const searchName = ref('')
    const searching = ref(false)
    const selectedCustomer = ref(null)

    // Bills and payments
    const pendingBills = ref([])
    const recentPayments = ref([])
    const loadingBills = ref(false)
    const loadingPayments = ref(false)

    // All bills view
    const statusFilter = ref('')
    const customerFilter = ref('')
    const loadingAllBills = ref(false)
    const allBills = ref([])
    const pagination = ref(null)

    // Payment processing
    const selectedBill = ref(null)
    const cashReceived = ref(0)
    const processingPayment = ref(false)

    // Payment modal for all bills
    const showPaymentModal = ref(false)

    // Payment success
    const showPaymentSuccess = ref(false)
    const paymentReceipt = ref(null)

    // Void bill dialog
    const showVoidDialog = ref(false)
    const selectedBillForVoid = ref(null)
    const voidLoading = ref(false)

    // Stats
    const todaysPayments = ref(0)
    const totalCollected = ref(0)
    const pendingBillsCount = ref(0)
    const overdueBillsCount = ref(0)

    const changeAmount = computed(() => {
      if (!selectedBill.value || cashReceived.value <= 0) return 0
      return cashReceived.value - selectedBill.value.total_due
    })

    const handleSearchCustomer = async ({ accountNo, name, searching: searchRef }) => {
      if (!accountNo.trim() && !name.trim()) return

      searchRef.value = true
      try {
        let url = '/customers?'
        if (accountNo.trim()) {
          url += `account_no=${encodeURIComponent(accountNo.trim())}`
        } else if (name.trim()) {
          url += `name=${encodeURIComponent(name.trim())}`
        }

        const response = await fetch(url, {
          headers: { 'Accept': 'application/json' },
          credentials: 'same-origin'
        })

        if (response.ok) {
          const data = await response.json()
          if (data.data && data.data.length > 0) {
            selectedCustomer.value = data.data[0]
            await loadCustomerBills()
            await loadRecentPayments()
          } else {
            alert('Customer not found')
          }
        }
      } catch (error) {
        console.error('Search failed:', error)
        alert('Failed to search customer')
      } finally {
        searchRef.value = false
      }
    }

    const loadCustomerBills = async () => {
      if (!selectedCustomer.value) return

      loadingBills.value = true
      try {
        const response = await fetch(`/customers/${selectedCustomer.value.id}/bills?status=pending`, {
          headers: { 'Accept': 'application/json' },
          credentials: 'same-origin'
        })

        if (response.ok) {
          const data = await response.json()
          pendingBills.value = data.data || []
        }
      } catch (error) {
        console.error('Failed to load bills:', error)
      } finally {
        loadingBills.value = false
      }
    }

    const loadRecentPayments = async () => {
      if (!selectedCustomer.value) return

      loadingPayments.value = true
      try {
        const response = await fetch(`/customers/${selectedCustomer.value.id}/payments?limit=10`, {
          headers: { 'Accept': 'application/json' },
          credentials: 'same-origin'
        })

        if (response.ok) {
          const data = await response.json()
          recentPayments.value = data.data || []
        }
      } catch (error) {
        console.error('Failed to load payments:', error)
      } finally {
        loadingPayments.value = false
      }
    }

    const clearCustomer = () => {
      selectedCustomer.value = null
      pendingBills.value = []
      recentPayments.value = []
      selectedBill.value = null
      cashReceived.value = 0
    }

    const selectBillForPayment = (bill) => {
      selectedBill.value = bill
      cashReceived.value = bill.total_due
    }

    const cancelPayment = () => {
      selectedBill.value = null
      cashReceived.value = 0
    }
    const processPayment = async (payload) => {
      processingPayment.value = true
      try {
        const bill = payload?.bill || selectedBill.value
        const received = payload?.cashReceived ?? cashReceived.value
        const change = payload?.changeAmount ?? changeAmount.value

        if (!bill || !bill.id) {
          alert('No bill selected for payment.')
          processingPayment.value = false
          return
        }

        const csrf = document.querySelector('meta[name="csrf-token"]')?.getAttribute('content')

        const response = await fetch(`/bills/${bill.id}/pay`, {
          method: 'POST',
          headers: {
            'Accept': 'application/json',
            'Content-Type': 'application/json',
            ...(csrf ? { 'X-CSRF-TOKEN': csrf } : {}),
          },
          credentials: 'same-origin',
          body: JSON.stringify({
            amount_paid: bill.total_due,
            cash_received: received,
            change_amount: change,
            payment_method: 'cash'
          })
        })

        if (response.ok) {
          const data = await response.json()
          paymentReceipt.value = {
            ...data.data,
            bill_no: bill.bill_no,
            cash_received: received,
            change: change,
            created_at: new Date().toISOString()
          }
          showPaymentSuccess.value = true

          // Refresh data
          await loadCustomerBills()
          await loadRecentPayments()
          await loadAllBills()

          // Close modal if open and reset payment form
          showPaymentModal.value = false
          selectedBill.value = null
          cashReceived.value = 0
        } else {
          let errorMessage = 'Payment failed. Please try again.'
          try {
            const errorData = await response.json()
            if (errorData.message) {
              errorMessage = errorData.message
            } else if (errorData.errors) {
              errorMessage = Object.values(errorData.errors).flat().join(', ')
            }
          } catch (e) {
            // Ignore JSON parsing errors
          }

          if (response.status === 404) {
            errorMessage = 'Bill not found. Please refresh the page and try again.'
          } else if (response.status === 422) {
            errorMessage = 'Invalid payment data. Please check the amounts and try again.'
          } else if (response.status === 401) {
            errorMessage = 'You need to log in to process payments.'
          }

          alert(errorMessage)
        }
      } catch (error) {
        console.error('Payment failed:', error)
        alert('Payment failed. Please try again.')
      } finally {
        processingPayment.value = false
      }
    }

    const closePaymentSuccess = () => {
      showPaymentSuccess.value = false
      paymentReceipt.value = null
    }

    const closePaymentModal = () => {
      showPaymentModal.value = false
      selectedBill.value = null
      cashReceived.value = 0
    }

    const printReceipt = () => {
      // For now, just show an alert. In a real implementation, this would open a print dialog
      alert('Print functionality would be implemented here with receipt formatting')
    }

    const loadBillingStats = async () => {
      try {
        // Load today's payments count
        const todayResponse = await fetch('/bills/today-payments', {
          headers: { 'Accept': 'application/json' },
          credentials: 'same-origin'
        })
        if (todayResponse.ok) {
          const todayData = await todayResponse.json()
          todaysPayments.value = todayData.count || 0
          totalCollected.value = todayData.total || 0
        }

        // Load pending bills count
        const pendingResponse = await fetch('/bills/stats', {
          headers: { 'Accept': 'application/json' },
          credentials: 'same-origin'
        })
        if (pendingResponse.ok) {
          const statsData = await pendingResponse.json()
          pendingBillsCount.value = statsData.pending || 0
          overdueBillsCount.value = statsData.overdue || 0
        }
      } catch (error) {
        console.error('Failed to load billing stats:', error)
      }
    }

    const loadAllBills = async (page = 1) => {
      loadingAllBills.value = true
      try {
        let url = `/bills?page=${page}`
        const params = []

        if (statusFilter.value) {
          params.push(`status=${encodeURIComponent(statusFilter.value)}`)
        }

        if (customerFilter.value.trim()) {
          params.push(`customer_id=${encodeURIComponent(customerFilter.value.trim())}`)
        }

        if (params.length > 0) {
          url += `&${params.join('&')}`
        }

        const response = await fetch(url, {
          headers: { 'Accept': 'application/json' },
          credentials: 'same-origin'
        })

        if (response.ok) {
          const data = await response.json()
          allBills.value = data.data.data || []
          pagination.value = {
            current_page: data.data.current_page,
            last_page: data.data.last_page,
            per_page: data.data.per_page,
            total: data.data.total
          }
        } else {
          if (response.status === 401) {
            alert('You need to log in to view bills.')
          } else {
            alert('Failed to load bills. Please try again.')
          }
        }
      } catch (error) {
        console.error('Failed to load bills:', error)
        alert('Failed to load bills')
      } finally {
        loadingAllBills.value = false
      }
    }

    const handleFilterChange = ({ statusFilter: newStatusFilter, customerFilter: newCustomerFilter }) => {
      statusFilter.value = newStatusFilter
      customerFilter.value = newCustomerFilter
      loadAllBills()
    }

    const payBill = (bill) => {
      selectedBill.value = bill
      cashReceived.value = bill.total_due
      showPaymentModal.value = true
    }

    const voidBill = (bill) => {
      selectedBillForVoid.value = bill
      showVoidDialog.value = true
    }

    const goToPage = (page) => {
      if (page >= 1 && page <= pagination.value.last_page) {
        loadAllBills(page)
      }
    }

    const getStatusClass = (status) => {
      switch (status) {
        case 'paid':
          return 'bg-green-100 text-green-800'
        case 'pending':
          return 'bg-yellow-100 text-yellow-800'
        case 'void':
          return 'bg-red-100 text-red-800'
        default:
          return 'bg-gray-100 text-gray-800'
      }
    }

    const formatDate = (date) => {
      if (!date) return 'N/A'
      return new Date(date).toLocaleDateString()
    }

    const formatMoney = (amount) => {
      if (amount === null || amount === undefined) return '0.00'
      return parseFloat(amount).toFixed(2)
    }

    const confirmVoid = async (reason) => {
      if (!selectedBillForVoid.value) return

      voidLoading.value = true
      try {
        const csrf = document.querySelector('meta[name="csrf-token"]')?.getAttribute('content')

        const response = await fetch(`/bills/${selectedBillForVoid.value.id}/void`, {
          method: 'POST',
          headers: {
            'Accept': 'application/json',
            'Content-Type': 'application/json',
            ...(csrf ? { 'X-CSRF-TOKEN': csrf } : {}),
          },
          credentials: 'same-origin',
          body: JSON.stringify({
            reason: reason
          })
        })

        if (response.ok) {
          alert('Bill voided successfully')
          // Refresh the bills list
          loadAllBills()
          showVoidDialog.value = false
          selectedBillForVoid.value = null
        } else {
          let errorMessage = 'Failed to void bill. Please try again.'
          try {
            const errorData = await response.json()
            if (errorData.message) {
              errorMessage = errorData.message
            }
          } catch (e) {
            // Ignore JSON parsing errors
          }
          alert(errorMessage)
        }
      } catch (error) {
        console.error('Void bill failed:', error)
        alert('Failed to void bill. Please try again.')
      } finally {
        voidLoading.value = false
      }
    }

    const cancelVoid = () => {
      showVoidDialog.value = false
      selectedBillForVoid.value = null
      voidLoading.value = false
    }

    // Load initial stats
    onMounted(() => {
      loadBillingStats()
    })

    return {
      // Tab management
      activeTab,

      // Search
      searchAccountNo,
      searchName,
      searching,
      handleSearchCustomer,

      // Customer
      selectedCustomer,
      clearCustomer,

      // Bills
      pendingBills,
      loadingBills,
      loadCustomerBills,

      // Payments
      recentPayments,
      loadingPayments,
      loadRecentPayments,

      // Payment processing
      selectedBill,
      cashReceived,
      processingPayment,
      changeAmount,
      selectBillForPayment,
      cancelPayment,
      processPayment,

      // Success modal
      showPaymentSuccess,
      paymentReceipt,
      closePaymentSuccess,
      printReceipt,

      // All bills functionality
      statusFilter,
      customerFilter,
      loadingAllBills,
      allBills,
      pagination,
      loadAllBills,
      handleFilterChange,
      payBill,
      voidBill,
      confirmVoid,
      cancelVoid,
      goToPage,
      getStatusClass,
      formatDate,
      formatMoney,
      showPaymentModal,
      closePaymentModal,
      showVoidDialog,
      selectedBillForVoid,
      voidLoading,

      // Stats
      todaysPayments,
      totalCollected,
      pendingBillsCount,
      overdueBillsCount,
      loadBillingStats
    }
}