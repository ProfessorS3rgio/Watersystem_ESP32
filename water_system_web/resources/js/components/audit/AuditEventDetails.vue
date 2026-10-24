<template>
  <div v-if="eventId" class="dmbc-modal fixed inset-0 z-50" :data-dark="isDark" @keydown.esc="$emit('close')">
    <div class="absolute inset-0 bg-black/50" @click="$emit('close')"></div><div class="absolute inset-0 flex items-center justify-center p-4"><section class="dmbc-dialog w-full max-w-4xl" role="dialog" aria-modal="true" aria-labelledby="audit-event-title">
      <header class="dmbc-dialog-header flex items-center justify-between gap-4"><div><p class="mb-1 text-xs font-bold text-teal-600">EVENT #{{ eventId }}</p><h3 id="audit-event-title">{{ log ? auditLabel(log.action) : 'Event details' }}</h3></div><button @click="$emit('close')" aria-label="Close event details">Close</button></header>
      <div v-if="loading" role="status" class="p-8 text-sm audit-muted">Loading event details…</div><div v-else-if="error" role="alert" class="p-6"><p class="text-red-500 text-sm">{{ error }}</p><button @click="load" class="audit-secondary mt-4">Try again</button></div>
      <div v-else-if="log" class="space-y-6 p-5 sm:p-6">
        <dl class="grid gap-3 sm:grid-cols-3"><div class="audit-soft rounded-xl p-4"><dt class="audit-muted text-xs">Recorded · Philippine time</dt><dd class="mt-2 text-sm font-semibold">{{ auditDate(log.created_at) }}</dd></div><div class="audit-soft rounded-xl p-4"><dt class="audit-muted text-xs">Performed by</dt><dd class="mt-2 text-sm font-semibold">{{ log.actor_name }}</dd><dd class="mt-1 text-xs audit-muted">{{ auditLabel(log.actor_type) }}{{ log.actor_id ? ' · ID ' + log.actor_id : '' }}</dd></div><div class="audit-soft rounded-xl p-4"><dt class="audit-muted text-xs">Affected record</dt><dd class="mt-2 text-sm font-semibold">{{ auditType(log.auditable_type) }} #{{ log.auditable_id ?? '—' }}</dd><dd class="mt-1 break-words text-xs audit-muted">{{ log.subject_label || log.subject_name }}</dd></div></dl>
        <section><h4 class="mb-3 font-bold">Recorded changes</h4><div v-if="changes.length" class="overflow-x-auto rounded-xl border border-inherit"><table class="w-full text-left text-sm"><thead class="audit-soft"><tr><th class="p-3">Field</th><th class="p-3">Before</th><th class="p-3">After</th></tr></thead><tbody class="divide-y audit-divide"><tr v-for="change in changes" :key="change.key"><th class="p-3 align-top text-xs font-semibold">{{ auditLabel(change.key) }}</th><td class="max-w-sm p-3 align-top"><pre class="whitespace-pre-wrap break-words text-xs audit-muted">{{ auditValue(change.before) }}</pre></td><td class="max-w-sm p-3 align-top"><pre class="whitespace-pre-wrap break-words text-xs">{{ auditValue(change.after) }}</pre></td></tr></tbody></table></div><p v-else class="text-sm audit-muted">No field differences were captured for this event. Check its recorded context below.</p></section>
        <section v-if="rows.length"><h4 class="mb-3 font-bold">Applied import rows</h4><div class="max-h-64 overflow-auto rounded-xl border border-inherit"><pre v-for="(row, index) in rows.slice(0, rowLimit)" :key="index" class="border-b border-inherit p-3 text-xs whitespace-pre-wrap break-words">{{ auditValue(row) }}</pre></div><button v-if="rows.length > rowLimit" class="audit-secondary mt-3" @click="rowLimit += 50">Show more rows ({{ Math.min(rowLimit, rows.length) }} / {{ rows.length }})</button></section>
        <section><h4 class="mb-3 font-bold">Event context</h4><div class="space-y-3"><div v-if="log.metadata.sync_run_id || log.metadata.request_id" class="flex flex-wrap gap-3"><button v-if="log.metadata.sync_run_id" class="audit-secondary" @click="$emit('related', { key: 'sync_run_id', value: log.metadata.sync_run_id })">View this sync run’s events →</button><button v-if="log.metadata.request_id" class="audit-secondary" @click="$emit('related', { key: 'request_id', value: log.metadata.request_id })">View related request events →</button></div><p v-if="log.device_uid" class="break-all text-xs audit-muted">Device: {{ log.device_uid }}</p><p v-if="log.ip_address" class="text-xs audit-muted">IP address: {{ log.ip_address }}</p><details class="audit-soft rounded-xl p-4"><summary class="cursor-pointer text-sm font-semibold">Full recorded context</summary><pre class="mt-4 max-h-64 overflow-auto whitespace-pre-wrap break-words text-xs">{{ auditValue(context) }}</pre></details></div></section>
      </div>
    </section></div>
  </div>
</template>

<script setup>
import { ref, computed, watch, onBeforeUnmount } from 'vue'
import { auditLabel, auditType, auditDate, auditValue, auditChanges } from '../../utils/auditPresentation.js'
const props = defineProps({ eventId: Number, isDark: Boolean })
defineEmits(['close', 'related'])
const log = ref(null), loading = ref(false), error = ref(''), rowLimit = ref(50)
let controller, revision = 0
const changes = computed(() => log.value ? auditChanges(log.value) : [])
const rows = computed(() => Array.isArray(log.value?.metadata.imported_rows) ? log.value.metadata.imported_rows : [])
const context = computed(() => { const { imported_rows, ...rest } = log.value?.metadata || {}; return rest })
async function load() {
  controller?.abort(); const current = ++revision
  if (!props.eventId) return
  controller = new AbortController(); loading.value = true; error.value = ''; log.value = null; rowLimit.value = 50
  try {
    const response = await fetch('/audit-logs/' + props.eventId, { headers: { Accept: 'application/json' }, credentials: 'same-origin', signal: controller.signal })
    if (!response.ok) throw new Error(response.status === 404 ? 'This event is no longer available.' : 'Unable to load the audit event.')
    const data = await response.json()
    if (current === revision) log.value = data.data
  } catch (err) { if (current === revision && err.name !== 'AbortError') error.value = err.message } finally { if (current === revision) loading.value = false }
}
watch(() => props.eventId, load, { immediate: true })
onBeforeUnmount(() => { revision++; controller?.abort() })
</script>
