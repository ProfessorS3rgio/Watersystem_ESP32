<template>
  <div v-if="selectedBill" class="bg-white rounded-lg shadow-sm border" :class="isDark ? 'bg-gray-800 border-gray-700' : 'bg-white border-gray-200'">
    <div class="px-6 py-4 border-b" :class="isDark ? 'border-gray-700' : 'border-gray-200'">
      <h3 class="text-lg font-semibold">Process Cash Payment</h3>
    </div>
    <div class="p-6">
      <div class="mb-6 p-4 rounded-lg border" :class="isDark ? 'bg-gray-700 border-gray-600' : 'bg-gray-50 border-gray-200'">
        <h4 class="font-semibold mb-2">Bill Details</h4>
        <div class="grid grid-cols-2 gap-4 text-sm">
          <div>
            <p><span class="font-medium">Bill Number:</span> {{ selectedBill.reference_number }}</p>
            <p><span class="font-medium">Customer:</span> {{ customerName }}</p>
          </div>
          <div>
            <p><span class="font-medium">Amount Due:</span> ₱{{ formatMoney(selectedBill.total_due) }}</p>
            <p><span class="font-medium">Due Date:</span> {{ formatDate(selectedBill.due_date) }}</p>
          </div>
        </div>
      </div>

      <form @submit.prevent="handlePayment" class="space-y-4">
        <div>
          <label class="block text-sm font-medium mb-2" :class="isDark ? 'text-gray-200' : 'text-gray-700'">
            Cash Received *
          </label>
          <input
            v-model.number="cashReceived"
            type="number"
            step="0.01"
            min="0"
            placeholder="0.00"
            class="w-full px-3 py-2 border rounded-lg focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500"
            :class="isDark ? 'bg-gray-700 border-gray-600 text-white' : 'bg-white border-gray-300 text-gray-900'"
            required
          >
        </div>

        <div v-if="cashReceived > 0" class="p-4 rounded-lg" :class="changeAmount >= 0 ? (isDark ? 'bg-green-900/20 border border-green-700' : 'bg-green-50 border border-green-200') : (isDark ? 'bg-red-900/20 border border-red-700' : 'bg-red-50 border border-red-200')">
          <div class="flex justify-between items-center">
            <span class="font-medium">Change:</span>
            <span class="text-lg font-bold" :class="changeAmount >= 0 ? 'text-green-600' : 'text-red-600'">
              ₱{{ formatMoney(Math.abs(changeAmount)) }}
            </span>
          </div>
          <p v-if="changeAmount < 0" class="text-sm text-red-600 mt-1">
            Insufficient payment amount
          </p>
        </div>

        <div class="flex gap-3">
          <button
            type="button"
            @click="$emit('cancel-payment')"
            class="flex-1 px-4 py-2 rounded-lg font-medium transition-colors duration-200"
            :class="isDark ? 'bg-gray-700 hover:bg-gray-600 text-white' : 'bg-gray-200 hover:bg-gray-300 text-gray-900'"
          >
            Cancel
          </button>
          <button
            type="submit"
            :disabled="processingPayment || cashReceived <= 0 || changeAmount < 0"
            class="flex-1 bg-green-600 hover:bg-green-700 text-white px-4 py-2 rounded-lg font-medium disabled:opacity-50 transition-colors duration-200"
          >
            {{ processingPayment ? 'Processing...' : 'Complete Payment' }}
          </button>
        </div>
      </form>
    </div>
  </div>
</template>

<script>
import { ref, computed } from 'vue'

export default {
  name: 'PaymentForm',
  props: {
    isDark: {
      type: Boolean,
      required: true
    },
    selectedBill: {
      type: Object,
      default: null
    },
    customerName: {
      type: String,
      required: true
    },
    processingPayment: {
      type: Boolean,
      default: false
    }
  },
  emits: ['process-payment', 'cancel-payment'],
  setup(props, { emit }) {
    const cashReceived = ref(props.selectedBill ? props.selectedBill.total_due : 0)

    const changeAmount = computed(() => {
      if (!props.selectedBill || cashReceived.value <= 0) return 0
      return cashReceived.value - props.selectedBill.total_due
    })

    const handlePayment = () => {
      if (!props.selectedBill || cashReceived.value <= 0 || changeAmount.value < 0) return

      emit('process-payment', {
        bill: props.selectedBill,
        cashReceived: cashReceived.value,
        changeAmount: changeAmount.value
      })
    }

    const formatDate = (dateString) => {
      if (!dateString) return 'N/A'
      const date = new Date(dateString)
      return date.toLocaleDateString()
    }

    const formatMoney = (amount) => {
      return Number(amount).toFixed(2)
    }

    return {
      cashReceived,
      changeAmount,
      handlePayment,
      formatDate,
      formatMoney
    }
  }
}
</script>