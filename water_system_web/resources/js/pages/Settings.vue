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

          <form v-if="activeSection === 'billing'" class="space-y-6" @submit.prevent="saveSettings">
            <div class="flex items-start gap-3 border-b pb-5" :class="isDark ? 'border-gray-700' : 'border-gray-200'">
              <div class="settings-section-icon bg-indigo-100 text-indigo-600" :class="isDark ? 'bg-indigo-900/50 text-indigo-300' : ''"><svg class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.8" d="M6 4.75h12A1.25 1.25 0 0 1 19.25 6v12A1.25 1.25 0 0 1 18 19.25H6A1.25 1.25 0 0 1 4.75 18V6A1.25 1.25 0 0 1 6 4.75ZM8 9h8M8 12h5M8 15h8" /></svg></div>
              <div><h2 class="text-lg font-semibold">Billing rules</h2><p class="mt-1 text-sm" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Control when bills become due and how overdue accounts are handled.</p></div>
            </div>
            <div class="grid gap-4 md:grid-cols-2">
            <div class="rounded-xl border p-4" :class="isDark ? 'border-gray-700 bg-gray-800/50' : 'border-gray-200 bg-gray-50/70'">
              <label class="flex items-center gap-2 text-sm font-medium mb-2"><svg class="h-4 w-4 text-indigo-500" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.8" d="M7 4.75h10A2.25 2.25 0 0 1 19.25 7v10A2.25 2.25 0 0 1 17 19.25H7A2.25 2.25 0 0 1 4.75 17V7A2.25 2.25 0 0 1 7 4.75ZM8 8.25h8M8 12h8M8 15.75h4" /></svg>Days Before Due Date</label>
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
            <div class="rounded-xl border p-4" :class="isDark ? 'border-gray-700 bg-gray-800/50' : 'border-gray-200 bg-gray-50/70'">
              <label class="flex items-center gap-2 text-sm font-medium mb-2"><svg class="h-4 w-4 text-rose-500" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.8" d="M12 4.75 20 19.25H4L12 4.75ZM12 9v4M12 16.5h.01" /></svg>Disconnection Days</label>
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
            <div class="rounded-xl border p-4" :class="isDark ? 'border-gray-700 bg-gray-800/50' : 'border-gray-200 bg-gray-50/70'">
              <label class="flex items-center gap-2 text-sm font-medium mb-2"><svg class="h-4 w-4 text-amber-500" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.8" d="M12 3.75v16.5M16.25 7.25c0-1.38-1.9-2.5-4.25-2.5s-4.25 1.12-4.25 2.5 1.35 2.1 4.25 2.75 4.25 1.37 4.25 2.75-1.9 2.5-4.25 2.5-4.25-1.12-4.25-2.5" /></svg>Days Before Additional Charge</label>
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
            <div class="rounded-xl border p-4" :class="isDark ? 'border-gray-700 bg-gray-800/50' : 'border-gray-200 bg-gray-50/70'">
              <label class="flex items-center gap-2 text-sm font-medium mb-2"><svg class="h-4 w-4 text-emerald-500" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.8" d="M6.5 5.25h11A2.25 2.25 0 0 1 19.75 7.5v9A2.25 2.25 0 0 1 17.5 18.75h-11A2.25 2.25 0 0 1 4.25 16.5v-9A2.25 2.25 0 0 1 6.5 5.25ZM4.25 9h15.5M8.25 13h.01M11.5 13h4.25" /></svg>Additional Charge</label>
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
            </div></div>
            <div class="rounded-xl border p-4" :class="isDark ? 'border-gray-700 bg-gray-800/50' : 'border-gray-200 bg-gray-50/70'">
              <div class="flex items-center justify-between gap-4">
                <div>
                  <label class="flex items-center gap-2 text-sm font-medium"><svg class="h-4 w-4 text-violet-500" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.8" d="M12 3.75a8.25 8.25 0 1 0 0 16.5 8.25 8.25 0 0 0 0-16.5ZM8.5 12h7" /></svg>Enable penalty charges</label>
                  <p class="mt-1 text-xs" :class="isDark ? 'text-gray-400' : 'text-gray-500'">When disabled, marking a bill as paid will not add an overdue penalty.</p>
                </div>
                <button type="button" role="switch" :aria-checked="form.penalty_enabled" @click="form.penalty_enabled = !form.penalty_enabled" class="relative inline-flex h-7 w-12 shrink-0 items-center rounded-full transition-colors" :class="form.penalty_enabled ? 'bg-indigo-600' : (isDark ? 'bg-gray-600' : 'bg-gray-300')">
                  <span class="inline-block h-5 w-5 transform rounded-full bg-white shadow transition-transform" :class="form.penalty_enabled ? 'translate-x-6' : 'translate-x-1'"></span>
                </button>
              </div>
              <p class="mt-3 text-xs font-medium" :class="form.penalty_enabled ? 'text-green-600' : 'text-amber-600'">{{ form.penalty_enabled ? 'Penalty charges are enabled.' : 'Penalty charges are disabled.' }}</p>
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
                <thead :class="isDark ? 'bg-gray-800 text-gray-300' : 'bg-gray-50 text-gray-600'"><tr><th class="px-3 py-2 text-left"><span class="flex items-center gap-2"><svg class="h-4 w-4 text-indigo-500" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.8" d="M8 11.25a3 3 0 1 0 0-6 3 3 0 0 0 0 6ZM4.75 19.25a3.25 3.25 0 0 1 6.5 0" /></svg>Category</span></th><th class="px-3 py-2 text-left"><span class="flex items-center gap-2"><svg class="h-4 w-4 text-emerald-500" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.8" d="M12 3.75v16.5M16.25 7.25c0-1.38-1.9-2.5-4.25-2.5s-4.25 1.12-4.25 2.5 1.35 2.1 4.25 2.75 4.25 1.37 4.25 2.75-1.9 2.5-4.25 2.5-4.25-1.12-4.25-2.5" /></svg>Rate / m³</span></th><th class="px-3 py-2 text-left"><span class="flex items-center gap-2"><svg class="h-4 w-4 text-amber-500" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.8" d="M5 7.25h14M5 12h14M5 16.75h9" /></svg>Minimum charge</span></th><th class="px-3 py-2 text-left"><span class="flex items-center gap-2"><svg class="h-4 w-4 text-rose-500" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.8" d="M12 4.75v14.5M16 8c0-1.1-1.8-2-4-2s-4 0-4 2 1.8 2 4 2 4 .9 4 2-1.8 2-4 2-4-.9-4-2" /></svg>Late penalty</span></th></tr></thead>
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
                <p class="mt-1 text-xs" :class="isDark ? 'text-gray-400' : 'text-gray-500'">Device sync automatically creates a server-side restore point before changing web data.</p>
              </div>
              <button type="button" :disabled="backupSaving" @click="backupData" class="rounded-lg bg-indigo-600 px-4 py-2 text-sm font-medium text-white hover:bg-indigo-700 disabled:cursor-not-allowed disabled:opacity-50">{{ backupSaving ? 'Preparing...' : 'Backup data' }}</button>
            </div>
            <div class="mt-4 rounded-lg border p-4" :class="isDark ? 'border-gray-700 bg-gray-900' : 'border-gray-200 bg-white'">
              <div class="flex items-center justify-between gap-4">
                <div>
                  <p class="font-medium">Restore points</p>
                  <p class="mt-1 text-xs" :class="isDark ? 'text-gray-400' : 'text-gray-500'">Restore the web database to a snapshot created before a sync.</p>
                </div>
                <button type="button" @click="loadRestorePoints" class="rounded-lg border px-3 py-2 text-sm">Refresh</button>
              </div>
              <div v-if="restorePoints.length" class="mt-3 space-y-2">
                <div v-for="point in restorePoints" :key="point.id" class="flex items-center justify-between gap-3 rounded border p-3 text-sm">
                  <div>
                    <p class="font-medium">{{ point.label }}</p>
                    <p class="text-xs" :class="isDark ? 'text-gray-400' : 'text-gray-500'">{{ formatRestorePointDate(point.created_at) }} · {{ point.row_count }} rows</p>
                  </div>
                  <button type="button" :disabled="restoringPointId === point.id" @click="restorePoint(point)" class="rounded-lg bg-amber-600 px-3 py-2 text-white disabled:opacity-50">{{ restoringPointId === point.id ? 'Restoring...' : 'Restore' }}</button>
                </div>
              </div>
              <p v-else class="mt-3 text-sm" :class="isDark ? 'text-gray-400' : 'text-gray-500'">No restore points yet.</p>
            </div>
            <p v-if="backupMessage" class="mt-3 text-sm" :class="backupError ? 'text-red-600' : 'text-green-600'">{{ backupMessage }}</p>
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
      backupSaving: false,
      backupMessage: '',
      backupError: false,
      restorePoints: [],
      restoringPointId: null,
      errorMessage: '',
      successMessage: '',
      form: {
        bill_due_days: 5,
        disconnection_days: 8,
        penalty_after_days: 0,
        penalty_amount: 0,
        penalty_enabled: true,
      }
    }
  },
  mounted() {
    void this.reload()
    void this.loadCategories()
    void this.loadRestorePoints()
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
          penalty_enabled: d.penalty_enabled !== false,
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
    async backupData() {
      this.backupSaving = true
      this.backupMessage = ''
      this.backupError = false
      try {
        const res = await fetch('/backup/export', { headers: { Accept: 'application/json' }, credentials: 'same-origin' })
        if (!res.ok) throw new Error('Failed to create backup')
        const blob = await res.blob()
        const disposition = res.headers.get('content-disposition') || ''
        const match = disposition.match(/filename="?([^";]+)"?/i)
        const filename = match?.[1] || `watersystem-backup-${new Date().toISOString().slice(0, 10)}.json`
        const url = URL.createObjectURL(blob)
        const link = document.createElement('a')
        link.href = url
        link.download = filename
        document.body.appendChild(link)
        link.click()
        link.remove()
        URL.revokeObjectURL(url)
        this.backupMessage = 'Backup downloaded successfully.'
      } catch (e) {
        this.backupError = true
        this.backupMessage = e?.message || 'Failed to create backup'
      } finally {
        this.backupSaving = false
      }
    },
    async loadRestorePoints() {
      try {
        const res = await fetch('/backup/restore-points', { headers: { Accept: 'application/json' }, credentials: 'same-origin' })
        if (!res.ok) throw new Error('Failed to load restore points')
        const json = await res.json()
        this.restorePoints = Array.isArray(json?.data) ? json.data : []
      } catch (e) {
        this.backupError = true
        this.backupMessage = e?.message || 'Failed to load restore points'
      }
    },
    formatRestorePointDate(value) {
      return value ? new Date(value).toLocaleString() : 'Unknown date'
    },
    async restorePoint(point) {
      if (!window.confirm(`Restore "${point.label}"? Current web data will be replaced.`)) return
      this.restoringPointId = point.id
      this.backupMessage = ''
      this.backupError = false
      try {
        const csrf = document.querySelector('meta[name="csrf-token"]')?.getAttribute('content')
        const res = await fetch(`/backup/restore-points/${point.id}/restore`, {
          method: 'POST',
          headers: { Accept: 'application/json', ...(csrf ? { 'X-CSRF-TOKEN': csrf } : {}) },
          credentials: 'same-origin',
        })
        const json = await res.json()
        if (!res.ok) throw new Error(json?.message || 'Failed to restore point')
        this.backupMessage = 'Restore point restored successfully. Reload the page to see restored data.'
      } catch (e) {
        this.backupError = true
        this.backupMessage = e?.message || 'Failed to restore point'
      } finally {
        this.restoringPointId = null
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

<style scoped>
.settings-section-icon { display: flex; width: 2.5rem; height: 2.5rem; flex-shrink: 0; align-items: center; justify-content: center; border-radius: .75rem; }
</style>
