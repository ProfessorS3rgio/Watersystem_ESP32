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
          <table class="w-full text-sm"><thead :class="isDark ? 'bg-gray-700 text-gray-200' : 'bg-gray-50 text-gray-600'"><tr><th class="px-5 py-3 text-left">Date</th><th class="px-5 py-3 text-left">Action</th><th class="px-5 py-3 text-left">Record</th><th class="px-5 py-3 text-left">Source</th><th class="px-5 py-3 text-left">Details</th></tr></thead>
            <tbody class="divide-y" :class="isDark ? 'divide-gray-700' : 'divide-gray-200'"><tr v-for="log in logs" :key="log.id"><td class="whitespace-nowrap px-5 py-4" :class="isDark ? 'text-gray-300' : 'text-gray-600'">{{ formatDate(log.created_at) }}</td><td class="px-5 py-4 font-semibold">{{ label(log.action) }}</td><td class="px-5 py-4">{{ shortType(log.auditable_type) }} #{{ log.auditable_id || '—' }}</td><td class="px-5 py-4"><span class="rounded-full px-2 py-1 text-xs" :class="log.actor_type === 'user' ? 'bg-indigo-100 text-indigo-700' : 'bg-gray-100 text-gray-700'">{{ log.actor_type }}</span></td><td class="max-w-sm px-5 py-4 text-xs" :class="isDark ? 'text-gray-400' : 'text-gray-600'">{{ detail(log) }}</td></tr></tbody>
          </table>
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
  data: () => ({ logs: [], loading: false, error: '' }),
  mounted() { this.load() },
  methods: {
    async load() { this.loading = true; try { const res = await fetch('/audit-logs', { headers: { Accept: 'application/json' }, credentials: 'same-origin' }); if (!res.ok) throw new Error('Failed to load audit logs'); const json = await res.json(); this.logs = json.data || [] } catch (e) { this.error = e.message } finally { this.loading = false } },
    label(value) { return String(value || '').replace(/_/g, ' ').replace(/\b\w/g, c => c.toUpperCase()) },
    shortType(value) { return String(value || '').split('\\').pop() },
    formatDate(value) { return value ? new Date(value).toLocaleString('en-US', { year: 'numeric', month: '2-digit', day: '2-digit', hour: '2-digit', minute: '2-digit' }) : '—' },
    detail(log) { const data = log.metadata || {}; return Object.entries(data).map(([key, value]) => `${key}: ${value}`).join(' · ') || '—' }
  }
}
</script>
