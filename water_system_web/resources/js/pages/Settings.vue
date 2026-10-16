<template>
  <AppSidebar v-slot="{ isDark }">
    <div class="p-8">
      <div class="mb-8">
        <h1 class="text-3xl font-bold mb-2 transition-colors duration-200" :class="isDark ? 'text-white' : 'text-gray-900'">Settings</h1>
        <p class="transition-colors duration-200" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Manage billing rules, service policies, and data protection</p>
      </div>

      <div class="bg-white rounded-lg shadow p-6" :class="isDark ? 'bg-gray-900 text-white' : 'bg-white text-gray-900'">
        <div v-if="isLoading" class="text-sm" :class="isDark ? 'text-gray-300' : 'text-gray-600'">Loading settings...</div>
        <div v-else>
          <div v-if="errorMessage" class="text-sm text-red-600 mb-4">{{ errorMessage }}</div>

          <form v-if="activeSection === 'billing'" class="space-y-4" @submit.prevent="saveSettings">
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

            <div>
              <label class="block text-sm font-medium mb-1">Days Before Additional Charge</label>
              <input
                v-model.number="form.penalty_after_days"
                type="number"
                min="0"
                class="w-full px-4 py-2 rounded-lg border"
                :class="inputClass(isDark)"
                required
              />
              <p class="text-xs mt-1" :class="isDark ? 'text-gray-400' : 'text-gray-500'">Additional charge starts after the bill due date plus this many days.</p>
            </div>

            <div>
              <label class="block text-sm font-medium mb-1">Additional Charge</label>
              <input
                v-model.number="form.penalty_amount"
                type="number"
                min="0"
                step="0.01"
                class="w-full px-4 py-2 rounded-lg border"
                :class="inputClass(isDark)"
                required
              />
              <p class="text-xs mt-1" :class="isDark ? 'text-gray-400' : 'text-gray-500'">This charge is added when an overdue bill is paid.</p>
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

          <form v-else-if="activeSection === 'categories'" class="space-y-4" @submit.prevent="saveCategories(isDark)">
            <div>
              <h2 class="text-lg font-semibold">Customer categories</h2>
              <p class="mt-1 text-sm" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Update the rate, minimum charge, and overdue penalty used for each customer category.</p>
            </div>
            <div v-if="categoriesLoading" class="text-sm" :class="isDark ? 'text-gray-300' : 'text-gray-600'">Loading customer categories...</div>
            <div v-else class="overflow-x-auto">
              <table class="w-full text-sm">
                <thead :class="isDark ? 'bg-gray-800 text-gray-300' : 'bg-gray-50 text-gray-600'"><tr><th class="px-3 py-2 text-left">Category</th><th class="px-3 py-2 text-left">Rate / m³</th><th class="px-3 py-2 text-left">Minimum charge</th><th class="px-3 py-2 text-left">Late penalty</th></tr></thead>
                <tbody class="divide-y" :class="isDark ? 'divide-gray-700' : 'divide-gray-200'">
                  <tr v-for="category in categories" :key="category.type_id">
                    <td class="px-3 py-3 font-medium">{{ category.type_name }}</td>
                    <td class="px-3 py-3"><input v-model.number="category.rate_per_m3" type="number" min="0" step="0.01" class="w-32 rounded-lg border px-3 py-2" :class="inputClass(isDark)" required /></td>
                    <td class="px-3 py-3"><input v-model.number="category.min_charge" type="number" min="0" step="0.01" class="w-32 rounded-lg border px-3 py-2" :class="inputClass(isDark)" required /></td>
                    <td class="px-3 py-3"><input v-model.number="category.penalty" type="number" min="0" step="0.01" class="w-32 rounded-lg border px-3 py-2" :class="inputClass(isDark)" required /></td>
                  </tr>
                </tbody>
              </table>
            </div>
            <div class="flex items-center justify-end gap-3"><button type="submit" :disabled="categoriesSaving || categoriesLoading" class="bg-indigo-600 hover:bg-indigo-700 text-white px-4 py-2 rounded-lg font-medium disabled:opacity-50">{{ categoriesSaving ? 'Saving...' : 'Save Categories' }}</button></div>
            <p v-if="categoriesMessage" class="text-sm" :class="categoriesError ? 'text-red-600' : 'text-green-600'">{{ categoriesMessage }}</p>
          </form>

          <div v-else class="rounded-xl border p-6" :class="isDark ? 'border-gray-700 bg-gray-800/60' : 'border-gray-200 bg-gray-50'">
            <div class="flex items-start gap-4">
              <div class="flex h-11 w-11 shrink-0 items-center justify-center rounded-lg bg-indigo-100 text-indigo-600" :class="isDark ? 'bg-indigo-900/50 text-indigo-300' : ''">
                <svg class="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor" aria-hidden="true">
                  <path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.8" d="M12 16V4m0 0L8 8m4-4 4 4M5 14v4a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2v-4" />
                </svg>
              </div>
              <div>
                <h2 class="text-lg font-semibold">Data backup</h2>
                <p class="mt-1 text-sm" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Create a downloadable copy of customers, readings, bills, payments, and system settings.</p>
              </div>
            </div>
            <div class="mt-6 flex items-center justify-between gap-4 rounded-lg border p-4" :class="isDark ? 'border-gray-700 bg-gray-900' : 'border-gray-200 bg-white'">
              <div>
                <p class="font-medium">Backup workflow</p>
                <p class="mt-1 text-xs" :class="isDark ? 'text-gray-400' : 'text-gray-500'">The backup format and restore process will be added here.</p>
              </div>
              <button type="button" disabled class="rounded-lg bg-gray-400 px-4 py-2 text-sm font-medium text-white opacity-70 cursor-not-allowed">Backup data</button>
            </div>
          </div>
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
      activeSection: this.$route.path.endsWith('/backup') ? 'backup' : (this.$route.path.endsWith('/categories') ? 'categories' : 'billing'),
      categories: [],
      categoriesLoading: false,
      categoriesSaving: false,
      categoriesMessage: '',
      categoriesError: false,
      errorMessage: '',
      successMessage: '',
      form: {
        bill_due_days: 5,
        disconnection_days: 8,
        penalty_after_days: 0,
        penalty_amount: 0,
      }
    }
  },
  mounted() {
    void this.reload()
    void this.loadCategories()
  },
  watch: {
    '$route.path'(path) {
      this.activeSection = path.endsWith('/backup') ? 'backup' : (path.endsWith('/categories') ? 'categories' : 'billing')
    }
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
          penalty_after_days: Number(d.penalty_after_days ?? 0),
          penalty_amount: Number(d.penalty_amount ?? 0),
        }
      } catch (e) {
        this.errorMessage = e?.message || 'Failed to load settings'
      } finally {
        this.isLoading = false
      }
    },
    async loadCategories() {
      this.categoriesLoading = true
      try {
        const res = await fetch('/customer-types', { headers: { Accept: 'application/json' }, credentials: 'same-origin' })
        if (!res.ok) throw new Error('Failed to load customer categories')
        const json = await res.json()
        this.categories = Array.isArray(json?.data) ? json.data : []
      } catch (e) {
        this.categoriesError = true
        this.categoriesMessage = e?.message || 'Failed to load customer categories'
      } finally {
        this.categoriesLoading = false
      }
    },
    async saveCategories() {
      this.categoriesSaving = true
      this.categoriesMessage = ''
      this.categoriesError = false
      try {
        const csrf = document.querySelector('meta[name="csrf-token"]')?.getAttribute('content')
        for (const category of this.categories) {
          const res = await fetch(`/customer-types/${category.type_id}`, {
            method: 'PUT',
            headers: { Accept: 'application/json', 'Content-Type': 'application/json', ...(csrf ? { 'X-CSRF-TOKEN': csrf } : {}) },
            credentials: 'same-origin',
            body: JSON.stringify({ rate_per_m3: category.rate_per_m3, min_m3: category.min_m3, min_charge: category.min_charge, penalty: category.penalty })
          })
          if (!res.ok) throw new Error('Failed to save customer categories')
        }
        this.categoriesMessage = 'Customer category rates saved.'
      } catch (e) {
        this.categoriesError = true
        this.categoriesMessage = e?.message || 'Failed to save customer categories'
      } finally {
        this.categoriesSaving = false
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
