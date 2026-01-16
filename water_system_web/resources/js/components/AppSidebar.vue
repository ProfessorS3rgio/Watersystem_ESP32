<template>
  <div class="flex h-screen" :class="isDark ? 'bg-gray-900' : 'bg-gray-100'">
    <!-- Sidebar -->
    <aside class="w-64 flex flex-col shadow-2xl transition-colors duration-200" :class="isDark ? 'bg-gray-800 text-gray-100' : 'bg-white text-gray-900 border-r border-gray-200'">
      <!-- Logo/Header -->
      <div class="p-6 border-b transition-colors duration-200" :class="isDark ? 'border-gray-700' : 'border-gray-200'">
        <div class="flex items-center space-x-3">
          <div class="h-10 w-10 rounded-lg flex items-center justify-center transition-colors duration-200" :class="isDark ? 'bg-indigo-600' : 'bg-indigo-600'">
            <svg class="h-6 w-6 text-white" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 6V4m0 2a2 2 0 100 4m0-4a2 2 0 110 4m-6 8a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4m6 6v10m6-2a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4" />
            </svg>
          </div>
          <div>
            <h1 class="text-xl font-bold">Water System</h1>
            <p class="text-xs transition-colors duration-200" :class="isDark ? 'text-gray-400' : 'text-gray-500'">Management Portal</p>
          </div>
        </div>
      </div>

      <!-- Navigation -->
      <nav class="flex-1 p-4 space-y-2">
        <router-link
          to="/"
          class="flex items-center space-x-3 px-4 py-3 rounded-lg transition-all duration-200"
          :class="isActive('/') ? (isDark ? 'bg-indigo-600 text-white shadow-lg' : 'bg-indigo-600 text-white shadow-lg') : (isDark ? 'text-gray-300 hover:bg-gray-700' : 'text-gray-700 hover:bg-gray-100')"
        >
          <svg class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M3 12l2-2m0 0l7-7 7 7M5 10v10a1 1 0 001 1h3m10-11l2 2m-2-2v10a1 1 0 01-1 1h-3m-6 0a1 1 0 001-1v-4a1 1 0 011-1h2a1 1 0 011 1v4a1 1 0 001 1m-6 0h6" />
          </svg>
          <span class="font-medium">Dashboard</span>
        </router-link>

        <router-link
          to="/clients"
          class="flex items-center space-x-3 px-4 py-3 rounded-lg transition-all duration-200"
          :class="isActive('/clients') ? (isDark ? 'bg-indigo-600 text-white shadow-lg' : 'bg-indigo-600 text-white shadow-lg') : (isDark ? 'text-gray-300 hover:bg-gray-700' : 'text-gray-700 hover:bg-gray-100')"
        >
          <svg class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M17 20h5v-2a3 3 0 00-5.356-1.857M17 20H7m10 0v-2c0-.656-.126-1.283-.356-1.857M7 20H2v-2a3 3 0 015.356-1.857M7 20v-2c0-.656.126-1.283.356-1.857m0 0a5.002 5.002 0 019.288 0M15 7a3 3 0 11-6 0 3 3 0 016 0zm6 3a2 2 0 11-4 0 2 2 0 014 0zM7 10a2 2 0 11-4 0 2 2 0 014 0z" />
          </svg>
          <span class="font-medium">Client Management</span>
        </router-link>

        <router-link
          to="/sync"
          class="flex items-center space-x-3 px-4 py-3 rounded-lg transition-all duration-200"
          :class="isActive('/sync') ? (isDark ? 'bg-indigo-600 text-white shadow-lg' : 'bg-indigo-600 text-white shadow-lg') : (isDark ? 'text-gray-300 hover:bg-gray-700' : 'text-gray-700 hover:bg-gray-100')"
        >
          <svg class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M8 9l3 3-3 3m5 0h3M5 20h14a2 2 0 002-2V6a2 2 0 00-2-2H5a2 2 0 00-2 2v12a2 2 0 002 2z" />
          </svg>
          <span class="font-medium">Sync Device</span>
        </router-link>

        <router-link
          to="/settings"
          class="flex items-center space-x-3 px-4 py-3 rounded-lg transition-all duration-200"
          :class="isActive('/settings') ? (isDark ? 'bg-indigo-600 text-white shadow-lg' : 'bg-indigo-600 text-white shadow-lg') : (isDark ? 'text-gray-300 hover:bg-gray-700' : 'text-gray-700 hover:bg-gray-100')"
        >
          <svg class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 6V4m0 2a2 2 0 100 4m0-4a2 2 0 110 4m-6 8a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4m6 6v10m6-2a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4" />
          </svg>
          <span class="font-medium">Settings</span>
        </router-link>
      </nav>

      <!-- Theme Toggle -->
      <div class="p-4 border-t transition-colors duration-200" :class="isDark ? 'border-gray-700' : 'border-gray-200'">
        <button
          @click="toggleTheme"
          class="w-full flex items-center space-x-3 px-4 py-3 rounded-lg transition-all duration-200"
          :class="isDark ? 'bg-gray-700 hover:bg-gray-600 text-gray-200' : 'bg-gray-100 hover:bg-gray-200 text-gray-700'"
        >
          <svg v-if="isDark" class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 3v1m0 16v1m9-9h-1M4 12H3m15.364 6.364l-.707-.707M6.343 6.343l-.707-.707m12.728 0l-.707.707M6.343 17.657l-.707.707M16 12a4 4 0 11-8 0 4 4 0 018 0z" />
          </svg>
          <svg v-else class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M20.354 15.354A9 9 0 018.646 3.646 9.003 9.003 0 0012 21a9.003 9.003 0 008.354-5.646z" />
          </svg>
          <span class="font-medium">{{ isDark ? 'Light Mode' : 'Dark Mode' }}</span>
        </button>
      </div>

      <!-- User Profile -->
      <div class="p-4 border-t transition-colors duration-200" :class="isDark ? 'border-gray-700' : 'border-gray-200'">
        <div class="flex items-center space-x-3 px-4 py-3 rounded-lg transition-colors duration-200" :class="isDark ? 'bg-gray-700' : 'bg-gray-100'">
          <div class="h-8 w-8 bg-indigo-600 rounded-full flex items-center justify-center">
            <svg class="h-5 w-5 text-white" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M16 7a4 4 0 11-8 0 4 4 0 018 0zM12 14a7 7 0 00-7 7h14a7 7 0 00-7-7z" />
            </svg>
          </div>
          <div class="flex-1">
            <p class="text-sm font-medium">{{ user?.name || 'Admin' }}</p>
            <p class="text-xs transition-colors duration-200" :class="isDark ? 'text-gray-400' : 'text-gray-500'">Administrator</p>
          </div>
          <button
            @click="logout"
            class="transition-colors duration-200" :class="isDark ? 'text-gray-400 hover:text-white' : 'text-gray-500 hover:text-gray-900'"
            title="Logout"
          >
            <svg class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M17 16l4-4m0 0l-4-4m4 4H7m6 4v1a3 3 0 01-3 3H6a3 3 0 01-3-3V7a3 3 0 013-3h4a3 3 0 013 3v1" />
            </svg>
          </button>
        </div>
      </div>
    </aside>

    <!-- Main Content -->
    <main class="flex-1 overflow-y-auto transition-colors duration-200" :class="isDark ? 'bg-gray-900' : 'bg-gray-50'">
      <slot :isDark="isDark" />
    </main>
  </div>
</template>

<script>
import { useRoute, useRouter } from 'vue-router'
import { ref, onMounted } from 'vue'

export default {
  name: 'AppSidebar',
  setup() {
    const route = useRoute()
    const router = useRouter()
    const user = ref(null)
    const isDark = ref(localStorage.getItem('theme') === 'dark')

    const isActive = (path) => {
      return route.path === path
    }

    const fetchUser = async () => {
      try {
        const response = await fetch('/user')
        if (response.ok) {
          const data = await response.json()
          user.value = data.user
        }
      } catch (error) {
        console.error('Failed to fetch user:', error)
      }
    }

    const toggleTheme = () => {
      isDark.value = !isDark.value
      localStorage.setItem('theme', isDark.value ? 'dark' : 'light')
    }

    const logout = async () => {
      try {
        await fetch('/logout', {
          method: 'POST',
          headers: {
            'X-CSRF-TOKEN': document.querySelector('meta[name="csrf-token"]').getAttribute('content')
          }
        })
        router.push('/login')
      } catch (error) {
        console.error('Logout failed:', error)
      }
    }

    onMounted(() => {
      fetchUser()
    })

    return {
      isActive,
      logout,
      user,
      isDark,
      toggleTheme
    }
  }
}
</script>
