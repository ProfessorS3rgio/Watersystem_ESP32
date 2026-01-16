<template>
  <AppSidebar v-slot="{ isDark }">
    <div class="p-8">
      <div class="mb-8">
        <h1 class="text-3xl font-bold mb-2 transition-colors duration-200" :class="isDark ? 'text-white' : 'text-gray-900'">Dashboard</h1>
        <p class="transition-colors duration-200" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Monitor and control your water system</p>
      </div>

      <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6 mb-8">
        <div class="bg-gradient-to-r from-blue-500 to-blue-600 rounded-lg p-6 text-white">
          <div class="flex items-center">
            <svg class="h-8 w-8 mr-3" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 8c-1.657 0-3 .895-3 2s1.343 2 3 2 3 .895 3 2-1.343 2-3 2m0-8c1.11 0 2.08.402 2.599 1M12 8V7m0 1v8m0 0v1m0-1c-1.11 0-2.08-.402-2.599-1" />
            </svg>
            <div>
              <p class="text-blue-100 text-sm">Total Collectable (This Month)</p>
              <p class="text-2xl font-bold">{{ formatCurrency(totalCollectableThisMonth) }}</p>
            </div>
          </div>
        </div>

        <div class="bg-gradient-to-r from-purple-500 to-purple-600 rounded-lg p-6 text-white">
          <div class="flex items-center">
            <svg class="h-8 w-8 mr-3" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M8 9l3 3-3 3m5 0h3M5 20h14a2 2 0 002-2V6a2 2 0 00-2-2H5a2 2 0 00-2 2v12a2 2 0 002 2z" />
            </svg>
            <div>
              <p class="text-purple-100 text-sm">Number of Devices</p>
              <p class="text-2xl font-bold">{{ numberOfDevices }}</p>
            </div>
          </div>
        </div>

        <div class="bg-gradient-to-r from-green-500 to-green-600 rounded-lg p-6 text-white">
          <div class="flex items-center">
            <svg class="h-8 w-8 mr-3" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M17 20h5v-2a3 3 0 00-5.356-1.857M17 20H7m10 0v-2c0-.656-.126-1.283-.356-1.857M7 20H2v-2a3 3 0 015.356-1.857M7 20v-2c0-.656.126-1.283.356-1.857m0 0a5.002 5.002 0 019.288 0M15 7a3 3 0 11-6 0 3 3 0 016 0zM21 10a3 3 0 11-6 0 3 3 0 016 0zM3 10a3 3 0 116 0 3 3 0 01-6 0z" />
            </svg>
            <div>
              <p class="text-green-100 text-sm">Total Customers</p>
              <p class="text-2xl font-bold">{{ totalCustomers }}</p>
            </div>
          </div>
        </div>

        <div class="bg-gradient-to-r from-indigo-500 to-indigo-600 rounded-lg p-6 text-white">
          <div class="flex items-center">
            <svg class="h-8 w-8 mr-3" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M17 9V7a2 2 0 00-2-2H5a2 2 0 00-2 2v6a2 2 0 002 2h2m2 4h10a2 2 0 002-2v-6a2 2 0 00-2-2H7a2 2 0 00-2 2v6a2 2 0 002 2zm7-5a2 2 0 11-4 0 2 2 0 014 0z" />
            </svg>
            <div>
              <p class="text-indigo-100 text-sm">Total Sales (This Month)</p>
              <p class="text-2xl font-bold">{{ formatCurrency(totalSalesThisMonth) }}</p>
            </div>
          </div>
        </div>
      </div>
      </div>

      <div class="grid grid-cols-1 lg:grid-cols-2 gap-6">
          <div class="bg-gray-50 rounded-lg p-6">
            <h3 class="text-lg font-semibold text-gray-900 mb-4">Quick Actions</h3>
            <div class="space-y-3">
              <button class="w-full bg-white border border-gray-300 rounded-lg px-4 py-3 text-left hover:bg-gray-50 transition-colors duration-200">
                <div class="flex items-center">
                  <svg class="h-5 w-5 text-gray-400 mr-3" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M10.325 4.317c.426-1.756 2.924-1.756 3.35 0a1.724 1.724 0 002.573 1.066c1.543-.94 3.31.826 2.37 2.37a1.724 1.724 0 001.065 2.572c1.756.426 1.756 2.924 0 3.35a1.724 1.724 0 00-1.066 2.573c.94 1.543-.826 3.31-2.37 2.37a1.724 1.724 0 00-2.572 1.065c-.426 1.756-2.924 1.756-3.35 0a1.724 1.724 0 00-2.573-1.066c-1.543.94-3.31-.826-2.37-2.37a1.724 1.724 0 00-1.065-2.572c-1.756-.426-1.756-2.924 0-3.35a1.724 1.724 0 001.066-2.573c-.94-1.543.826-3.31 2.37-2.37.996.608 2.296.07 2.572-1.065z" />
                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M15 12a3 3 0 11-6 0 3 3 0 016 0z" />
                  </svg>
                  <span class="text-gray-900">System Settings</span>
                </div>
              </button>
              <button class="w-full bg-white border border-gray-300 rounded-lg px-4 py-3 text-left hover:bg-gray-50 transition-colors duration-200">
                <div class="flex items-center">
                  <svg class="h-5 w-5 text-gray-400 mr-3" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 19v-6a2 2 0 00-2-2H5a2 2 0 00-2 2v6a2 2 0 002 2h2a2 2 0 002-2zm0 0V9a2 2 0 012-2h2a2 2 0 012 2v10m-6 0a2 2 0 002 2h2a2 2 0 002-2m0 0V5a2 2 0 012-2h2a2 2 0 012 2v14a2 2 0 01-2 2h-2a2 2 0 01-2-2z" />
                  </svg>
                  <span class="text-gray-900">View Reports</span>
                </div>
              </button>
            </div>
          </div>

          <div class="bg-gray-50 rounded-lg p-6">
            <h3 class="text-lg font-semibold text-gray-900 mb-4">Recent Activity</h3>
            <div class="space-y-3">
              <div class="flex items-center text-sm text-gray-600">
                <div class="w-2 h-2 bg-green-500 rounded-full mr-3"></div>
                <span>System started successfully</span>
                <span class="ml-auto">2 min ago</span>
              </div>
              <div class="flex items-center text-sm text-gray-600">
                <div class="w-2 h-2 bg-blue-500 rounded-full mr-3"></div>
                <span>Sensors calibrated</span>
                <span class="ml-auto">5 min ago</span>
              </div>
              <div class="flex items-center text-sm text-gray-600">
                <div class="w-2 h-2 bg-yellow-500 rounded-full mr-3"></div>
                <span>Water level check completed</span>
                <span class="ml-auto">10 min ago</span>
              </div>
            </div>
          </div>
        </div>
    </AppSidebar>
</template>

<script>
import AppSidebar from '../components/AppSidebar.vue'

export default {
  name: 'Home',
  components: {
    AppSidebar
  },
  data() {
    return {
      totalCollectableThisMonth: 0,
      numberOfDevices: 0,
      totalCustomers: 0,
      totalSalesThisMonth: 0,
    }
  },
  mounted() {
    void this.loadDashboard()
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

        if (!res.ok) {
          return
        }

        const data = await res.json()
        this.totalCollectableThisMonth = Number(data?.total_collectable_this_month || 0)
        this.numberOfDevices = Number(data?.number_of_devices || 0)
        this.totalCustomers = Number(data?.total_customers || 0)
        this.totalSalesThisMonth = Number(data?.total_sales_this_month || 0)
      } catch (e) {
        // keep defaults
      }
    },
    formatCurrency(value) {
      const n = Number(value || 0)
      if (!Number.isFinite(n)) return '₱0.00'
      return '₱' + n.toFixed(2)
    }
  }
}
</script>
