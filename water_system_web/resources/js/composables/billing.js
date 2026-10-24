import { ref, computed, onMounted } from 'vue'
import { billBalance } from '../utils/billingPresentation.js'

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
    const barangays = ref([])

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
    const transactions = ref([])
    const loadingStats = ref(false)
    const statsError = ref('')
    let billsRequestId = 0

    const changeAmount = computed(() => {
      if (!selectedBill.value || cashReceived.value <= 0) return 0
      return Math.round((cashReceived.value - billBalance(selectedBill.value)) * 100) / 100
    })

    const handleSearchCustomer = async ({ customer }) => {
      if (!customer) return
      clearCustomer()
      selectedCustomer.value = customer
      await Promise.all([loadCustomerBills(), loadRecentPayments()])
    }

    const loadCustomerBills = async () => {
      if (!selectedCustomer.value) return
      const customerId = selectedCustomer.value.id

      loadingBills.value = true
      try {
        const response = await fetch(`/customers/${customerId}/bills`, {
          headers: { 'Accept': 'application/json' },
          credentials: 'same-origin'
        })

        if (response.ok) {
          const data = await response.json()
          if (selectedCustomer.value?.id !== customerId) return
          pendingBills.value = (data.data || []).filter(bill => ['pending', 'due', 'partially paid'].includes(String(bill.status).toLowerCase()))
        }
      } catch (error) {
        console.error('Failed to load bills:', error)
      } finally {
        if (selectedCustomer.value?.id === customerId) loadingBills.value = false
      }
    }

    const loadRecentPayments = async () => {
      if (!selectedCustomer.value) return
      const customerId = selectedCustomer.value.id

      loadingPayments.value = true
      try {
        const response = await fetch(`/customers/${customerId}/payments?limit=10`, {
          headers: { 'Accept': 'application/json' },
          credentials: 'same-origin'
        })

        if (response.ok) {
          const data = await response.json()
          if (selectedCustomer.value?.id !== customerId) return
          recentPayments.value = data.data || []
        }
      } catch (error) {
        console.error('Failed to load payments:', error)
      } finally {
        if (selectedCustomer.value?.id === customerId) loadingPayments.value = false
      }
    }

    const clearCustomer = () => {
      loadingBills.value = false
      loadingPayments.value = false
      selectedCustomer.value = null
      pendingBills.value = []
      recentPayments.value = []
      selectedBill.value = null
      cashReceived.value = 0
    }

    const selectBillForPayment = (bill) => {
      selectedBill.value = bill
      cashReceived.value = billBalance(bill)
    }

    const cancelPayment = () => {
      if (processingPayment.value) return
      selectedBill.value = null
      cashReceived.value = 0
    }
    const processPayment = async (payload) => {
      if (processingPayment.value) return
      processingPayment.value = true
      try {
        const bill = payload?.bill || selectedBill.value
        const received = payload?.cashReceived ?? cashReceived.value
        const amount = billBalance(bill)
        const change = Math.round((Number(received) - amount) * 100) / 100

        if (!bill || !bill.id) {
          alert('No bill selected for payment.')
          processingPayment.value = false
          return
        }

        if (!Number.isFinite(Number(received)) || amount <= 0 || change < 0) {
          alert('Enter enough cash to cover the remaining balance.')
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
            amount_paid: amount,
            cash_received: received,
            change_amount: change,
            payment_method: 'cash'
          })
        })

        if (response.ok) {
          const data = await response.json()
          paymentReceipt.value = {
            ...data.data,
            reference_number: bill.reference_number,
            amount_paid: amount,
            customer_name: bill.customer?.customer_name || selectedCustomer.value?.customer_name || 'Customer',
            cash_received: received,
            change: change,
            created_at: new Date().toISOString()
          }
          showPaymentSuccess.value = true

          // Refresh data
          await loadCustomerBills()
          await loadRecentPayments()
          await loadAllBills()
          await loadBillingStats()

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
      if (processingPayment.value) return
      showPaymentModal.value = false
      selectedBill.value = null
      cashReceived.value = 0
    }

    const printReceipt = async () => {
      if (!paymentReceipt.value) return

      await printToThermal(paymentReceipt.value, paymentReceipt.value.customer_name)
    }

    const printBill = async (bill) => {
      try {
        // Fetch payment transaction for this bill
        const response = await fetch(`/bills/${bill.id}/payment`, {
          headers: { 'Accept': 'application/json' },
          credentials: 'same-origin'
        })

        if (!response.ok) {
          alert('No payment found for this bill')
          return
        }

        const paymentData = await response.json()

        // Create receipt data from bill and payment
        const receiptData = {
          reference_number: bill.reference_number,
          amount_paid: paymentData.amount ?? bill.total_due,
          cash_received: paymentData.cash_received,
          change: paymentData.change_amount || paymentData.change,
          created_at: paymentData.created_at || paymentData.transaction_date
        }

        // Print to thermal printer
        await printToThermal(receiptData, bill.customer?.customer_name)

      } catch (error) {
        console.error('Error printing bill:', error)
        alert('Failed to print receipt')
      }
    }

    const loadBillingStats = async () => {
      if (loadingStats.value) return
      loadingStats.value = true
      statsError.value = ''
      try {
        const paths = ['/bills/monthly-collectable', '/bills/monthly-payments', '/bills/stats', '/bills/transactions?limit=20']
        const results = await Promise.all(paths.map(async path => {
          const response = await fetch(path, { headers: { Accept: 'application/json' }, credentials: 'same-origin' })
          if (!response.ok) throw new Error('Unable to refresh billing overview.')
          return response.json()
        }))
        todaysPayments.value = Number(results[0].total || 0)
        totalCollected.value = Number(results[1].total || 0)
        pendingBillsCount.value = Number(results[2].pending || 0)
        overdueBillsCount.value = Number(results[2].overdue || 0)
        transactions.value = results[3].data || []
      } catch (error) {
        statsError.value = error.message || 'Unable to refresh billing overview.'
      } finally {
        loadingStats.value = false
      }
    }

    const loadAllBills = async (page = 1) => {
      const requestId = ++billsRequestId
      loadingAllBills.value = true
      try {
        let url = `/bills?page=${page}`
        const params = []

        if (statusFilter.value) {
          params.push(`status=${encodeURIComponent(statusFilter.value)}`)
        }

        if (customerFilter.value.trim()) {
          params.push(`customer_search=${encodeURIComponent(customerFilter.value.trim())}`)
        }

        if (params.length > 0) {
          url += `&${params.join('&')}`
        }

        const response = await fetch(url, {
          headers: { 'Accept': 'application/json' },
          credentials: 'same-origin'
        })

        if (requestId !== billsRequestId) return
        if (response.ok) {
          const data = await response.json()
          if (requestId !== billsRequestId) return
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
        if (requestId !== billsRequestId) return
        console.error('Failed to load bills:', error)
        alert('Failed to load bills')
      } finally {
        if (requestId === billsRequestId) loadingAllBills.value = false
      }
    }

    const loadBarangays = async () => {
      if (barangays.value.length) return

      try {
        const response = await fetch('/barangays', {
          headers: { 'Accept': 'application/json' },
          credentials: 'same-origin'
        })
        if (response.ok) {
          const data = await response.json()
          barangays.value = data.data || []
        }
      } catch (error) {
        console.error('Failed to load barangays:', error)
      }
    }

    const downloadMonthlyReport = ({ month, barangayId }) => {
      const params = new URLSearchParams({ month })
      if (barangayId) params.set('brgy_id', barangayId)
      window.location.assign(`/bills/monthly-report?${params.toString()}`)
    }

    const handleFilterChange = ({ statusFilter: newStatusFilter, customerFilter: newCustomerFilter }) => {
      statusFilter.value = newStatusFilter
      customerFilter.value = newCustomerFilter
      loadAllBills()
    }

    const payBill = (bill) => {
      selectedBill.value = bill
      cashReceived.value = billBalance(bill)
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
          await loadAllBills()
          await loadBillingStats()
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

    const printToThermal = async (receiptData, customerName) => {
      try {
        // Send print request to local Node.js print server
        const response = await fetch('http://localhost:3000/print', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
          },
          body: JSON.stringify({
            reference_number: receiptData.reference_number,
            customer_name: customerName,
            amount_paid: receiptData.amount_paid,
            cash_received: receiptData.cash_received,
            change: receiptData.change,
            created_at: receiptData.created_at
          })
        });

        if (response.ok) {
          const result = await response.json();
          alert(result.message || 'Receipt printed successfully');
        } else {
          const error = await response.json();
          throw new Error(error.error || 'Print failed');
        }
      } catch (error) {
        console.error('Print failed:', error);
        alert('Failed to print receipt. Make sure the local print server is running on port 3000.');
      }
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
      barangays,
      loadBarangays,
      downloadMonthlyReport,
      handleFilterChange,
      payBill,
      voidBill,
      printBill,
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
      transactions,
      loadingStats,
      statsError,
      loadBillingStats
    }
}
