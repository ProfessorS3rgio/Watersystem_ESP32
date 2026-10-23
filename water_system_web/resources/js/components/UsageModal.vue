<template>
  <div v-if="isOpen" class="fixed inset-0 z-50">
    <div class="absolute inset-0 bg-black/50" @click="$emit('close')"></div>
    <div class="absolute inset-0 flex items-center justify-center p-4">
      <div class="flex max-h-[96vh] w-full max-w-6xl flex-col overflow-hidden rounded-2xl shadow-2xl" :class="isDark ? 'bg-gray-900 text-white' : 'bg-white text-gray-900'">
        <div class="shrink-0 px-6 py-3 border-b" :class="isDark ? 'border-gray-800' : 'border-gray-200'">
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

        <div class="min-h-0 overflow-y-auto px-6 py-3">
          <div class="mb-3 grid grid-cols-1 gap-3 sm:grid-cols-3">
            <div class="rounded-xl p-3" :class="isDark ? 'bg-gray-800' : 'bg-gray-50'">
              <p class="text-xs uppercase tracking-wide" :class="isDark ? 'text-gray-400' : 'text-gray-500'">Account balance</p>
              <p class="mt-1 text-xl font-bold" :class="Number(customer?.account_balance || 0) > 0 ? 'text-red-600' : 'text-green-600'">₱{{ formatMoney(customer?.account_balance || 0) }}</p>
            </div>
            <div class="rounded-xl p-3" :class="isDark ? 'bg-gray-800' : 'bg-gray-50'">
              <p class="text-xs uppercase tracking-wide" :class="isDark ? 'text-gray-400' : 'text-gray-500'">Latest bill</p>
              <p class="mt-1 text-xl font-bold" :class="isDark ? 'text-white' : 'text-gray-900'">₱{{ formatMoney(customer?.latest_bill_total_due || 0) }}</p>
            </div>
            <div class="rounded-xl p-3" :class="isDark ? 'bg-gray-800' : 'bg-gray-50'">
              <p class="text-xs uppercase tracking-wide" :class="isDark ? 'text-gray-400' : 'text-gray-500'">Latest remaining</p>
              <p class="mt-1 text-xl font-bold text-amber-600">₱{{ formatMoney(customer?.latest_bill_balance_due || 0) }}</p>
            </div>
          </div>
          <div v-if="loading" class="text-sm" :class="isDark ? 'text-gray-300' : 'text-gray-600'">Loading readings...</div>
          <div v-else-if="error" class="text-sm text-red-600">{{ error }}</div>
          <div v-else-if="readings.length === 0" class="text-sm" :class="isDark ? 'text-gray-300' : 'text-gray-600'">
            No readings yet for this customer.
          </div>
          <div v-else class="overflow-x-auto rounded-lg border" :class="isDark ? 'border-gray-800' : 'border-gray-200'">
            <table class="w-full">
              <thead class="sticky top-0 z-10" :class="isDark ? 'bg-gray-800' : 'bg-gray-50'">
                <tr>
                  <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-200' : 'text-gray-500'">Date</th>
                  <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-200' : 'text-gray-500'">Previous</th>
                  <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-200' : 'text-gray-500'">Current</th>
                  <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-200' : 'text-gray-500'">Usage (m³)</th>
                  <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-200' : 'text-gray-500'">Bill</th>
                  <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-200' : 'text-gray-500'">Water Bill</th>
                  <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-200' : 'text-gray-500'">Penalty</th>
                  <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-200' : 'text-gray-500'">Bill Total</th>
                  <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-200' : 'text-gray-500'">Balance</th>
                  <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-200' : 'text-gray-500'">Actions</th>
                </tr>
              </thead>
              <tbody :class="isDark ? 'divide-y divide-gray-800' : 'divide-y divide-gray-200'">
                <tr v-for="reading in orderedReadings" :key="reading.id" :class="isDark ? 'hover:bg-gray-800/60' : 'hover:bg-gray-50'">
                  <td class="px-4 py-3 text-sm whitespace-nowrap">{{ formatReadingAt(reading.reading_at) }}</td>
                  <td class="px-4 py-3 text-sm">{{ reading.previous_reading }}</td>
                  <td class="px-4 py-3 text-sm">{{ reading.current_reading }}</td>
                  <td class="px-4 py-3 text-sm font-medium">{{ reading.usage_m3 }}</td>
                  <td class="px-4 py-3 text-sm">
                    <span v-if="!reading.bill_id" class="px-2 py-1 rounded-full text-xs font-medium bg-gray-100 text-gray-700">No Bill</span>
                    <span v-else-if="String(reading.status || '').toLowerCase() === 'paid'" class="px-2 py-1 rounded-full text-xs font-medium bg-green-100 text-green-800">Paid</span>
                    <span v-else-if="String(reading.status || '').toLowerCase() === 'void'" class="px-2 py-1 rounded-full text-xs font-medium bg-gray-100 text-gray-700">Void</span>
                    <span v-else-if="String(reading.status || '').toLowerCase() === 'partially paid'" class="px-2 py-1 rounded-full text-xs font-medium bg-orange-100 text-orange-800">Partially Paid</span>
                    <span v-else-if="isBillDue(reading)" class="px-2 py-1 rounded-full text-xs font-medium bg-red-100 text-red-800">Due</span>
                    <span v-else class="px-2 py-1 rounded-full text-xs font-medium bg-yellow-100 text-yellow-800">Pending</span>
                  </td>
                  <td class="px-4 py-3 text-sm font-medium text-green-600">
                    <span v-if="reading.bill_id">₱{{ formatMoney(reading.charges || 0) }}</span>
                    <span v-else class="text-gray-400">-</span>
                  </td>
                  <td class="px-4 py-3 text-sm font-medium" :class="Number(reading.penalty || 0) > 0 ? 'text-red-600' : 'text-gray-400'">
                    <span v-if="reading.bill_id">₱{{ formatMoney(reading.penalty || 0) }}</span>
                    <span v-else>-</span>
                  </td>
                  <td class="px-4 py-3 text-sm font-medium text-green-600">
                    <span v-if="reading.bill_id">₱{{ formatMoney(reading.total_due || 0) }}</span>
                    <span v-else class="text-gray-400">-</span>
                  </td>
                  <td class="px-4 py-3 text-sm font-semibold text-amber-600">
                    <span v-if="reading.bill_id">₱{{ formatMoney(cumulativeBalance(reading)) }}</span>
                    <span v-else class="text-gray-400">-</span>
                  </td>
                  <td class="px-4 py-3 text-sm">
                    <div class="flex gap-2">
                      <button
                        v-if="reading.bill_id && String(reading.status || '').toLowerCase() !== 'paid' && String(reading.status || '').toLowerCase() !== 'void'"
                        @click="openCashier(reading)"
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

  <div v-if="cashierReading" class="fixed inset-0 z-[60]">
    <div class="absolute inset-0 bg-black/60" @click="closeCashier"></div>
    <div class="absolute inset-0 flex items-center justify-center p-4">
      <form class="w-full max-w-md rounded-xl p-6 shadow-2xl" :class="isDark ? 'bg-gray-900 text-white' : 'bg-white text-gray-900'" @submit.prevent="submitCashier">
        <div class="flex items-start justify-between gap-4">
          <div>
            <p class="text-xs font-semibold uppercase tracking-wider text-indigo-500">Cashier payment</p>
            <h3 class="mt-1 text-xl font-semibold">Record payment</h3>
            <p class="mt-1 text-sm" :class="isDark ? 'text-gray-400' : 'text-gray-600'">{{ cashierReading.customer_name || customer?.customer_name }}</p>
          </div>
          <button type="button" class="text-sm" :class="isDark ? 'text-gray-400 hover:text-white' : 'text-gray-500 hover:text-gray-900'" @click="closeCashier">Cancel</button>
        </div>

        <div class="mt-5 rounded-lg p-4" :class="isDark ? 'bg-gray-800' : 'bg-indigo-50'">
          <p class="text-sm" :class="isDark ? 'text-gray-300' : 'text-indigo-700'">Amount to collect</p>
          <p class="mt-1 text-3xl font-bold" :class="isDark ? 'text-white' : 'text-indigo-900'">₱{{ formatMoney(cashierAmountDue) }}</p>
        </div>

        <label class="mt-5 block text-sm font-medium">Cash received
          <input v-model.number="cashReceived" type="number" min="0" step="0.01" autofocus required class="mt-2 w-full rounded-lg border px-4 py-3 text-lg" :class="isDark ? 'border-gray-700 bg-gray-800 text-white' : 'border-gray-300 bg-white text-gray-900'" placeholder="0.00" />
        </label>
        <div class="mt-4 flex items-center justify-between rounded-lg border px-4 py-3" :class="changeAmount >= 0 ? (isDark ? 'border-green-800 bg-green-900/20' : 'border-green-200 bg-green-50') : (isDark ? 'border-red-800 bg-red-900/20' : 'border-red-200 bg-red-50')">
          <span class="text-sm font-medium">Change</span>
          <span class="text-xl font-bold" :class="changeAmount >= 0 ? 'text-green-600' : 'text-red-600'">₱{{ formatMoney(Math.abs(changeAmount)) }}</span>
        </div>
        <p v-if="changeAmount < 0" class="mt-2 text-sm text-red-600">Cash received must cover the amount to collect.</p>

        <div class="mt-6 flex justify-end gap-3">
          <button type="button" class="rounded-lg px-4 py-2" :class="isDark ? 'bg-gray-800 hover:bg-gray-700' : 'bg-gray-100 hover:bg-gray-200'" @click="closeCashier">Cancel</button>
          <button type="submit" :disabled="cashReceived <= 0 || changeAmount < 0" class="rounded-lg bg-indigo-600 px-5 py-2 font-medium text-white hover:bg-indigo-700 disabled:cursor-not-allowed disabled:opacity-50">Record payment</button>
        </div>
      </form>
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
      cashierReading: null,
      cashReceived: 0,
    }
  },
  computed: {
    // Meter values are the authoritative sequence. A corrected or normalized
    // timestamp must not make a lower meter reading appear above a later one.
    orderedReadings() {
      return [...this.readings].sort((first, second) => {
        const currentDifference = Number(second.current_reading || 0) - Number(first.current_reading || 0)
        if (currentDifference !== 0) return currentDifference

        const previousDifference = Number(second.previous_reading || 0) - Number(first.previous_reading || 0)
        if (previousDifference !== 0) return previousDifference

        const secondDate = new Date(second.reading_at || 0).getTime() || 0
        const firstDate = new Date(first.reading_at || 0).getTime() || 0
        if (secondDate !== firstDate) return secondDate - firstDate

        return Number(second.id || 0) - Number(first.id || 0)
      })
    },
    cumulativeBalances() {
      const balances = {}
      let outstanding = 0

      for (let index = this.orderedReadings.length - 1; index >= 0; index -= 1) {
        const reading = this.orderedReadings[index]
        const status = String(reading.status || '').toLowerCase()

        if (reading.bill_id && status !== 'paid' && status !== 'void') {
          outstanding += Number(reading.balance_due ?? reading.total_due ?? 0)
        }

        balances[reading.id] = outstanding
      }

      return balances
    },
    cashierAmountDue() {
      return Number(this.cashierReading?.balance_due ?? this.cashierReading?.total_due ?? 0)
    },
    changeAmount() {
      return Number(this.cashReceived || 0) - this.cashierAmountDue
    }
  },
  methods: {
    openCashier(reading) {
      this.cashierReading = reading
      this.cashReceived = this.cashierAmountDue
    },
    closeCashier() {
      this.cashierReading = null
      this.cashReceived = 0
    },
    submitCashier() {
      if (this.changeAmount < 0 || !this.cashierReading?.bill_id) return
      this.$emit('mark-paid', {
        reading: this.cashierReading,
        amount_paid: this.cashierAmountDue,
        cash_received: Number(this.cashReceived),
        change_amount: this.changeAmount
      })
      this.closeCashier()
    },
    formatReadingAt(value) {
      if (!value) return '-'
      const d = new Date(value)
      if (Number.isNaN(d.getTime())) return String(value)
      return d.toLocaleString('en-PH', {
        month: 'long',
        day: 'numeric',
        year: 'numeric',
        hour: 'numeric',
        minute: '2-digit',
        second: '2-digit',
        hour12: true,
      })
    },
    formatMoney(value) {
      const n = Number(value)
      if (Number.isNaN(n)) return '0.00'
      return n.toFixed(2)
    },
    cumulativeBalance(reading) {
      return this.cumulativeBalances[reading.id] ?? 0
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
