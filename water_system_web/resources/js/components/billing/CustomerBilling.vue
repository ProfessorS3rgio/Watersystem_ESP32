<template>
  <div class="grid grid-cols-1 lg:grid-cols-3 gap-8">
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