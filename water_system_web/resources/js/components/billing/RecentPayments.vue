<template>
  <div v-if="selectedCustomer && !selectedBill" class="bg-white rounded-lg shadow-sm border" :class="isDark ? 'bg-gray-800 border-gray-700' : 'bg-white border-gray-200'">
    <div class="px-6 py-4 border-b" :class="isDark ? 'border-gray-700' : 'border-gray-200'">
      <h3 class="text-lg font-semibold">Recent Payments</h3>
    </div>
    <div class="p-6">
      <div v-if="loadingPayments" class="text-center py-8">
        <div class="animate-spin rounded-full h-8 w-8 border-b-2 border-indigo-600 mx-auto"></div>
        <p class="mt-2 text-sm" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Loading payments...</p>
      </div>

      <div v-else-if="recentPayments.length === 0" class="text-center py-8">
        <svg class="mx-auto h-12 w-12" :class="isDark ? 'text-gray-400' : 'text-gray-400'" fill="none" viewBox="0 0 24 24" stroke="currentColor">
          <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M17 9V7a2 2 0 00-2-2H5a2 2 0 00-2 2v6a2 2 0 002 2h2m2 4h10a2 2 0 002-2v-6a2 2 0 00-2-2H9a2 2 0 00-2 2v6a2 2 0 002 2zm7-5a2 2 0 11-4 0 2 2 0 014 0z" />
        </svg>
        <h3 class="mt-2 text-sm font-medium" :class="isDark ? 'text-gray-300' : 'text-gray-900'">No recent payments</h3>
        <p class="mt-1 text-sm" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Payment history will appear here.</p>
      </div>

      <div v-else class="space-y-3">
        <div v-for="payment in recentPayments" :key="payment.id" class="flex justify-between items-center p-3 border rounded-lg" :class="isDark ? 'border-gray-600 bg-gray-700' : 'border-gray-200 bg-gray-50'">
          <div>
            <p class="font-medium">Bill #{{ payment.bill_no }}</p>
            <p class="text-sm" :class="isDark ? 'text-gray-400' : 'text-gray-600'">{{ formatDate(payment.created_at) }}</p>
          </div>
          <div class="text-right">
            <p class="font-bold text-green-600">₱{{ formatMoney(payment.amount_paid) }}</p>
            <p class="text-sm" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Cash</p>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
export default {
  name: 'RecentPayments',
  props: {
    isDark: {
      type: Boolean,
      required: true
    },
    selectedCustomer: {
      type: Object,
      default: null
    },
    selectedBill: {
      type: Object,
      default: null
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
  setup() {
    const formatDate = (dateString) => {
      if (!dateString) return 'N/A'
      const date = new Date(dateString)
      return date.toLocaleDateString()
    }

    const formatMoney = (amount) => {
      return Number(amount).toFixed(2)
    }

    return {
      formatDate,
      formatMoney
    }
  }
}
</script>