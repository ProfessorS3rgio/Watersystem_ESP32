<template>
  <AppSidebar v-slot="{ isDark }">
    <div class="p-8">
      <div class="mb-8">
        <h1 class="text-3xl font-bold mb-2 transition-colors duration-200" :class="isDark ? 'text-white' : 'text-gray-900'">Billing</h1>
        <p class="transition-colors duration-200" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Process customer payments and manage billing</p>
      </div>

      <!-- Tab Navigation -->
      <div class="mb-6">
        <nav class="flex space-x-1 bg-gray-100 p-1 rounded-lg" :class="isDark ? 'bg-gray-800' : 'bg-gray-100'">
          <button
            @click="activeTab = 'customer'"
            class="flex-1 py-2 px-4 text-sm font-medium rounded-md transition-colors duration-200"
            :class="activeTab === 'customer' ? (isDark ? 'bg-indigo-600 text-white' : 'bg-indigo-600 text-white') : (isDark ? 'text-gray-300 hover:text-white' : 'text-gray-700 hover:text-gray-900')"
          >
            Customer Billing
          </button>
          <button
            @click="activeTab = 'all-bills'"
            class="flex-1 py-2 px-4 text-sm font-medium rounded-md transition-colors duration-200"
            :class="activeTab === 'all-bills' ? (isDark ? 'bg-indigo-600 text-white' : 'bg-indigo-600 text-white') : (isDark ? 'text-gray-300 hover:text-white' : 'text-gray-700 hover:text-gray-900')"
          >
            All Bills
          </button>
        </nav>
      </div>

      <!-- Stats Cards -->
      <BillingStats
        :isDark="isDark"
        :todaysPayments="todaysPayments"
        :totalCollected="totalCollected"
        :pendingBillsCount="pendingBillsCount"
        :overdueBillsCount="overdueBillsCount"
      />

      <!-- Customer Billing Tab -->
      <div v-if="activeTab === 'customer'" class="grid grid-cols-1 lg:grid-cols-3 gap-8">
        <!-- Customer Search & Selection -->
        <CustomerSearch
          :isDark="isDark"
          :selectedCustomer="selectedCustomer"
          @search-customer="handleSearchCustomer"
          @clear-customer="clearCustomer"
        />

        <!-- Bills & Payment Processing -->
        <BillList
          :isDark="isDark"
          :selectedCustomer="selectedCustomer"
          :pendingBills="pendingBills"
          :loadingBills="loadingBills"
          @select-bill="selectBillForPayment"
        />

        <!-- Payment Processing -->
        <div class="lg:col-span-2">
          <PaymentForm
            v-if="selectedBill"
            :isDark="isDark"
            :selectedBill="selectedBill"
            :customerName="selectedCustomer?.customer_name || ''"
            :processingPayment="processingPayment"
            @process-payment="processPayment"
            @cancel-payment="cancelPayment"
          />

          <!-- Recent Payments -->
          <RecentPayments
            :isDark="isDark"
            :selectedCustomer="selectedCustomer"
            :selectedBill="selectedBill"
            :recentPayments="recentPayments"
            :loadingPayments="loadingPayments"
          />
        </div>
      </div>

      <!-- All Bills Tab -->
      <div v-if="activeTab === 'all-bills'">
        <!-- Bills List -->
        <div class="bg-white rounded-lg shadow-sm border" :class="isDark ? 'bg-gray-800 border-gray-700' : 'bg-white border-gray-200'">
          <div class="px-6 py-4 border-b" :class="isDark ? 'border-gray-700' : 'border-gray-200'">
            <div class="flex justify-between items-center">
              <h3 class="text-lg font-semibold">All Bills</h3>
              <div class="flex gap-3">
                <div class="flex items-center gap-2">
                  <label class="text-sm font-medium" :class="isDark ? 'text-gray-200' : 'text-gray-700'">Status:</label>
                  <select
                    v-model="statusFilter"
                    class="px-2 py-1 text-sm border rounded focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500"
                    :class="isDark ? 'bg-gray-700 border-gray-600 text-white' : 'bg-white border-gray-300 text-gray-900'"
                    @change="loadAllBills"
                  >
                    <option value="">All</option>
                    <option value="pending">Pending</option>
                    <option value="paid">Paid</option>
                    <option value="void">Void</option>
                  </select>
                </div>
                <div class="flex items-center gap-2">
                  <label class="text-sm font-medium" :class="isDark ? 'text-gray-200' : 'text-gray-700'">Customer:</label>
                  <input
                    v-model="customerFilter"
                    type="text"
                    placeholder="Name or account"
                    class="px-2 py-1 text-sm border rounded focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500"
                    :class="isDark ? 'bg-gray-700 border-gray-600 text-white' : 'bg-white border-gray-300 text-gray-900'"
                    @keyup.enter="loadAllBills"
                  >
                </div>
                <button
                  @click="loadAllBills"
                  :disabled="loadingAllBills"
                  class="bg-indigo-600 hover:bg-indigo-700 text-white px-3 py-1 rounded text-sm font-medium disabled:opacity-50 transition-colors duration-200"
                >
                  {{ loadingAllBills ? 'Loading...' : 'Filter' }}
                </button>
              </div>
            </div>
          </div>
          <div class="p-6">
            <div v-if="loadingAllBills" class="text-center py-8">
              <div class="animate-spin rounded-full h-8 w-8 border-b-2 border-indigo-600 mx-auto"></div>
              <p class="mt-2 text-sm" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Loading bills...</p>
            </div>

            <div v-else-if="allBills.length === 0" class="text-center py-8">
              <svg class="mx-auto h-12 w-12" :class="isDark ? 'text-gray-400' : 'text-gray-400'" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 12h6m-6 4h6m2 5H7a2 2 0 01-2-2V5a2 2 0 012-2h5.586a1 1 0 01.707.293l5.414 5.414a1 1 0 01.293.707V19a2 2 0 01-2 2z" />
              </svg>
              <h3 class="mt-2 text-sm font-medium" :class="isDark ? 'text-gray-300' : 'text-gray-900'">No bills found</h3>
              <p class="mt-1 text-sm" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Try adjusting your filters.</p>
            </div>

            <div v-else class="overflow-x-auto">
              <table class="w-full text-sm">
                <thead class="border-b" :class="isDark ? 'border-gray-600' : 'border-gray-200'">
                  <tr>
                    <th class="text-left py-2 px-2 font-medium" :class="isDark ? 'text-gray-200' : 'text-gray-700'">Customer</th>
                    <th class="text-left py-2 px-2 font-medium" :class="isDark ? 'text-gray-200' : 'text-gray-700'">Account</th>
                    <th class="text-left py-2 px-2 font-medium" :class="isDark ? 'text-gray-200' : 'text-gray-700'">Due Date</th>
                    <th class="text-left py-2 px-2 font-medium" :class="isDark ? 'text-gray-200' : 'text-gray-700'">Amount</th>
                    <th class="text-left py-2 px-2 font-medium" :class="isDark ? 'text-gray-200' : 'text-gray-700'">Status</th>
                    <th class="text-left py-2 px-2 font-medium" :class="isDark ? 'text-gray-200' : 'text-gray-700'">Actions</th>
                  </tr>
                </thead>
                <tbody class="divide-y" :class="isDark ? 'divide-gray-600' : 'divide-gray-200'">
                  <tr v-for="bill in allBills" :key="bill.id" class="hover:bg-gray-50" :class="isDark ? 'hover:bg-gray-700' : 'hover:bg-gray-50'">
                    <td class="py-2 px-2">
                      <span :class="isDark ? 'text-gray-300' : 'text-gray-900'">{{ bill.customer?.customer_name || 'N/A' }}</span>
                    </td>
                    <td class="py-2 px-2">
                      <span :class="isDark ? 'text-gray-400' : 'text-gray-600'">{{ bill.customer?.account_no || 'N/A' }}</span>
                    </td>
                    <td class="py-2 px-2">
                      <span :class="isDark ? 'text-gray-400' : 'text-gray-600'">{{ formatDate(bill.due_date) }}</span>
                    </td>
                    <td class="py-2 px-2">
                      <span class="font-semibold text-green-600">₱{{ formatMoney(bill.total_due) }}</span>
                    </td>
                    <td class="py-2 px-2">
                      <span class="px-2 py-1 rounded-full text-xs font-medium"
                            :class="getStatusClass(bill.status)">
                        {{ bill.status.charAt(0).toUpperCase() + bill.status.slice(1) }}
                      </span>
                    </td>
                    <td class="py-2 px-2">
                      <div class="flex gap-1">
                        <button
                          v-if="bill.status === 'pending'"
                          @click="payBill(bill)"
                          class="bg-green-600 hover:bg-green-700 text-white px-2 py-1 rounded text-xs font-medium transition-colors duration-200"
                        >
                          Pay
                        </button>
                        <button
                          v-if="bill.status === 'paid'"
                          @click="voidBill(bill)"
                          class="bg-red-600 hover:bg-red-700 text-white px-2 py-1 rounded text-xs font-medium transition-colors duration-200"
                        >
                          Void
                        </button>
                        <button
                          v-if="bill.status === 'void'"
                          disabled
                          class="bg-gray-400 text-white px-2 py-1 rounded text-xs font-medium cursor-not-allowed"
                        >
                          Voided
                        </button>
                      </div>
                    </td>
                  </tr>
                </tbody>
              </table>
            </div>

            <!-- Pagination -->
            <div v-if="pagination && pagination.last_page > 1" class="mt-6 flex justify-center">
              <nav class="flex items-center space-x-2">
                <button
                  @click="goToPage(pagination.current_page - 1)"
                  :disabled="pagination.current_page === 1"
                  class="px-3 py-2 rounded-lg border transition-colors duration-200 disabled:opacity-50 disabled:cursor-not-allowed"
                  :class="isDark ? 'border-gray-600 text-gray-300 hover:bg-gray-700' : 'border-gray-300 text-gray-700 hover:bg-gray-100'"
                >
                  Previous
                </button>

                <span class="px-3 py-2" :class="isDark ? 'text-gray-400' : 'text-gray-600'">
                  Page {{ pagination.current_page }} of {{ pagination.last_page }}
                </span>

                <button
                  @click="goToPage(pagination.current_page + 1)"
                  :disabled="pagination.current_page === pagination.last_page"
                  class="px-3 py-2 rounded-lg border transition-colors duration-200 disabled:opacity-50 disabled:cursor-not-allowed"
                  :class="isDark ? 'border-gray-600 text-gray-300 hover:bg-gray-700' : 'border-gray-300 text-gray-700 hover:bg-gray-100'"
                >
                  Next
                </button>
              </nav>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- Payment Success Modal -->
    <SuccessModal
      :isOpen="showPaymentSuccess"
      title="Payment Successful!"
      :message="`Payment of ₱${formatMoney(paymentReceipt?.cash_received || 0)} has been processed successfully for ${selectedCustomer?.customer_name || 'customer'}.<br><br>Bill: ${paymentReceipt?.bill_no}<br>Change: ₱${formatMoney(paymentReceipt?.change || 0)}`"
      buttonText="Continue"
      :isDark="isDark"
      @close="closePaymentSuccess"
    />

    <!-- Payment Modal for All Bills -->
    <div v-if="showPaymentModal" class="fixed inset-0 z-50">
      <div class="absolute inset-0 bg-black/50" @click="closePaymentModal"></div>
      <div class="absolute inset-0 flex items-center justify-center p-4">
        <div class="w-full max-w-md rounded-lg shadow-lg" :class="isDark ? 'bg-gray-800 text-white' : 'bg-white text-gray-900'">
          <div class="px-6 py-4 border-b" :class="isDark ? 'border-gray-700' : 'border-gray-200'">
            <div class="flex items-center justify-between">
              <h3 class="text-lg font-semibold">Pay Bill</h3>
              <button class="text-sm" :class="isDark ? 'text-gray-300 hover:text-white' : 'text-gray-600 hover:text-gray-900'" @click="closePaymentModal">
                ✕
              </button>
            </div>
          </div>
          <div class="p-6">
            <div class="mb-4 p-4 rounded-lg border" :class="isDark ? 'bg-gray-700 border-gray-600' : 'bg-gray-50 border-gray-200'">
              <h4 class="font-semibold mb-2">Bill Details</h4>
              <div class="space-y-1 text-sm">
                <p><span class="font-medium">Bill:</span> {{ selectedBill?.bill_no }}</p>
                <p><span class="font-medium">Customer:</span> {{ selectedBill?.customer?.customer_name }}</p>
                <p><span class="font-medium">Amount Due:</span> ₱{{ formatMoney(selectedBill?.total_due) }}</p>
              </div>
            </div>

            <form @submit.prevent="processPayment" class="space-y-4">
              <div>
                <label class="block text-sm font-medium mb-2" :class="isDark ? 'text-gray-200' : 'text-gray-700'">
                  Cash Received *
                </label>
                <input
                  v-model.number="cashReceived"
                  type="number"
                  step="0.01"
                  min="0"
                  placeholder="0.00"
                  class="w-full px-3 py-2 border rounded-lg focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500"
                  :class="isDark ? 'bg-gray-700 border-gray-600 text-white' : 'bg-white border-gray-300 text-gray-900'"
                  required
                >
              </div>

              <div v-if="cashReceived > 0" class="p-4 rounded-lg" :class="changeAmount >= 0 ? (isDark ? 'bg-green-900/20 border border-green-700' : 'bg-green-50 border border-green-200') : (isDark ? 'bg-red-900/20 border border-red-700' : 'bg-red-50 border border-red-200')">
                <div class="flex justify-between items-center">
                  <span class="font-medium">Change:</span>
                  <span class="text-lg font-bold" :class="changeAmount >= 0 ? 'text-green-600' : 'text-red-600'">
                    ₱{{ formatMoney(Math.abs(changeAmount)) }}
                  </span>
                </div>
                <p v-if="changeAmount < 0" class="text-sm text-red-600 mt-1">
                  Insufficient payment amount
                </p>
              </div>

              <div class="flex gap-3">
                <button
                  type="button"
                  @click="closePaymentModal"
                  class="flex-1 px-4 py-2 rounded-lg font-medium transition-colors duration-200"
                  :class="isDark ? 'bg-gray-700 hover:bg-gray-600 text-white' : 'bg-gray-200 hover:bg-gray-300 text-gray-900'"
                >
                  Cancel
                </button>
                <button
                  type="submit"
                  :disabled="processingPayment || cashReceived <= 0 || changeAmount < 0"
                  class="flex-1 bg-green-600 hover:bg-green-700 text-white px-4 py-2 rounded-lg font-medium disabled:opacity-50 transition-colors duration-200"
                >
                  {{ processingPayment ? 'Processing...' : 'Complete Payment' }}
                </button>
              </div>
            </form>
          </div>
        </div>
      </div>
    </div>
  </AppSidebar>

  <!-- Void Bill Dialog -->
  <VoidBillDialog
    :is-open="showVoidDialog"
    message="Are you sure you want to void this bill? This action cannot be undone."
    :is-loading="voidLoading"
    :is-dark="isDark"
    @confirm="confirmVoid"
    @cancel="cancelVoid"
  />
</template>

<script>
import { ref, computed, onMounted } from 'vue'
import AppSidebar from '../components/AppSidebar.vue'
import BillingStats from '../components/billing/BillingStats.vue'
import CustomerSearch from '../components/billing/CustomerSearch.vue'
import BillList from '../components/billing/BillList.vue'
import PaymentForm from '../components/billing/PaymentForm.vue'
import RecentPayments from '../components/billing/RecentPayments.vue'
import SuccessModal from '../components/SuccessModal.vue'
import VoidBillDialog from '../components/VoidBillDialog.vue'

export default {
  name: 'Billing',
  components: {
    AppSidebar,
    BillingStats,
    CustomerSearch,
    BillList,
    PaymentForm,
    RecentPayments,
    SuccessModal,
    VoidBillDialog
  },
  setup() {
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
}
</script>