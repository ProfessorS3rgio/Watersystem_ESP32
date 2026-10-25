<template>
  <Teleport to="body"><div class="incident-workspace incident-overlay dmbc-modal" :data-dark="isDark" :class="{ dark: isDark }" @click.self="!busy && $emit('close')">
    <section ref="dialog" tabindex="-1" role="dialog" aria-modal="true" aria-labelledby="incident-detail-title" class="dmbc-dialog w-full max-w-6xl" @keydown.esc.stop="!busy && $emit('close')">
      <header class="dmbc-dialog-header flex items-start justify-between gap-4"><div><p class="incident-eyebrow">{{ incident?.reference_number || 'DMBC Water · Service care' }}</p><h2 id="incident-detail-title" class="mt-1 text-xl font-extrabold">{{ incident?.title || 'Incident details' }}</h2><div v-if="incident" class="mt-3 flex flex-wrap gap-2"><span class="incident-badge" :class="statusColor(incident.status)">{{ options.statuses[incident.status] }}</span><span class="incident-badge" :class="priorityColor(incident.priority)">{{ options.priorities[incident.priority] }} priority</span></div></div><button :disabled="busy" @click="$emit('close')" aria-label="Close incident details">✕</button></header>
      <div v-if="loading" role="status" class="p-10 text-center incident-muted">Loading the incident and its history…</div>
      <div v-else-if="!incident" class="p-8"><p role="alert" class="text-sm text-rose-500">{{ error }}</p><button class="incident-secondary mt-4" @click="load">Try again</button></div>
      <div v-else class="grid gap-7 p-5 sm:p-7 lg:grid-cols-[1.15fr_1fr]">
        <div class="space-y-6">
          <section class="incident-soft rounded-2xl p-5"><p class="incident-eyebrow">Reported for</p><h3 class="mt-2 text-lg font-bold">{{ incident.customer_name }}</h3><p class="mt-1 text-sm font-semibold text-teal-600">{{ incident.account_no }}<span v-if="incident.barangay"> · {{ incident.barangay }}</span></p><p class="mt-2 text-sm incident-muted">{{ incident.customer_address || 'No address recorded' }}</p><p v-if="!incident.customer" class="mt-3 text-xs text-amber-600">This account is no longer linked. The original customer details and repair history remain available.</p><p v-else-if="incident.customer.account_no !== incident.account_no" class="mt-3 text-xs incident-muted">Current account number: {{ incident.customer.account_no }}. This report retains the number recorded when it was created.</p><p class="mt-4 text-xs incident-muted">Reported {{ auditDate(incident.created_at) }} · {{ incident.reported_by_name }}</p></section>
          <section><h3 class="font-bold">Issue &amp; location</h3><p class="mt-3 whitespace-pre-wrap break-words text-sm leading-6">{{ incident.description }}</p><div class="mt-4 grid gap-3 sm:grid-cols-2"><div class="incident-outline rounded-xl p-4"><p class="text-xs incident-muted">Location / landmark</p><p class="mt-2 whitespace-pre-wrap break-words text-sm font-semibold">{{ incident.location }}</p></div><div class="incident-outline rounded-xl p-4"><p class="text-xs incident-muted">Line section</p><p class="mt-2 text-sm font-semibold">{{ options.sections[incident.pipe_section] }}</p><p class="mt-3 text-xs incident-muted">Crew: {{ incident.assigned_to || 'Not assigned' }}</p></div></div></section>
          <section v-if="incident.resolution" class="rounded-2xl border border-teal-500/25 bg-teal-500/5 p-5"><h3 class="font-bold text-teal-600">Repair outcome</h3><p class="mt-2 whitespace-pre-wrap break-words text-sm leading-6">{{ incident.resolution }}</p><p class="mt-3 text-xs incident-muted">Resolved {{ auditDate(incident.resolved_at) }}<span v-if="incident.closed_at"> · Closed {{ auditDate(incident.closed_at) }}</span></p></section>
          <section v-if="incident.other_open_incidents?.length" class="incident-soft rounded-2xl p-4"><h3 class="text-sm font-bold">Other open reports for this account</h3><button v-for="other in incident.other_open_incidents" :key="other.id" @click="$emit('select', other.id)" class="mt-3 block text-left text-sm text-teal-600 hover:underline">{{ other.reference_number }} · {{ other.title }} →</button></section>
          <section><div class="mb-4 flex items-center justify-between"><h3 class="font-bold">Activity &amp; repair history</h3><span class="text-xs incident-muted">{{ incident.events_count }} entries</span></div><ol class="space-y-4"><li v-for="event in incident.events" :key="event.id" class="incident-outline relative rounded-2xl p-4"><p class="text-xs incident-muted">{{ auditDate(event.created_at) }} · {{ event.actor_name }}</p><p class="mt-2 text-sm font-bold">{{ eventLabel(event.kind) }}<span v-if="event.from_status !== event.to_status"> · {{ options.statuses[event.from_status] }} → {{ options.statuses[event.to_status] }}</span></p><p v-if="event.message" class="mt-2 whitespace-pre-wrap break-words text-sm leading-6">{{ event.message }}</p><details v-if="event.changes && Object.keys(event.changes).length" class="mt-3 text-xs"><summary class="cursor-pointer font-semibold text-teal-600">View recorded changes</summary><dl class="mt-3 space-y-3"><div v-for="(change, field) in event.changes" :key="field"><dt class="font-bold">{{ incidentField(field) }}</dt><dd class="mt-1 whitespace-pre-wrap break-words incident-muted">{{ displayChange(field, change.before) }} → {{ displayChange(field, change.after) }}</dd></div></dl></details></li></ol><p v-if="historyError" role="alert" class="mt-3 text-sm text-rose-500">{{ historyError }}</p><button v-if="incident.more_history_before" class="incident-secondary mt-4" :disabled="loadingHistory || busy" @click="olderHistory">{{ loadingHistory ? 'Loading history…' : 'Show older activity' }}</button></section>
        </div>
        <form @submit.prevent="save" class="incident-soft h-fit space-y-4 rounded-2xl p-5 lg:sticky lg:top-3"><div><p class="incident-eyebrow">Next action</p><h3 class="mt-2 text-lg font-bold">Update this incident</h3><p class="mt-2 text-xs leading-5 incident-muted">Every saved update records who changed it and when. Mark a repair resolved, then close it after confirming the outcome.</p></div>
          <label class="block">Status<select v-model="form.status" :disabled="busy" class="mt-2 w-full"><option :value="incident.status">{{ options.statuses[incident.status] }}</option><option v-for="status in incident.allowed_statuses" :key="status" :value="status">{{ options.statuses[status] }}{{ terminal && ['reported', 'in_progress'].includes(status) ? ' · Reopen' : '' }}</option></select></label>
          <template v-if="!terminal || reopening"><div class="grid gap-4 sm:grid-cols-2 lg:grid-cols-1 xl:grid-cols-2"><label>Priority<select v-model="form.priority" :disabled="busy" class="mt-2 w-full"><option v-for="(label, key) in options.priorities" :key="key" :value="key">{{ label }}</option></select></label><label>Issue type<select v-model="form.category" :disabled="busy" class="mt-2 w-full"><option v-for="(label, key) in options.categories" :key="key" :value="key">{{ label }}</option></select></label></div><label class="block">Assigned crew<input v-model.trim="form.assigned_to" maxlength="160" :disabled="busy" placeholder="Technician or repair team" class="mt-2 w-full"/></label><label class="block">Line section<select v-model="form.pipe_section" :disabled="busy" class="mt-2 w-full"><option v-for="(label, key) in options.sections" :key="key" :value="key">{{ label }}</option></select></label><details><summary class="cursor-pointer text-sm font-semibold text-teal-600">Edit report details</summary><div class="mt-4 space-y-4"><label class="block">Title<input v-model.trim="form.title" required maxlength="180" :disabled="busy" class="mt-2 w-full"/></label><label class="block">Description<textarea v-model.trim="form.description" required minlength="5" maxlength="5000" rows="3" :disabled="busy" class="mt-2 w-full"/></label><label class="block">Location<textarea v-model.trim="form.location" required maxlength="1000" rows="2" :disabled="busy" class="mt-2 w-full"/></label></div></details></template>
          <p v-else class="rounded-xl border border-teal-500/20 p-3 text-xs incident-muted">Report details are retained as recorded. Reopen the incident if more repair work is needed.</p>
          <label v-if="form.status === 'resolved' && incident.status !== 'resolved'" class="block">Repair &amp; verification notes<textarea v-model.trim="form.resolution" required minlength="5" maxlength="5000" rows="4" :disabled="busy" placeholder="What was repaired, and how was the result checked?" class="mt-2 w-full"/></label>
          <label class="block">{{ reasonRequired ? 'Reason for this change' : 'Progress note (optional)' }}<textarea v-model.trim="form.note" :required="reasonRequired" minlength="3" maxlength="5000" rows="3" :disabled="busy" placeholder="Inspection findings, next steps, or why the report needs more work…" class="mt-2 w-full"/></label>
          <p v-if="error" role="alert" class="rounded-xl bg-rose-500/10 p-3 text-sm text-rose-600">{{ error }}</p><button v-if="conflict" type="button" class="incident-secondary w-full" @click="load">Reload saved report and discard draft</button>
          <div class="flex items-center justify-between gap-3 border-t border-inherit pt-4"><span class="text-xs incident-muted">{{ dirty ? 'Unsaved update' : 'All changes recorded' }}</span><button class="incident-primary" :disabled="busy || !dirty || conflict">{{ busy ? 'Saving…' : 'Save update' }}</button></div>
        </form>
      </div>
    </section>
  </div></Teleport>
</template>

<script setup>
import { ref, reactive, computed, watch, onBeforeUnmount } from 'vue';
import { incidentApi } from '../../services/incidentApi.js';
import { auditDate } from '../../utils/auditPresentation.js';
import { statusColor, priorityColor, incidentField, eventLabel } from '../../utils/incidentPresentation.js';
import { useIncidentDialog } from '../../composables/useIncidentDialog.js';
const props = defineProps({ incidentId: Number, isDark: Boolean });
const emit = defineEmits(['close', 'saved', 'select']);
const dialog = ref(null); useIncidentDialog(dialog);
const incident = ref(null), options = ref({ statuses: {}, priorities: {}, categories: {}, sections: {} });
const loading = ref(true), busy = ref(false), error = ref(''), conflict = ref(false), baseline = ref('');
const loadingHistory = ref(false), historyError = ref('');
const form = reactive({});
const fields = ['title', 'description', 'location', 'category', 'priority', 'pipe_section', 'assigned_to'];
const dirty = computed(() => Boolean(incident.value) && JSON.stringify(form) !== baseline.value);
const terminal = computed(() => incident.value && ['resolved', 'closed', 'cancelled'].includes(incident.value.status));
const reopening = computed(() => terminal.value && ['reported', 'in_progress'].includes(form.status));
const reasonRequired = computed(() => reopening.value || form.status !== incident.value?.status && (form.status === 'cancelled' || form.status === 'reported'));
let controller, revision = 0;
function receive(data) {
  loadingHistory.value = false; historyError.value = '';
  incident.value = data.data; options.value = data.options;
  for (const key of Object.keys(form)) delete form[key];
  for (const field of fields) form[field] = incident.value[field] ?? '';
  form.status = incident.value.status; form.note = ''; form.resolution = '';
  baseline.value = JSON.stringify(form);
}
async function load() {
  controller?.abort(); controller = new AbortController(); const current = ++revision;
  loading.value = true; error.value = ''; conflict.value = false;
  try { const data = await incidentApi.detail(props.incidentId, controller.signal); if (current === revision) receive(data); }
  catch (err) { if (current === revision && err.name !== 'AbortError') error.value = err.message; }
  finally { if (current === revision) loading.value = false; }
}
function displayChange(field, value) { if (value == null || value === '') return '—'; if (field === 'status') return options.value.statuses[value] || value; if (field === 'priority') return options.value.priorities[value] || value; if (field === 'category') return options.value.categories[value] || value; if (field === 'pipe_section') return options.value.sections[value] || value; return value; }
async function olderHistory() {
  if (loadingHistory.value || !incident.value?.more_history_before || busy.value) return;
  const current = revision; loadingHistory.value = true; historyError.value = '';
  try {
    const result = await incidentApi.history(incident.value.id, incident.value.more_history_before, controller.signal);
    if (current === revision) { incident.value.events.push(...result.data); incident.value.more_history_before = result.before_id; }
  } catch (err) { if (current === revision && err.name !== 'AbortError') historyError.value = err.message; }
  finally { if (current === revision) loadingHistory.value = false; }
}
async function save() {
  if (busy.value || !dirty.value || conflict.value) return;
  busy.value = true; error.value = '';
  const payload = { version: incident.value.version, status: form.status, note: form.note || null };
  if (!terminal.value || reopening.value) for (const field of fields) payload[field] = field === 'assigned_to' ? form[field] || null : form[field];
  if (form.status === 'resolved' && incident.value.status !== 'resolved') payload.resolution = form.resolution;
  try { const result = await incidentApi.update(incident.value.id, payload); controller?.abort(); controller = new AbortController(); ++revision; receive(result); emit('saved', result.data); }
  catch (err) { error.value = err.message; conflict.value = err.status === 409; }
  finally { busy.value = false; }
}
watch(() => props.incidentId, () => { incident.value = null; load(); }, { immediate: true });
onBeforeUnmount(() => { controller?.abort(); ++revision; });
defineExpose({ dirty, busy });
</script>
