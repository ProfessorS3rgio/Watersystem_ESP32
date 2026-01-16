<template>
  <AppSidebar v-slot="{ isDark }">
    <div class="p-8">
      <div class="mb-8">
        <h1 class="text-3xl font-bold mb-2 transition-colors duration-200" :class="isDark ? 'text-white' : 'text-gray-900'">Billing</h1>
        <p class="transition-colors duration-200" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Process customer payments and manage billing</p>
      </div>

      <!-- Billing Summary View -->
      <BillingSummary
        v-if="activeView === 'summary'"
        :isDark="isDark"
        :todaysPayments="todaysPayments"
        :totalCollected="totalCollected"
        :pendingBillsCount="pendingBillsCount"
        :overdueBillsCount="overdueBillsCount"
        :transactions="transactions"
      />

      <!-- Customer Billing View -->
      <CustomerBilling
        v-if="activeView === 'customer'"
        :isDark="isDark"
        :selectedCustomer="selectedCustomer"
        :pendingBills="pendingBills"
        :loadingBills="loadingBills"
        :selectedBill="selectedBill"
        :processingPayment="processingPayment"
        :recentPayments="recentPayments"
        :loadingPayments="loadingPayments"
        @search-customer="handleSearchCustomer"
        @clear-customer="clearCustomer"
        @select-bill="selectBillForPayment"
        @process-payment="processPayment"
        @cancel-payment="cancelPayment"
      />

      <!-- All Bills View -->
      <AllBills
        v-if="activeView === 'all-bills'"
        :isDark="isDark"
        :statusFilter="statusFilter"
        :customerFilter="customerFilter"
        :loadingAllBills="loadingAllBills"
        :allBills="allBills"
        :pagination="pagination"
        @filter-change="handleFilterChange"
        @pay-bill="payBill"
        @void-bill="voidBill"
        @go-to-page="goToPage"
      />
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

    <!-- Void Bill Dialog -->
    <VoidBillDialog
      :is-open="showVoidDialog"
      message="Are you sure you want to void this bill? This action cannot be undone."
      :is-loading="voidLoading"
      :is-dark="isDark"
      @confirm="confirmVoid"
      @cancel="cancelVoid"
    />
  </AppSidebar>
</template>

<script>
import AppSidebar from '../components/AppSidebar.vue'
import BillingSummary from '../components/billing/BillingSummary.vue'
import CustomerBilling from '../components/billing/CustomerBilling.vue'
import AllBills from '../components/billing/AllBills.vue'
import SuccessModal from '../components/SuccessModal.vue'
import VoidBillDialog from '../components/VoidBillDialog.vue'
import { useBilling } from '../composables/billing.js'
import { useRoute, useRouter } from 'vue-router'
import { computed, watch } from 'vue'

export default {
  name: 'Billing',
  components: {
    AppSidebar,
    BillingSummary,
    CustomerBilling,
    AllBills,
    SuccessModal,
    VoidBillDialog
  },
  setup() {
    const route = useRoute()
    const router = useRouter()
    const billingComposable = useBilling()

    // Override activeTab with activeView for dropdown
    const activeView = computed({
      get: () => route.path.split('/')[2] || 'summary',
      set: (value) => router.push(`/billing/${value}`)
    })

    // Redirect /billing to /billing/summary
    if (route.path === '/billing') {
      router.replace('/billing/summary')
    }

    // Destructure loadAllBills
    const { loadAllBills } = billingComposable

    // Watch for activeView changes to load data
    watch(activeView, (newView) => {
      if (newView === 'all-bills') {
        loadAllBills()
      }
    })

    // Load all bills initially if on all-bills view
    if (activeView.value === 'all-bills') {
      loadAllBills()
    }

    return {
      ...billingComposable,
      activeView
    }
  }
}
</script>