<template>
  <div v-if="showModal" class="fixed inset-0 z-50">
    <div class="absolute inset-0 bg-black/50" @click="$emit('close')"></div>
    <div class="absolute inset-0 flex items-center justify-center p-4">
      <div class="w-full max-w-md rounded-lg shadow-lg" :class="isDark ? 'bg-gray-800 text-white' : 'bg-white text-gray-900'">
        <div class="px-6 py-4 border-b" :class="isDark ? 'border-gray-700' : 'border-gray-200'">
          <div class="flex items-center justify-between">
            <h3 class="text-lg font-semibold text-green-600">Payment Successful!</h3>
            <button class="text-sm" :class="isDark ? 'text-gray-300 hover:text-white' : 'text-gray-600 hover:text-gray-900'" @click="$emit('close')">
              ✕
            </button>
          </div>
        </div>
        <div class="p-6">
          <div class="text-center mb-4">
            <div class="mx-auto flex items-center justify-center h-12 w-12 rounded-full bg-green-100">
              <svg class="h-6 w-6 text-green-600" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M5 13l4 4L19 7" />
              </svg>
            </div>
          </div>
          <div class="space-y-2 text-sm">
            <p><span class="font-medium">Customer:</span> {{ customerName }}</p>
            <p><span class="font-medium">Bill:</span> {{ receipt.reference_number }}</p>
            <p><span class="font-medium">Amount Paid:</span> ₱{{ formatMoney(receipt.amount_paid) }}</p>
            <p><span class="font-medium">Cash Received:</span> ₱{{ formatMoney(receipt.cash_received) }}</p>
            <p><span class="font-medium">Change:</span> ₱{{ formatMoney(receipt.change) }}</p>
            <p><span class="font-medium">Date:</span> {{ formatDate(receipt.created_at) }}</p>
          </div>
          <div class="mt-6 flex gap-3">
            <button
              @click="$emit('print')"
              class="flex-1 bg-blue-600 hover:bg-blue-700 text-white px-4 py-2 rounded-lg font-medium transition-colors duration-200"
            >
              Print Receipt
            </button>
            <button
              @click="$emit('close')"
              class="flex-1 px-4 py-2 rounded-lg font-medium transition-colors duration-200"
              :class="isDark ? 'bg-gray-700 hover:bg-gray-600 text-white' : 'bg-gray-200 hover:bg-gray-300 text-gray-900'"
            >
              Close
            </button>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
export default {
  name: 'PaymentReceipt',
  props: {
    isDark: {
      type: Boolean,
      required: true
    },
    showModal: {
      type: Boolean,
      default: false
    },
    customerName: {
      type: String,
      required: true
    },
    receipt: {
      type: Object,
      default: null
    }
  },
  emits: ['close', 'print'],
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