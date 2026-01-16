<template>
  <div class="lg:col-span-1">
    <div class="bg-white rounded-lg shadow-sm border" :class="isDark ? 'bg-gray-800 border-gray-700' : 'bg-white border-gray-200'">
      <div class="px-6 py-4 border-b" :class="isDark ? 'border-gray-700' : 'border-gray-200'">
        <h3 class="text-lg font-semibold">Find Customer</h3>
      </div>
      <div class="p-6 space-y-4">
        <!-- Search by Account Number -->
        <div>
          <label class="block text-sm font-medium mb-2" :class="isDark ? 'text-gray-200' : 'text-gray-700'">Account Number</label>
          <input
            v-model="searchAccountNo"
            type="text"
            placeholder="Enter account number"
            class="w-full px-3 py-2 border rounded-lg focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500"
            :class="isDark ? 'bg-gray-700 border-gray-600 text-white' : 'bg-white border-gray-300 text-gray-900'"
            @keyup.enter="handleSearch"
          >
        </div>

        <!-- Search by Name -->
        <div>
          <label class="block text-sm font-medium mb-2" :class="isDark ? 'text-gray-200' : 'text-gray-700'">Customer Name</label>
          <input
            v-model="searchName"
            type="text"
            placeholder="Enter customer name"
            class="w-full px-3 py-2 border rounded-lg focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500"
            :class="isDark ? 'bg-gray-700 border-gray-600 text-white' : 'bg-white border-gray-300 text-gray-900'"
            @keyup.enter="handleSearch"
          >
        </div>

        <button
          @click="handleSearch"
          :disabled="searching"
          class="w-full bg-indigo-600 hover:bg-indigo-700 text-white px-4 py-2 rounded-lg font-medium disabled:opacity-50 transition-colors duration-200"
        >
          {{ searching ? 'Searching...' : 'Search Customer' }}
        </button>

        <!-- Selected Customer Info -->
        <div v-if="selectedCustomer" class="mt-6 p-4 rounded-lg border" :class="isDark ? 'bg-gray-700 border-gray-600' : 'bg-gray-50 border-gray-200'">
          <h4 class="font-semibold mb-2">Selected Customer</h4>
          <div class="space-y-1 text-sm">
            <p><span class="font-medium">Name:</span> {{ selectedCustomer.customer_name }}</p>
            <p><span class="font-medium">Account:</span> {{ selectedCustomer.account_no }}</p>
            <p><span class="font-medium">Address:</span> {{ selectedCustomer.address || 'N/A' }}</p>
          </div>
          <button
            @click="$emit('clear-customer')"
            class="mt-3 text-sm text-red-600 hover:text-red-800"
          >
            Clear Selection
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
import { ref } from 'vue'

export default {
  name: 'CustomerSearch',
  props: {
    isDark: {
      type: Boolean,
      required: true
    },
    selectedCustomer: {
      type: Object,
      default: null
    }
  },
  emits: ['search-customer', 'clear-customer'],
  setup(props, { emit }) {
    const searchAccountNo = ref('')
    const searchName = ref('')
    const searching = ref(false)

    const handleSearch = () => {
      if (!searchAccountNo.value.trim() && !searchName.value.trim()) return

      emit('search-customer', {
        accountNo: searchAccountNo.value.trim(),
        name: searchName.value.trim(),
        searching: searching
      })
    }

    return {
      searchAccountNo,
      searchName,
      searching,
      handleSearch
    }
  }
}
</script>