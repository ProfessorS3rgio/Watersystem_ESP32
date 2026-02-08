<template>
  <AppSidebar v-slot="{ isDark }">
    <div class="p-8">
      <div class="mb-8">
        <h1 class="text-3xl font-bold mb-2 transition-colors duration-200" :class="isDark ? 'text-white' : 'text-gray-900'">Settings</h1>
        <p class="transition-colors duration-200" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Configure billing due days and disconnection rules</p>
      </div>

      <div class="bg-white rounded-lg shadow p-6" :class="isDark ? 'bg-gray-900 text-white' : 'bg-white text-gray-900'">
        <div v-if="isLoading" class="text-sm" :class="isDark ? 'text-gray-300' : 'text-gray-600'">Loading settings...</div>
        <div v-else>
          <div v-if="errorMessage" class="text-sm text-red-600 mb-4">{{ errorMessage }}</div>

          <form class="space-y-4" @submit.prevent="saveSettings">
            <div>
              <label class="block text-sm font-medium mb-1">Days Before Due Date</label>
              <input
                v-model.number="form.bill_due_days"
                type="number"
                min="0"
                class="w-full px-4 py-2 rounded-lg border"
                :class="inputClass(isDark)"
                required
              />
              <p class="text-xs mt-1" :class="isDark ? 'text-gray-400' : 'text-gray-500'">Due date = bill date + this many days</p>
            </div>

            <div>
              <label class="block text-sm font-medium mb-1">Disconnection Days (days past due)</label>
              <input
                v-model.number="form.disconnection_days"
                type="number"
                min="0"
                class="w-full px-4 py-2 rounded-lg border"
                :class="inputClass(isDark)"
                required
              />
              <p class="text-xs mt-1" :class="isDark ? 'text-gray-400' : 'text-gray-500'">If today is greater than due date + this many days, service may be disconnected</p>
            </div>

            <div class="pt-2 flex items-center justify-end gap-3">
              <button
                type="button"
                @click="reload"
                :disabled="isSaving"
                class="px-4 py-2 rounded-lg"
                :class="isDark ? 'bg-gray-800 hover:bg-gray-700 text-white disabled:opacity-50' : 'bg-gray-100 hover:bg-gray-200 text-gray-900 disabled:opacity-50'"
              >
                Reset
              </button>
              <button
                type="submit"
                :disabled="isSaving"
                class="bg-indigo-600 hover:bg-indigo-700 text-white px-4 py-2 rounded-lg font-medium disabled:opacity-50"
              >
                {{ isSaving ? 'Saving...' : 'Save Settings' }}
              </button>
            </div>

            <p v-if="successMessage" class="text-sm text-green-600">{{ successMessage }}</p>
          </form>
        </div>
      </div>
    </div>
  </AppSidebar>
</template>

<script>
import AppSidebar from '../components/AppSidebar.vue'

export default {
  name: 'Settings',
  components: { AppSidebar },
  data() {
    return {
      isLoading: false,
      isSaving: false,
      errorMessage: '',
      successMessage: '',
      form: {
        bill_due_days: 5,
        disconnection_days: 8,
      }
    }
  },
  mounted() {
    void this.reload()
  },
  methods: {
    inputClass(isDark) {
      return isDark
        ? 'bg-gray-800 border-gray-700 text-white placeholder:text-gray-400 focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500'
        : 'bg-white border-gray-300 text-gray-900 placeholder:text-gray-400 focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500'
    },
    async reload() {
      this.isLoading = true
      this.errorMessage = ''
      this.successMessage = ''
      try {
        const res = await fetch('/settings', {
          headers: { 'Accept': 'application/json' },
          credentials: 'same-origin',
        })

        if (!res.ok) {
          if (res.status === 401) {
            this.errorMessage = 'Unauthorized. Please login again.'
            return
          }
          throw new Error('Failed to load settings')
        }

        const json = await res.json()
        const d = json?.data || {}
        this.form = {
          bill_due_days: Number(d.bill_due_days ?? 5),
          disconnection_days: Number(d.disconnection_days ?? 8),
        }
      } catch (e) {
        this.errorMessage = e?.message || 'Failed to load settings'
      } finally {
        this.isLoading = false
      }
    },
    async saveSettings() {
      this.isSaving = true
      this.errorMessage = ''
      this.successMessage = ''

      const csrf = document.querySelector('meta[name="csrf-token"]')?.getAttribute('content')
      try {
        const res = await fetch('/settings', {
          method: 'PUT',
          headers: {
            'Accept': 'application/json',
            'Content-Type': 'application/json',
            ...(csrf ? { 'X-CSRF-TOKEN': csrf } : {}),
          },
          credentials: 'same-origin',
          body: JSON.stringify(this.form),
        })

        if (res.status === 422) {
          const json = await res.json()
          const msg = json?.message || 'Validation failed'
          throw new Error(msg)
        }

        if (!res.ok) {
          if (res.status === 401) throw new Error('Unauthorized. Please login again.')
          const text = await res.text()
          throw new Error('Failed to save settings: ' + text)
        }

        this.successMessage = 'Settings saved.'
        await this.reload()
      } catch (e) {
        this.errorMessage = e?.message || 'Failed to save settings'
      } finally {
        this.isSaving = false
      }
    }
  }
}
</script>
