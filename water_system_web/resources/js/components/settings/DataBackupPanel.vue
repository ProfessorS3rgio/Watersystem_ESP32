<template>
  <section class="space-y-6">
    <div class="settings-panel overflow-hidden"><header class="settings-panel-heading"><p class="settings-eyebrow">Data protection</p><h2>Backup &amp; restore</h2><p class="mt-2 text-sm settings-muted">Keep an archive of your association’s data and recover from saved restore points.</p></header>
      <div class="grid gap-5 p-5 sm:p-6 md:grid-cols-2"><article class="settings-field-card"><span class="mb-4 flex h-11 w-11 items-center justify-center rounded-xl bg-cyan-500/10 text-cyan-600"><svg class="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="1.7" aria-hidden="true"><path d="M12 3v12m0 0 4-4m-4 4-4-4M4 15v5h16v-5"/></svg></span><h3 class="font-bold">Download an archive</h3><p class="mt-2 text-sm leading-6 settings-muted">Export customers, readings, bills, payments, and settings as a ZIP archive containing JSON data and an SQL dump.</p><button @click="downloadBackup" :disabled="busy" class="settings-primary mt-5">{{ exporting ? 'Preparing download…' : 'Download backup' }}</button></article>
      <article class="settings-field-card"><span class="mb-4 flex h-11 w-11 items-center justify-center rounded-xl bg-teal-500/10 text-teal-600"><svg class="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="1.7" aria-hidden="true"><path d="M12 8v5l3 2M3 12a9 9 0 1 0 3-6.7M3 3v5h5"/></svg></span><h3 class="font-bold">Create a restore point</h3><p class="mt-2 text-sm leading-6 settings-muted">Save a snapshot on the web server before making changes. Device sync also creates a restore point automatically.</p><button @click="createPoint" :disabled="busy" class="settings-secondary mt-5">{{ creating ? 'Creating snapshot…' : 'Create restore point' }}</button></article></div>
      <p v-if="message" role="status" class="settings-feedback-success mx-6 mb-6">{{ message }}</p><p v-if="error" role="alert" class="settings-feedback-error mx-6 mb-6">{{ error }}</p>
    </div>
    <div class="settings-panel overflow-hidden"><header class="settings-panel-heading flex flex-wrap items-center justify-between gap-3"><div><h2>Saved restore points</h2><p class="mt-2 text-sm settings-muted">The latest {{ points.length }} snapshots stored on this web server.</p></div><button @click="loadPoints" :disabled="loading || busy" class="settings-secondary">{{ loading ? 'Refreshing…' : 'Refresh list' }}</button></header>
      <div v-if="loading" role="status" class="p-8 text-sm settings-muted">Loading restore points…</div><div v-else-if="listError" role="alert" class="settings-feedback-error m-6">{{ listError }}</div>
      <div v-else-if="!points.length" class="px-6 py-12 text-center"><h3 class="font-bold">Your first restore point starts here</h3><p class="mt-2 text-sm settings-muted">Create a snapshot above or complete a device sync to build your recovery history.</p></div>
      <ul v-else class="divide-y settings-divide"><li v-for="point in points" :key="point.id" class="flex flex-col gap-4 px-6 py-5 sm:flex-row sm:items-center sm:justify-between"><div class="min-w-0"><div class="flex flex-wrap items-center gap-2"><h3 class="break-words text-sm font-bold">{{ point.label }}</h3><span class="settings-soft rounded-full px-2.5 py-1 text-[10px] font-bold uppercase tracking-wider text-teal-600">{{ point.source === 'manual' ? 'Manual' : point.source === 'sync' ? 'Before sync' : point.source }}</span></div><p class="mt-2 text-xs settings-muted">{{ formatDate(point.created_at) }} · {{ Number(point.row_count).toLocaleString() }} records · {{ point.table_count }} tables</p></div><button @click="selectedPoint = point" :disabled="busy" class="settings-secondary shrink-0 text-amber-600">{{ restoring === point.id ? 'Restoring…' : 'Restore snapshot' }}</button></li></ul>
    </div>
    <ConfirmDialog :is-open="Boolean(selectedPoint)" :is-dark="isDark" title="Restore web data?" message="This replaces current web records with the selected snapshot. Any unsaved Settings edits will be discarded. The ESP32 device is not restored by this action." confirm-text="Restore snapshot" :is-loading="restoring !== null" @confirm="restoreSelected" @cancel="!busy && (selectedPoint = null)" />
  </section>
</template>

<script setup>
import { ref, computed, watch, onMounted } from 'vue'
import ConfirmDialog from '../ConfirmDialog.vue'
import { settingsRequest, settingsJson } from '../../services/settingsApi.js'
defineProps({ isDark: Boolean })
const emit = defineEmits(['state-change', 'restored'])
const points = ref([]), loading = ref(false), listError = ref(''), message = ref(''), error = ref('')
const exporting = ref(false), creating = ref(false), restoring = ref(null), selectedPoint = ref(null)
const busy = computed(() => exporting.value || creating.value || restoring.value !== null)
watch(busy, value => emit('state-change', { section: 'backup', dirty: false, busy: value }), { immediate: true })
async function loadPoints() {
  if (loading.value) return
  loading.value = true
  listError.value = ''
  try { points.value = (await settingsJson('/backup/restore-points')).data || [] } catch (err) { listError.value = err.message } finally { loading.value = false }
}
async function downloadBackup() {
  if (busy.value) return
  exporting.value = true; error.value = ''; message.value = ''
  let url
  try {
    const response = await settingsRequest('/backup/export')
    const blob = await response.blob()
    const disposition = response.headers.get('content-disposition') || ''
    const filename = disposition.match(/filename="?([^";]+)"?/i)?.[1] || 'dmbc-backup.zip'
    url = URL.createObjectURL(blob)
    const link = document.createElement('a')
    link.href = url; link.download = filename
    document.body.appendChild(link); link.click(); link.remove()
    message.value = 'Backup archive prepared. Check your browser’s downloads.'
  } catch (err) { error.value = err.message } finally { if (url) URL.revokeObjectURL(url); exporting.value = false }
}
async function createPoint() {
  if (busy.value) return
  creating.value = true; error.value = ''; message.value = ''
  try {
    await settingsJson('/backup/restore-points', { method: 'POST', body: JSON.stringify({ label: 'Manual DMBC restore point', source: 'manual' }) })
    message.value = 'Restore point created successfully.'
    await loadPoints()
  } catch (err) { error.value = err.message } finally { creating.value = false }
}
async function restoreSelected() {
  if (busy.value || !selectedPoint.value) return
  restoring.value = selectedPoint.value.id; error.value = ''; message.value = ''
  try {
    await settingsJson('/backup/restore-points/' + selectedPoint.value.id + '/restore', { method: 'POST' })
    selectedPoint.value = null
    restoring.value = null
    emit('restored')
  } catch (err) { error.value = err.message; selectedPoint.value = null } finally { restoring.value = null }
}
function formatDate(value) {
  const date = new Date(value)
  return Number.isNaN(date.getTime()) ? 'Unknown date' : date.toLocaleString('en-PH', { dateStyle: 'medium', timeStyle: 'short', timeZone: 'Asia/Manila' })
}
onMounted(loadPoints)
</script>
