<template>
  <div class="lg:col-span-2">
    <!-- Pending Bills -->
    <div v-if="selectedCustomer" class="bg-white rounded-lg shadow-sm border mb-6" :class="isDark ? 'bg-gray-800 border-gray-700' : 'bg-white border-gray-200'">
      <div class="px-6 py-4 border-b" :class="isDark ? 'border-gray-700' : 'border-gray-200'">
        <h3 class="text-lg font-semibold">Pending Bills</h3>
      </div>
      <div class="p-6">
        <div v-if="loadingBills" class="text-center py-8">
          <div class="animate-spin rounded-full h-8 w-8 border-b-2 border-indigo-600 mx-auto"></div>
          <p class="mt-2 text-sm" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Loading bills...</p>
        </div>

        <div v-else-if="pendingBills.length === 0" class="text-center py-8">
          <svg class="mx-auto h-12 w-12" :class="isDark ? 'text-gray-400' : 'text-gray-400'" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 12h6m-6 4h6m2 5H7a2 2 0 01-2-2V5a2 2 0 012-2h5.586a1 1 0 01.707.293l5.414 5.414a1 1 0 01.293.707V19a2 2 0 01-2 2z" />
          </svg>
          <h3 class="mt-2 text-sm font-medium" :class="isDark ? 'text-gray-300' : 'text-gray-900'">No pending bills</h3>
          <p class="mt-1 text-sm" :class="isDark ? 'text-gray-400' : 'text-gray-600'">This customer has no outstanding bills.</p>
        </div>

        <div v-else class="space-y-4">
          <div v-for="bill in pendingBills" :key="bill.id" class="border rounded-lg p-4" :class="isDark ? 'border-gray-600 bg-gray-700' : 'border-gray-200 bg-gray-50'">
            <div class="flex justify-between items-start mb-3">
              <div>
                <h4 class="font-semibold">Bill #{{ bill.reference_number }}</h4>
                <p class="text-sm" :class="isDark ? 'text-gray-400' : 'text-gray-600'">
                  Due: {{ formatDate(bill.due_date) }}
                </p>
              </div>
              <div class="text-right">
                <p class="text-lg font-bold text-green-600">₱{{ formatMoney(bill.total_due) }}</p>
                <p v-if="bill.penalty > 0" class="text-sm text-red-600">Includes ₱{{ formatMoney(bill.penalty) }} penalty</p>
              </div>
            </div>

            <div class="flex items-center justify-between">
              <div class="text-sm">
                <span class="px-2 py-1 rounded-full text-xs font-medium"
                      :class="isBillDue(bill) ? 'bg-red-100 text-red-800' : 'bg-yellow-100 text-yellow-800'">
                  {{ isBillDue(bill) ? 'Overdue' : 'Pending' }}
                </span>
              </div>
              <button
                @click="$emit('select-bill', bill)"
                class="bg-indigo-600 hover:bg-indigo-700 text-white px-4 py-2 rounded-lg font-medium text-sm transition-colors duration-200"
              >
                Process Payment
              </button>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
export default {
  name: 'BillList',
  props: {
    isDark: {
      type: Boolean,
      required: true
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
    }
  },
  emits: ['select-bill'],
  setup() {
    const formatDate = (dateString) => {
      if (!dateString) return 'N/A'
      const date = new Date(dateString)
      return date.toLocaleDateString()
    }

    const formatMoney = (amount) => {
      return Number(amount).toFixed(2)
    }

    const isBillDue = (bill) => {
      if (!bill.due_date) return false
      const due = new Date(bill.due_date)
      const today = new Date()
      today.setHours(0, 0, 0, 0)
      due.setHours(0, 0, 0, 0)
      return due < today
    }

    return {
      formatDate,
      formatMoney,
      isBillDue
    }
  }
}
</script>