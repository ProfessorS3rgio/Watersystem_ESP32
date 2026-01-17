<template>
  <div class="grid grid-cols-1 md:grid-cols-4 gap-6 mb-8">
    <div v-for="(barangay, index) in barangays" :key="barangay.brgy_id" class="bg-white rounded-lg shadow p-6">
      <div class="flex items-center">
        <div class="h-12 w-12 rounded-lg flex items-center justify-center" :class="getIconColor(index)">
          <svg class="h-6 w-6 text-white" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M17 20h5v-2a3 3 0 00-5.356-1.857M17 20H7m10 0v-2c0-.656-.126-1.283-.356-1.857M7 20H2v-2a3 3 0 015.356-1.857M7 20v-2c0-.656.126-1.283.356-1.857m0 0a5.002 5.002 0 019.288 0M15 7a3 3 0 11-6 0 3 3 0 016 0zm6 3a2 2 0 11-4 0 2 2 0 014 0zM7 10a2 2 0 11-4 0 2 2 0 014 0z" />
          </svg>
        </div>
        <div class="ml-4">
          <p class="text-sm text-gray-600">{{ barangay.barangay }}</p>
          <p class="text-2xl font-bold text-gray-900">{{ getClientCount(barangay.brgy_id) }}</p>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
import { computed } from 'vue'

export default {
  name: 'StatsCards',
  props: {
    customers: {
      type: Array,
      default: () => []
    },
    barangays: {
      type: Array,
      default: () => []
    }
  },
  setup(props) {
    const colors = ['bg-blue-500', 'bg-green-500', 'bg-yellow-500', 'bg-red-500', 'bg-purple-500', 'bg-indigo-500', 'bg-pink-500', 'bg-teal-500']

    const getIconColor = (index) => {
      return colors[index % colors.length]
    }

    const getClientCount = (brgyId) => {
      return props.customers.filter(customer => customer.brgy_id === brgyId).length
    }

    return {
      getIconColor,
      getClientCount
    }
  }
}
</script>