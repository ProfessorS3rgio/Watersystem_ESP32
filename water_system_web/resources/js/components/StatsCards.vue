<template>
  <div class="grid grid-cols-1 md:grid-cols-4 gap-6 mb-8">
    <!-- Barangay Cards -->
    <div
      v-for="(barangay, index) in barangays"
      :key="barangay.brgy_id"
      @click="clickable ? $emit('select-barangay', selectedBarangay === barangay.brgy_id ? null : barangay.brgy_id) : null"
      class="rounded-lg shadow p-5 transition-all duration-200 hover:shadow-lg"
      :class="[
        isDark ? 'bg-gray-800' : 'bg-white',
        clickable ? 'cursor-pointer' : '',
        clickable && selectedBarangay === barangay.brgy_id 
          ? (isDark ? 'ring-2 ring-indigo-400 bg-indigo-900/20' : 'ring-2 ring-indigo-500 bg-indigo-50')
          : clickable ? (isDark ? 'hover:bg-gray-700' : 'hover:bg-gray-50') : ''
      ]"
    >
      <div class="flex items-center">
        <div class="h-11 w-11 rounded-lg flex items-center justify-center" :class="getIconColor(index)">
          <svg class="h-6 w-6 text-white" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M17 20h5v-2a3 3 0 00-5.356-1.857M17 20H7m10 0v-2c0-.656-.126-1.283-.356-1.857M7 20H2v-2a3 3 0 015.356-1.857M7 20v-2c0-.656.126-1.283.356-1.857m0 0a5.002 5.002 0 019.288 0M15 7a3 3 0 11-6 0 3 3 0 016 0zm6 3a2 2 0 11-4 0 2 2 0 014 0zM7 10a2 2 0 11-4 0 2 2 0 014 0z" />
          </svg>
        </div>
        <div class="ml-4">
          <p :class="isDark ? 'text-gray-400' : 'text-gray-600'" class="text-sm">{{ barangay.barangay }}</p>
          <p :class="isDark ? 'text-white' : 'text-gray-900'" class="text-2xl font-bold">{{ displayedCounts[barangay.brgy_id] || 0 }}</p>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
import { ref, watchEffect } from 'vue'

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
    },
    selectedBarangay: {
      type: [Number, String, null],
      default: null
    },
    isDark: {
      type: Boolean,
      default: false
    },
    clickable: {
      type: Boolean,
      default: true
    }
  },
  emits: ['select-barangay'],
  setup(props) {
    const colors = ['bg-blue-500', 'bg-green-500', 'bg-yellow-500', 'bg-red-500', 'bg-purple-500', 'bg-indigo-500', 'bg-pink-500', 'bg-teal-500']
    const displayedCounts = ref({})

    const getIconColor = (index) => {
      return colors[index % colors.length]
    }

    const getClientCount = (brgyId) => {
      return props.customers.filter(customer => customer.brgy_id === brgyId).length
    }

    const animateCount = (key, target) => {
      const start = 0
      if (start === target) {
        displayedCounts.value[key] = target
        return
      }

      const duration = 1000 // 1 second total
      const steps = 60 // 60 fps for smooth animation
      const increment = Math.max(1, Math.ceil(target / steps))
      const delay = duration / steps
      let current = start

      const animate = () => {
        current += increment
        if (current >= target) {
          current = target
        }
        displayedCounts.value[key] = current
        if (current < target) {
          setTimeout(animate, delay)
        }
      }

      animate()
    }

    // Animate counts whenever customers or barangays change
    watchEffect(() => {
      props.barangays.forEach(barangay => {
        const target = getClientCount(barangay.brgy_id)
        animateCount(barangay.brgy_id, target)
      })
    })

    return {
      getIconColor,
      getClientCount,
      displayedCounts
    }
  }
}
</script>