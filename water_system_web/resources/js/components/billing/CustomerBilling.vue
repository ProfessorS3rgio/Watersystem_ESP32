<template>
  <div>
    <ol aria-label="Payment steps" class="mb-6 grid gap-2 text-sm sm:grid-cols-3">
      <li v-for="(step, index) in ['Find a member', 'Choose an unpaid bill', 'Record cash payment']" :key="step" class="flex items-center gap-3 rounded-xl px-4 py-3" :class="isDark ? 'bg-slate-900 text-slate-300' : 'bg-teal-50 text-teal-900'"><span class="flex h-6 w-6 items-center justify-center rounded-full bg-teal-700 text-xs font-bold text-white">{{ index + 1 }}</span>{{ step }}</li>
    </ol>
    <div class="grid items-start gap-6 xl:grid-cols-[minmax(260px,.7fr)_minmax(0,1.3fr)]">
      <CustomerSearch :isDark="isDark" :selectedCustomer="selectedCustomer" @search-customer="handleSearchCustomer" @clear-customer="clearCustomer" />
      <div class="min-w-0 space-y-6">
        <div v-if="!selectedCustomer" class="billing-panel flex min-h-80 flex-col items-center justify-center p-8 text-center">
          <div class="mb-5 flex h-16 w-16 items-center justify-center rounded-2xl bg-teal-50 text-teal-700"><svg aria-hidden="true" class="h-8 w-8" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-width="1.5" stroke-linecap="round" d="M16 7a4 4 0 1 1-8 0 4 4 0 0 1 8 0ZM4 21a8 8 0 0 1 16 0"/></svg></div>
          <h2 class="text-xl font-bold">Start with a member account</h2><p class="mt-2 max-w-sm text-sm text-slate-500">Search by account number or name to review bills and record a payment.</p>
        </div>
        <PaymentForm v-if="selectedBill" :key="selectedBill.id" :isDark="isDark" :selectedBill="selectedBill" :customerName="selectedCustomer?.customer_name || ''" :processingPayment="processingPayment" @process-payment="processPayment" @cancel-payment="cancelPayment" />
        <BillList v-else :isDark="isDark" :selectedCustomer="selectedCustomer" :pendingBills="pendingBills" :loadingBills="loadingBills" @select-bill="selectBillForPayment" />
        <RecentPayments :isDark="isDark" :selectedCustomer="selectedCustomer" :selectedBill="selectedBill" :recentPayments="recentPayments" :loadingPayments="loadingPayments" />
      </div>
    </div>
  </div>
</template>
<script>
import CustomerSearch from './CustomerSearch.vue'
import BillList from './BillList.vue'
import PaymentForm from './PaymentForm.vue'
import RecentPayments from './RecentPayments.vue'

export default {
  name: 'CustomerBilling',
  components: {
    CustomerSearch,
    BillList,
    PaymentForm,
    RecentPayments
  },
  props: {
    isDark: {
      type: Boolean,
      default: false
    },
    selectedCustomer: {
      type: Object,
      default: null
    },
    pendingBills: {
      type: Array,
      default: () => []
    },
    loadingBills: {
      type: Boolean,
      default: false
    },
    selectedBill: {
      type: Object,
      default: null
    },
    processingPayment: {
      type: Boolean,
      default: false
    },
    recentPayments: {
      type: Array,
      default: () => []
    },
    loadingPayments: {
      type: Boolean,
      default: false
    }
  },
  emits: ['search-customer', 'clear-customer', 'select-bill', 'process-payment', 'cancel-payment'],
  methods: {
    handleSearchCustomer(data) {
      this.$emit('search-customer', data)
    },
    clearCustomer() {
      this.$emit('clear-customer')
    },
    selectBillForPayment(bill) {
      this.$emit('select-bill', bill)
    },
    processPayment(data) {
      this.$emit('process-payment', data)
    },
    cancelPayment() {
      this.$emit('cancel-payment')
    }
  }
}
</script>
