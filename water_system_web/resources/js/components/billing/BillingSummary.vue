<template>
  <!-- Stats Cards -->
  <div class="grid grid-cols-1 md:grid-cols-4 gap-6 mb-8">
    <div class="bg-white rounded-lg shadow p-6" :class="isDark ? 'bg-gray-800' : 'bg-white'">
      <div class="flex items-center">
        <div class="h-12 w-12 bg-blue-100 rounded-lg flex items-center justify-center">
          <svg class="h-6 w-6 text-blue-600" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 8c-1.657 0-3 .895-3 2s1.343 2 3 2 3 .895 3 2-1.343 2-3 2m0-8c1.11 0 2.08.402 2.599 1M12 8V7m0 1v8m0 0v1m0-1c-1.11 0-2.08-.402-2.599-1" />
          </svg>
        </div>
        <div class="ml-4">
          <p class="text-sm font-medium" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Today's Payments</p>
          <p class="text-2xl font-bold" :class="isDark ? 'text-white' : 'text-gray-900'">₱{{ formatMoney(todaysPayments) }}</p>
        </div>
      </div>
    </div>

    <div class="bg-white rounded-lg shadow p-6" :class="isDark ? 'bg-gray-800' : 'bg-white'">
      <div class="flex items-center">
        <div class="h-12 w-12 bg-green-100 rounded-lg flex items-center justify-center">
          <svg class="h-6 w-6 text-green-600" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 8c-1.657 0-3 .895-3 2s1.343 2 3 2 3 .895 3 2-1.343 2-3 2m0-8c1.11 0 2.08.402 2.599 1M12 8V7m0 1v8m0 0v1m0-1c-1.11 0-2.08-.402-2.599-1" />
          </svg>
        </div>
        <div class="ml-4">
          <p class="text-sm font-medium" :class="isDark ? 'text-gray-400' : 'text-gray-600'">This Month Collected</p>
          <p class="text-2xl font-bold" :class="isDark ? 'text-white' : 'text-gray-900'">₱{{ formatMoney(totalCollected) }}</p>
        </div>
      </div>
    </div>

    <div class="bg-white rounded-lg shadow p-6" :class="isDark ? 'bg-gray-800' : 'bg-white'">
      <div class="flex items-center">
        <div class="h-12 w-12 bg-yellow-100 rounded-lg flex items-center justify-center">
          <svg class="h-6 w-6 text-yellow-600" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 8v4l3 3m6-3a9 9 0 11-18 0 9 9 0 0118 0z" />
          </svg>
        </div>
        <div class="ml-4">
          <p class="text-sm font-medium" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Pending Bills</p>
          <p class="text-2xl font-bold" :class="isDark ? 'text-white' : 'text-gray-900'">{{ pendingBillsCount }}</p>
        </div>
      </div>
    </div>

    <div class="bg-white rounded-lg shadow p-6" :class="isDark ? 'bg-gray-800' : 'bg-white'">
      <div class="flex items-center">
        <div class="h-12 w-12 bg-red-100 rounded-lg flex items-center justify-center">
          <svg class="h-6 w-6 text-red-600" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-2.5L13.732 4c-.77-.833-1.964-.833-2.732 0L3.732 16.5c-.77.833.192 2.5 1.732 2.5z" />
          </svg>
        </div>
        <div class="ml-4">
          <p class="text-sm font-medium" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Overdue Bills</p>
          <p class="text-2xl font-bold" :class="isDark ? 'text-white' : 'text-gray-900'">{{ overdueBillsCount }}</p>
        </div>
      </div>
    </div>
  </div>

  <!-- Transactions Table -->
  <div class="mt-8">
    <h3 class="text-lg font-semibold mb-4" :class="isDark ? 'text-white' : 'text-gray-900'">Recent Transactions</h3>
    <div class="overflow-x-auto">
      <table class="min-w-full divide-y" :class="isDark ? 'divide-gray-700' : 'divide-gray-200'">
        <thead :class="isDark ? 'bg-gray-800' : 'bg-gray-50'">
          <tr>
            <th class="px-6 py-3 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-300' : 'text-gray-500'">Date</th>
            <th class="px-6 py-3 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-300' : 'text-gray-500'">Type</th>
            <th class="px-6 py-3 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-300' : 'text-gray-500'">Bill</th>
            <th class="px-6 py-3 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-300' : 'text-gray-500'">Customer</th>
            <th class="px-6 py-3 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-300' : 'text-gray-500'">Amount</th>
            <th class="px-6 py-3 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-300' : 'text-gray-500'">Processed By</th>
          </tr>
        </thead>
        <tbody class="divide-y" :class="isDark ? 'bg-gray-800 divide-gray-700' : 'bg-white divide-gray-200'">
          <tr v-for="transaction in transactions" :key="transaction.id" :class="isDark ? 'hover:bg-gray-700' : 'hover:bg-gray-50'">
            <td class="px-6 py-4 whitespace-nowrap text-sm" :class="isDark ? 'text-gray-300' : 'text-gray-900'">
              {{ formatDate(transaction.transaction_date) }}
            </td>
            <td class="px-6 py-4 whitespace-nowrap">
              <span class="inline-flex px-2 py-1 text-xs font-semibold rounded-full"
                    :class="transaction.type === 'payment' ? (isDark ? 'bg-green-900 text-green-200' : 'bg-green-100 text-green-800') : (isDark ? 'bg-red-900 text-red-200' : 'bg-red-100 text-red-800')">
                {{ transaction.type }}
              </span>
            </td>
            <td class="px-6 py-4 whitespace-nowrap text-sm" :class="isDark ? 'text-gray-300' : 'text-gray-900'">
              {{ transaction.bill?.bill_no }}
            </td>
            <td class="px-6 py-4 whitespace-nowrap text-sm" :class="isDark ? 'text-gray-300' : 'text-gray-900'">
              {{ transaction.bill?.customer?.customer_name }}
            </td>
            <td class="px-6 py-4 whitespace-nowrap text-sm" :class="isDark ? 'text-gray-300' : 'text-gray-900'">
              <span v-if="transaction.type === 'void'" class="text-red-600 font-medium">
                -₱{{ formatMoney(transaction.amount) }}
              </span>
              <span v-else class="text-green-600 font-medium">
                +₱{{ formatMoney(transaction.amount) }}
              </span>
            </td>
            <td class="px-6 py-4 whitespace-nowrap text-sm" :class="isDark ? 'text-gray-300' : 'text-gray-900'">
              {{ transaction.processed_by?.name || 'System' }}
            </td>
          </tr>
        </tbody>
      </table>
    </div>
  </div>
</template>

<script>
export default {
  name: 'BillingSummary',
  props: {
    isDark: {
      type: Boolean,
      default: false
    },
    todaysPayments: {
      type: Number,
      default: 0
    },
    totalCollected: {
      type: Number,
      default: 0
    },
    pendingBillsCount: {
      type: Number,
      default: 0
    },
    overdueBillsCount: {
      type: Number,
      default: 0
    },
    transactions: {
      type: Array,
      default: () => []
    }
  },
  methods: {
    formatMoney(amount) {
      if (amount === null || amount === undefined) return '0.00'
      return parseFloat(amount).toFixed(2)
    },
    formatDate(date) {
      if (!date) return ''
      return new Date(date).toLocaleDateString() + ' ' + new Date(date).toLocaleTimeString([], {hour: '2-digit', minute:'2-digit'})
    }
  }
}
</script>