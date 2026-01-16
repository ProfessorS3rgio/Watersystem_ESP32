<template>
  <div v-if="isOpen" class="fixed inset-0 z-50">
    <div class="absolute inset-0 bg-black/50" @click="$emit('close')"></div>
    <div class="absolute inset-0 flex items-center justify-center p-4">
      <div class="w-full max-w-4xl rounded-lg shadow-lg" :class="isDark ? 'bg-gray-900 text-white' : 'bg-white text-gray-900'">
        <div class="px-6 py-4 border-b" :class="isDark ? 'border-gray-800' : 'border-gray-200'">
          <div class="flex items-center justify-between">
            <div>
              <h3 class="text-lg font-semibold">Usage History</h3>
              <p class="text-sm" :class="isDark ? 'text-gray-300' : 'text-gray-600'">
                {{ customer?.customer_name }} ({{ customer?.account_no }})
              </p>
            </div>
            <button class="text-sm" :class="isDark ? 'text-gray-300 hover:text-white' : 'text-gray-600 hover:text-gray-900'" @click="$emit('close')">
              Close
            </button>
          </div>
        </div>

        <div class="px-6 py-4">
          <div v-if="loading" class="text-sm" :class="isDark ? 'text-gray-300' : 'text-gray-600'">Loading readings...</div>
          <div v-else-if="error" class="text-sm text-red-600">{{ error }}</div>
          <div v-else-if="readings.length === 0" class="text-sm" :class="isDark ? 'text-gray-300' : 'text-gray-600'">
            No readings yet for this customer.
          </div>
          <div v-else class="overflow-x-auto">
            <table class="w-full">
              <thead :class="isDark ? 'bg-gray-800' : 'bg-gray-50'">
                <tr>
                  <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-200' : 'text-gray-500'">Date</th>
                  <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-200' : 'text-gray-500'">Previous</th>
                  <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-200' : 'text-gray-500'">Current</th>
                  <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-200' : 'text-gray-500'">Usage (m³)</th>
                  <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-200' : 'text-gray-500'">Bill</th>
                  <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-200' : 'text-gray-500'">Amount</th>
                  <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-200' : 'text-gray-500'">Actions</th>
                </tr>
              </thead>
              <tbody :class="isDark ? 'divide-y divide-gray-800' : 'divide-y divide-gray-200'">
                <tr v-for="reading in readings" :key="reading.id" :class="isDark ? 'hover:bg-gray-800/60' : 'hover:bg-gray-50'">
                  <td class="px-4 py-2 text-sm">{{ formatReadingAt(reading.reading_at) }}</td>
                  <td class="px-4 py-2 text-sm">{{ reading.previous_reading }}</td>
                  <td class="px-4 py-2 text-sm">{{ reading.current_reading }}</td>
                  <td class="px-4 py-2 text-sm font-medium">{{ reading.usage_m3 }}</td>
                  <td class="px-4 py-2 text-sm">
                    <span v-if="!reading.bill_id" class="px-2 py-1 rounded-full text-xs font-medium bg-gray-100 text-gray-700">No Bill</span>
                    <span v-else-if="String(reading.status || '').toLowerCase() === 'paid'" class="px-2 py-1 rounded-full text-xs font-medium bg-green-100 text-green-800">Paid</span>
                    <span v-else-if="String(reading.status || '').toLowerCase() === 'void'" class="px-2 py-1 rounded-full text-xs font-medium bg-gray-100 text-gray-700">Void</span>
                    <span v-else-if="isBillDue(reading)" class="px-2 py-1 rounded-full text-xs font-medium bg-red-100 text-red-800">Due</span>
                    <span v-else class="px-2 py-1 rounded-full text-xs font-medium bg-yellow-100 text-yellow-800">Pending</span>
                  </td>
                  <td class="px-4 py-2 text-sm font-medium text-green-600">
                    <span v-if="reading.bill_id">₱{{ formatMoney(reading.total_due || 0) }}</span>
                    <span v-else class="text-gray-400">-</span>
                  </td>
                  <td class="px-4 py-2 text-sm">
                    <div class="flex gap-2">
                      <button
                        v-if="reading.bill_id && String(reading.status || '').toLowerCase() !== 'paid' && String(reading.status || '').toLowerCase() !== 'void'"
                        @click="$emit('mark-paid', reading)"
                        class="text-indigo-600 hover:text-indigo-900 font-medium text-xs px-2 py-1 rounded"
                      >
                        Mark Paid
                      </button>
                      <span v-else class="text-xs" :class="isDark ? 'text-gray-400' : 'text-gray-500'">-</span>
                    </div>
                  </td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
export default {
  name: 'UsageModal',
  components: {
  },
  props: {
    isOpen: {
      type: Boolean,
      default: false
    },
    customer: {
      type: Object,
      default: null
    },
    readings: {
      type: Array,
      default: () => []
    },
    loading: {
      type: Boolean,
      default: false
    },
    error: {
      type: String,
      default: ''
    },
    isDark: {
      type: Boolean,
      default: false
    }
  },
  emits: ['close', 'mark-paid'],
  data() {
    return {
    }
  },
  methods: {
    formatReadingAt(value) {
      if (!value) return '-'
      const d = new Date(value)
      if (Number.isNaN(d.getTime())) return String(value)
      return d.toLocaleString()
    },
    formatMoney(value) {
      const n = Number(value)
      if (Number.isNaN(n)) return '0.00'
      return n.toFixed(2)
    },
    isBillDue(reading) {
      if (!reading?.due_date) return false
      const due = new Date(reading.due_date)
      if (Number.isNaN(due.getTime())) return false
      const today = new Date()
      today.setHours(0, 0, 0, 0)
      due.setHours(0, 0, 0, 0)
      return due < today
    }
  }
}
</script>