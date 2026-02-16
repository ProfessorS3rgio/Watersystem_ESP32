<template>
  <AppSidebar v-slot="{ isDark }">
    <div class="p-8">
      <div class="mb-8">
        <h1 class="text-3xl font-bold mb-2 transition-colors duration-200" :class="isDark ? 'text-white' : 'text-gray-900'">Dashboard</h1>
        <p class="transition-colors duration-200" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Monitor and control your water system</p>
      </div>

      <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6 mb-8">
        <div class="rounded-lg shadow p-5 transition-all duration-200 hover:shadow-lg" :class="isDark ? 'bg-gray-800' : 'bg-white'">
          <div class="flex items-center">
            <div class="h-11 w-11 rounded-lg flex items-center justify-center bg-blue-500">
              <svg class="h-6 w-6 text-white" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 8c-1.657 0-3 .895-3 2s1.343 2 3 2 3 .895 3 2-1.343 2-3 2m0-8c1.11 0 2.08.402 2.599 1M12 8V7m0 1v8m0 0v1m0-1c-1.11 0-2.08-.402-2.599-1" />
              </svg>
            </div>
            <div class="ml-4">
              <p :class="isDark ? 'text-gray-400' : 'text-gray-600'" class="text-sm">Total Collectable (This Month)</p>
              <p :class="isDark ? 'text-white' : 'text-gray-900'" class="text-xl font-bold">{{ formatCurrency(animatedCollectable) }}</p>
            </div>
          </div>
        </div>

        <div class="rounded-lg shadow p-5 transition-all duration-200 hover:shadow-lg" :class="isDark ? 'bg-gray-800' : 'bg-white'">
          <div class="flex items-center">
            <div class="h-11 w-11 rounded-lg flex items-center justify-center bg-purple-500">
              <svg class="h-6 w-6 text-white" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M8 9l3 3-3 3m5 0h3M5 20h14a2 2 0 002-2V6a2 2 0 00-2-2H5a2 2 0 00-2 2v12a2 2 0 002 2z" />
              </svg>
            </div>
            <div class="ml-4">
              <p :class="isDark ? 'text-gray-400' : 'text-gray-600'" class="text-sm">Devices</p>
              <p :class="isDark ? 'text-white' : 'text-gray-900'" class="text-xl font-bold">{{ animatedDevices }}</p>
            </div>
          </div>
        </div>

        <div class="rounded-lg shadow p-5 transition-all duration-200 hover:shadow-lg" :class="isDark ? 'bg-gray-800' : 'bg-white'">
          <div class="flex items-center">
            <div class="h-11 w-11 rounded-lg flex items-center justify-center bg-green-500">
              <svg class="h-6 w-6 text-white" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M17 20h5v-2a3 3 0 00-5.356-1.857M17 20H7m10 0v-2c0-.656-.126-1.283-.356-1.857M7 20H2v-2a3 3 0 015.356-1.857M7 20v-2c0-.656.126-1.283.356-1.857m0 0a5.002 5.002 0 019.288 0M15 7a3 3 0 11-6 0 3 3 0 016 0zM21 10a3 3 0 11-6 0 3 3 0 016 0zM3 10a3 3 0 116 0 3 3 0 01-6 0z" />
              </svg>
            </div>
            <div class="ml-4">
              <p :class="isDark ? 'text-gray-400' : 'text-gray-600'" class="text-sm">Customers</p>
              <p :class="isDark ? 'text-white' : 'text-gray-900'" class="text-xl font-bold">{{ animatedCustomers }}</p>
            </div>
          </div>
        </div>

        <div class="rounded-lg shadow p-5 transition-all duration-200 hover:shadow-lg" :class="isDark ? 'bg-gray-800' : 'bg-white'">
          <div class="flex items-center">
            <div class="h-11 w-11 rounded-lg flex items-center justify-center bg-indigo-500">
              <svg class="h-6 w-6 text-white" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M17 9V7a2 2 0 00-2-2H5a2 2 0 00-2 2v6a2 2 0 002 2h2m2 4h10a2 2 0 002-2v-6a2 2 0 00-2-2H7a2 2 0 00-2 2v6a2 2 0 002 2zm7-5a2 2 0 11-4 0 2 2 0 014 0z" />
              </svg>
            </div>
            <div class="ml-4">
              <p :class="isDark ? 'text-gray-400' : 'text-gray-600'" class="text-sm">Total Sales (This Month)</p>
              <p :class="isDark ? 'text-white' : 'text-gray-900'" class="text-xl font-bold">{{ formatCurrency(animatedSales) }}</p>
            </div>
          </div>
        </div>
      </div>

      <!-- Barangay Stats Cards -->
      <StatsCards
        :customers="customers"
        :barangays="barangays"
        :selectedBarangay="selectedBarangay"
        :isDark="isDark"
        :clickable="false"
        @select-barangay="selectBarangay"
      />

    </div>

   
    </AppSidebar>
</template>

<script>
import AppSidebar from '../components/AppSidebar.vue'
import StatsCards from '../components/StatsCards.vue'

export default {
  name: 'Home',
  components: {
    AppSidebar,
    StatsCards
  },
  data() {
    return {
      totalCollectableThisMonth: 0,
      numberOfDevices: 0,
      totalCustomers: 0,
      totalSalesThisMonth: 0,
      // Animated values
      animatedCollectable: 0,
      animatedDevices: 0,
      animatedCustomers: 0,
      animatedSales: 0,
      // StatsCards data
      customers: [],
      barangays: [],
      selectedBarangay: null,
    }
  },
  mounted() {
    void this.loadDashboard()
    void this.loadCustomers()
    void this.loadBarangays()
  },
  methods: {
    async loadDashboard() {
      try {
        const res = await fetch('/dashboard', {
          method: 'GET',
          credentials: 'same-origin',
          headers: {
            Accept: 'application/json'
          }
        })

        console.log('Dashboard response status:', res.status)
        console.log('Dashboard response ok:', res.ok)

        if (!res.ok) {
          console.error('Dashboard fetch failed with status:', res.status)
          return
        }

        const data = await res.json()
        console.log('Dashboard data:', data)
        this.totalCollectableThisMonth = Number(data?.total_collectable_this_month || 0)
        this.numberOfDevices = Number(data?.number_of_devices || 0)
        this.totalCustomers = Number(data?.total_customers || 0)
        this.totalSalesThisMonth = Number(data?.total_sales_this_month || 0)

        // Start animations
        this.animateValue('animatedCollectable', this.totalCollectableThisMonth)
        this.animateValue('animatedDevices', this.numberOfDevices)
        this.animateValue('animatedCustomers', this.totalCustomers)
        this.animateValue('animatedSales', this.totalSalesThisMonth)
      } catch (e) {
        console.error('Dashboard load error:', e)
        // keep defaults
      }
    },
    formatCurrency(value) {
      const n = Number(value || 0)
      if (!Number.isFinite(n)) return '₱0.00'
      return '₱' + n.toFixed(2)
    },
    animateValue(property, target) {
      const duration = 1000 // 1 second
      const start = this[property]
      const increment = (target - start) / (duration / 50) // 50ms intervals
      const isCurrency = property === 'animatedCollectable' || property === 'animatedSales'
      let current = start
      const timer = setInterval(() => {
        current += increment
        if ((increment > 0 && current >= target) || (increment < 0 && current <= target)) {
          this[property] = target
          clearInterval(timer)
        } else {
          this[property] = isCurrency ? current : Math.round(current)
        }
      }, 50)
    },
    async loadCustomers() {
      try {
        const res = await fetch('/customers', {
          method: 'GET',
          credentials: 'same-origin',
          headers: {
            Accept: 'application/json'
          }
        })
        if (res.ok) {
          const data = await res.json()
          this.customers = Array.isArray(data?.data) ? data.data : []
        }
      } catch (e) {
        console.error('Customers load error:', e)
      }
    },
    async loadBarangays() {
      try {
        const res = await fetch('/barangays', {
          method: 'GET',
          credentials: 'same-origin',
          headers: {
            Accept: 'application/json'
          }
        })
        if (res.ok) {
          const data = await res.json()
          this.barangays = Array.isArray(data?.data) ? data.data : []
        }
      } catch (e) {
        console.error('Barangays load error:', e)
      }
    },
    selectBarangay(brgyId) {
      this.selectedBarangay = brgyId
    }
  }
}
</script>
