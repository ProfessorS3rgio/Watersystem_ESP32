<template>
  <div class="grid grid-cols-1 md:grid-cols-4 gap-6 mb-8">
    <div class="bg-white rounded-lg shadow-sm p-6 transition-colors duration-200" :class="isDark ? 'bg-gray-800' : 'bg-white'">
      <div class="flex items-center">
        <div class="h-12 w-12 bg-blue-100 rounded-lg flex items-center justify-center transition-colors duration-200" :class="isDark ? 'bg-blue-900/20' : 'bg-blue-100'">
          <svg class="h-6 w-6 text-blue-600" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M17 20h5v-2a3 3 0 00-5.356-1.857M17 20H7m10 0v-2c0-.656-.126-1.283-.356-1.857M7 20H2v-2a3 3 0 015.356-1.857M7 20v-2c0-.656.126-1.283.356-1.857m0 0a5.002 5.002 0 019.288 0M15 7a3 3 0 11-6 0 3 3 0 016 0zm6 3a2 2 0 11-4 0 2 2 0 014 0zM7 10a2 2 0 11-4 0 2 2 0 014 0z" />
          </svg>
        </div>
        <div class="ml-4">
          <p class="text-sm transition-colors duration-200" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Today's Payments</p>
          <p class="text-2xl font-bold transition-colors duration-200" :class="isDark ? 'text-white' : 'text-gray-900'">{{ todaysPayments }}</p>
        </div>
      </div>
    </div>

    <div class="bg-white rounded-lg shadow-sm p-6 transition-colors duration-200" :class="isDark ? 'bg-gray-800' : 'bg-white'">
      <div class="flex items-center">
        <div class="h-12 w-12 bg-green-100 rounded-lg flex items-center justify-center transition-colors duration-200" :class="isDark ? 'bg-green-900/20' : 'bg-green-100'">
          <svg class="h-6 w-6 text-green-600" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 8c-1.657 0-3 .895-3 2s1.343 2 3 2 3 .895 3 2-1.343 2-3 2m0-8c1.11 0 2.08.402 2.599 1M12 8V7m0 1v8m0 0v1m0-1c-1.11 0-2.08-.402-2.599-1M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
          </svg>
        </div>
        <div class="ml-4">
          <p class="text-sm transition-colors duration-200" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Total Collected</p>
          <p class="text-2xl font-bold transition-colors duration-200" :class="isDark ? 'text-white' : 'text-gray-900'">₱{{ formatMoney(totalCollected) }}</p>
        </div>
      </div>
    </div>

    <div class="bg-white rounded-lg shadow-sm p-6 transition-colors duration-200" :class="isDark ? 'bg-gray-800' : 'bg-white'">
      <div class="flex items-center">
        <div class="h-12 w-12 bg-yellow-100 rounded-lg flex items-center justify-center transition-colors duration-200" :class="isDark ? 'bg-yellow-900/20' : 'bg-yellow-100'">
          <svg class="h-6 w-6 text-yellow-600" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 12h6m-6 4h6m2 5H7a2 2 0 01-2-2V5a2 2 0 012-2h5.586a1 1 0 01.707.293l5.414 5.414a1 1 0 01.293.707V19a2 2 0 01-2 2z" />
          </svg>
        </div>
        <div class="ml-4">
          <p class="text-sm transition-colors duration-200" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Pending Bills</p>
          <p class="text-2xl font-bold transition-colors duration-200" :class="isDark ? 'text-white' : 'text-gray-900'">{{ pendingBillsCount }}</p>
        </div>
      </div>
    </div>

    <div class="bg-white rounded-lg shadow-sm p-6 transition-colors duration-200" :class="isDark ? 'bg-gray-800' : 'bg-white'">
      <div class="flex items-center">
        <div class="h-12 w-12 bg-red-100 rounded-lg flex items-center justify-center transition-colors duration-200" :class="isDark ? 'bg-red-900/20' : 'bg-red-100'">
          <svg class="h-6 w-6 text-red-600" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 8v4m0 4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
          </svg>
        </div>
        <div class="ml-4">
          <p class="text-sm transition-colors duration-200" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Overdue Bills</p>
          <p class="text-2xl font-bold transition-colors duration-200" :class="isDark ? 'text-white' : 'text-gray-900'">{{ overdueBillsCount }}</p>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
export default {
  name: 'BillingStats',
  props: {
    isDark: {
      type: Boolean,
      required: true
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
    }
  },
  setup() {
    const formatMoney = (amount) => {
      return Number(amount).toFixed(2)
    }

    return {
      formatMoney
    }
  }
}
</script>