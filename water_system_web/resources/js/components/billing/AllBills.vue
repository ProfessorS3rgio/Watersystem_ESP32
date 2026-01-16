<template>
  <!-- Bills List -->
  <div class="bg-white rounded-lg shadow-sm border" :class="isDark ? 'bg-gray-800 border-gray-700' : 'bg-white border-gray-200'">
    <div class="px-6 py-4 border-b" :class="isDark ? 'border-gray-700' : 'border-gray-200'">
      <div class="flex justify-between items-center">
        <h3 class="text-lg font-semibold">All Bills</h3>
        <div class="flex gap-3">
          <div class="flex items-center gap-2">
            <label class="text-sm font-medium" :class="isDark ? 'text-gray-200' : 'text-gray-700'">Status:</label>
            <select
              v-model="localStatusFilter"
              class="px-2 py-1 text-sm border rounded focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500"
              :class="isDark ? 'bg-gray-700 border-gray-600 text-white' : 'bg-white border-gray-300 text-gray-900'"
              @change="handleFilterChange"
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
              v-model="localCustomerFilter"
              type="text"
              placeholder="Name or account"
              class="px-2 py-1 text-sm border rounded focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500"
              :class="isDark ? 'bg-gray-700 border-gray-600 text-white' : 'bg-white border-gray-300 text-gray-900'"
              @keyup.enter="handleFilterChange"
            >
          </div>
          <button
            @click="handleFilterChange"
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
                    @click="handlePayBill(bill)"
                    class="bg-green-600 hover:bg-green-700 text-white px-2 py-1 rounded text-xs font-medium transition-colors duration-200"
                  >
                    Pay
                  </button>
                  <button
                    v-if="bill.status === 'paid'"
                    @click="handleVoidBill(bill)"
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
            @click="handleGoToPage(pagination.current_page - 1)"
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
            @click="handleGoToPage(pagination.current_page + 1)"
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
</template>

<script>
export default {
  name: 'AllBills',
  props: {
    isDark: {
      type: Boolean,
      default: false
    },
    statusFilter: {
      type: String,
      default: ''
    },
    customerFilter: {
      type: String,
      default: ''
    },
    loadingAllBills: {
      type: Boolean,
      default: false
    },
    allBills: {
      type: Array,
      default: () => []
    },
    pagination: {
      type: Object,
      default: null
    }
  },
  emits: ['filter-change', 'pay-bill', 'void-bill', 'go-to-page'],
  data() {
    return {
      localStatusFilter: this.statusFilter,
      localCustomerFilter: this.customerFilter
    }
  },
  watch: {
    statusFilter(newVal) {
      this.localStatusFilter = newVal
    },
    customerFilter(newVal) {
      this.localCustomerFilter = newVal
    }
  },
  methods: {
    handleFilterChange() {
      this.$emit('filter-change', {
        statusFilter: this.localStatusFilter,
        customerFilter: this.localCustomerFilter
      })
    },
    handleFilterChange() {
      this.$emit('filter-change', {
        statusFilter: this.statusFilter,
        customerFilter: this.customerFilter
      })
    },
    handlePayBill(bill) {
      this.$emit('pay-bill', bill)
    },
    handleVoidBill(bill) {
      this.$emit('void-bill', bill)
    },
    handleGoToPage(page) {
      this.$emit('go-to-page', page)
    },
    getStatusClass(status) {
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
    },
    formatDate(date) {
      if (!date) return 'N/A'
      return new Date(date).toLocaleDateString()
    },
    formatMoney(amount) {
      if (amount === null || amount === undefined) return '0.00'
      return parseFloat(amount).toFixed(2)
    }
  }
}
</script>