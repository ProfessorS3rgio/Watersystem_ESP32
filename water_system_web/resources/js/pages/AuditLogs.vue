<template>
  <AppSidebar v-slot="{ isDark }">
    <div class="p-8">
      <div class="mb-8">
        <p class="text-xs font-semibold uppercase tracking-wider text-indigo-500">System history</p>
        <h1 class="mt-2 text-3xl font-bold" :class="isDark ? 'text-white' : 'text-gray-900'">Audit Logs</h1>
        <p class="mt-2" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Review changes, payments, and bill actions recorded by the system.</p>
      </div>
      <div class="overflow-hidden rounded-xl border shadow-sm" :class="isDark ? 'border-gray-700 bg-gray-800' : 'border-gray-200 bg-white'">
        <div v-if="loading" class="p-8 text-sm">Loading audit logs...</div>
        <div v-else-if="error" class="p-8 text-sm text-red-600">{{ error }}</div>
        <div v-else-if="logs.length === 0" class="p-8 text-sm" :class="isDark ? 'text-gray-300' : 'text-gray-600'">No audit activity yet.</div>
        <div v-else class="overflow-x-auto">
          <div
            v-if="pagination.last_page > 1"
            class="flex flex-col gap-3 border-b px-5 py-4 text-sm sm:flex-row sm:items-center sm:justify-between"
            :class="isDark ? 'border-gray-700 text-gray-300' : 'border-gray-200 text-gray-600'"
          >
            <p>Showing {{ pagination.from }}–{{ pagination.to }} of {{ pagination.total }} logs</p>
            <nav class="flex items-center gap-1" aria-label="Audit log pages">
              <button
                type="button"
                class="rounded-md border px-3 py-1.5 disabled:cursor-not-allowed disabled:opacity-40"
                :class="isDark ? 'border-gray-600 hover:bg-gray-700' : 'border-gray-300 hover:bg-gray-100'"
                :disabled="pagination.current_page === 1 || loading"
                @click="load(pagination.current_page - 1)"
              >
                Previous
              </button>
              <button
                v-for="page in pageNumbers"
                :key="`top-${page}`"
                type="button"
                class="min-w-8 rounded-md border px-2 py-1.5"
                :class="page === pagination.current_page
                  ? 'border-indigo-600 bg-indigo-600 text-white'
                  : (isDark ? 'border-gray-600 hover:bg-gray-700' : 'border-gray-300 hover:bg-gray-100')"
                :disabled="loading"
                @click="load(page)"
              >
                {{ page }}
              </button>
              <button
                type="button"
                class="rounded-md border px-3 py-1.5 disabled:cursor-not-allowed disabled:opacity-40"
                :class="isDark ? 'border-gray-600 hover:bg-gray-700' : 'border-gray-300 hover:bg-gray-100'"
                :disabled="pagination.current_page === pagination.last_page || loading"
                @click="load(pagination.current_page + 1)"
              >
                Next
              </button>
            </nav>
          </div>
          <table class="w-full text-sm"><thead :class="isDark ? 'bg-gray-700 text-gray-200' : 'bg-gray-50 text-gray-600'"><tr><th class="px-5 py-3 text-left">Date</th><th class="px-5 py-3 text-left">Action</th><th class="px-5 py-3 text-left">Record</th><th class="px-5 py-3 text-left">Source</th><th class="px-5 py-3 text-left">Details</th></tr></thead>
            <tbody class="divide-y" :class="isDark ? 'divide-gray-700' : 'divide-gray-200'"><tr v-for="log in logs" :key="log.id"><td class="whitespace-nowrap px-5 py-4" :class="isDark ? 'text-gray-300' : 'text-gray-600'">{{ formatDate(log.created_at) }}</td><td class="px-5 py-4 font-semibold">{{ label(log.action) }}</td><td class="px-5 py-4">{{ shortType(log.auditable_type) }} #{{ log.auditable_id || '—' }}</td><td class="px-5 py-4"><span class="rounded-full px-2 py-1 text-xs" :class="log.actor_type === 'user' ? 'bg-indigo-100 text-indigo-700' : 'bg-gray-100 text-gray-700'">{{ log.actor_type }}</span></td><td class="max-w-sm px-5 py-4 text-xs" :class="isDark ? 'text-gray-400' : 'text-gray-600'"><p>{{ detail(log) }}</p><details v-if="importedRows(log).length" class="mt-2"><summary class="cursor-pointer font-medium" :class="isDark ? 'text-indigo-300' : 'text-indigo-700'">View {{ importedRows(log).length }} applied row{{ importedRows(log).length === 1 ? '' : 's' }}</summary><ul class="mt-2 space-y-1"><li v-for="row in importedRows(log)" :key="`${row.account_no}-${row.source_row}`">{{ importedRowDetail(row) }}</li></ul></details></td></tr></tbody>
          </table>
        </div>
        <div
          v-if="pagination.last_page > 1"
          class="flex flex-col gap-3 border-t px-5 py-4 text-sm sm:flex-row sm:items-center sm:justify-between"
          :class="isDark ? 'border-gray-700 text-gray-300' : 'border-gray-200 text-gray-600'"
        >
          <p>
            Showing {{ pagination.from }}–{{ pagination.to }} of {{ pagination.total }} logs
          </p>
          <nav class="flex items-center gap-1" aria-label="Audit log pages">
            <button
              type="button"
              class="rounded-md border px-3 py-1.5 disabled:cursor-not-allowed disabled:opacity-40"
              :class="isDark ? 'border-gray-600 hover:bg-gray-700' : 'border-gray-300 hover:bg-gray-100'"
              :disabled="pagination.current_page === 1 || loading"
              @click="load(pagination.current_page - 1)"
            >
              Previous
            </button>
            <button
              v-for="page in pageNumbers"
              :key="page"
              type="button"
              class="min-w-8 rounded-md border px-2 py-1.5"
              :class="page === pagination.current_page
                ? 'border-indigo-600 bg-indigo-600 text-white'
                : (isDark ? 'border-gray-600 hover:bg-gray-700' : 'border-gray-300 hover:bg-gray-100')"
              :disabled="loading"
              @click="load(page)"
            >
              {{ page }}
            </button>
            <button
              type="button"
              class="rounded-md border px-3 py-1.5 disabled:cursor-not-allowed disabled:opacity-40"
              :class="isDark ? 'border-gray-600 hover:bg-gray-700' : 'border-gray-300 hover:bg-gray-100'"
              :disabled="pagination.current_page === pagination.last_page || loading"
              @click="load(pagination.current_page + 1)"
            >
              Next
            </button>
          </nav>
        </div>
      </div>
    </div>
  </AppSidebar>
</template>

<script>
import AppSidebar from '../components/AppSidebar.vue'
export default {
  name: 'AuditLogs',
  components: { AppSidebar },
  data: () => ({
    logs: [],
    loading: false,
    error: '',
    pagination: {
      current_page: 1,
      last_page: 1,
      from: 0,
      to: 0,
      total: 0,
    },
  }),
  computed: {
    pageNumbers() {
      const current = this.pagination.current_page
      const last = this.pagination.last_page
      const start = Math.max(1, Math.min(current - 2, last - 4))
      const end = Math.min(last, start + 4)
      return Array.from({ length: end - start + 1 }, (_, index) => start + index)
    },
  },
  mounted() { this.load() },
  methods: {
    async load(page = 1) {
      this.loading = true
      this.error = ''
      try {
        const res = await fetch(`/audit-logs?page=${page}`, {
          headers: { Accept: 'application/json' },
          credentials: 'same-origin',
        })
        if (!res.ok) throw new Error('Failed to load audit logs')
        const json = await res.json()
        this.logs = json.data || []
        this.pagination = {
          current_page: json.current_page || 1,
          last_page: json.last_page || 1,
          from: json.from || 0,
          to: json.to || 0,
          total: json.total || 0,
        }
      } catch (e) {
        this.error = e.message
      } finally {
        this.loading = false
      }
    },
    label(value) { return String(value || '').replace(/_/g, ' ').replace(/\b\w/g, c => c.toUpperCase()) },
    shortType(value) { return String(value || '').split('\\').pop() },
    formatDate(value) { return value ? new Date(value).toLocaleString('en-US', { year: 'numeric', month: '2-digit', day: '2-digit', hour: '2-digit', minute: '2-digit' }) : '—' },
    detail(log) { const data = log.metadata || {}; if (log.action === 'meter_readings_imported') { const worksheets = Array.isArray(data.worksheets) ? data.worksheets.join(', ') : '—'; const month = data.billing_month || 'Not specified'; return `File: ${data.source_file || '—'} · Sheet: ${worksheets} · Barangay: ${data.barangay || '—'} · Month: ${month} · Applied: ${data.applied_rows || 0} · Readings updated: ${log.after?.updated || 0} · Readings created: ${log.after?.created || 0} · Bills updated: ${log.after?.bill_statuses_updated || 0}` } return Object.entries(data).filter(([, value]) => typeof value !== 'object').map(([key, value]) => `${key}: ${value}`).join(' · ') || '—' },
    importedRows(log) { return Array.isArray(log.metadata?.imported_rows) ? log.metadata.imported_rows : [] },
    importedRowDetail(row) { const reading = row.reading ? `${row.reading.operation} reading ${row.reading.previous_from ?? '—'} / ${row.reading.present_from ?? '—'} to ${row.reading.previous_to} / ${row.reading.present_to}` : ''; const bill = row.bill_status ? `bill ${row.bill_status.from} to ${row.bill_status.to}` : ''; return `Row ${row.source_row}: ${row.account_no}${reading || bill ? ` · ${[reading, bill].filter(Boolean).join('; ')}` : ''}` }
  }
}
</script>
