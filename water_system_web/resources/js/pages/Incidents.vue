<template>
  <AppSidebar v-slot="{ isDark }">
    <div class="incident-workspace min-h-full space-y-6 p-5 sm:p-8 lg:p-10" :data-dark="isDark">
      <header class="flex flex-wrap items-start justify-between gap-5"><div><p class="incident-eyebrow">DMBC Water · Service care</p><h1 class="mt-2 text-3xl font-extrabold tracking-tight sm:text-4xl">Service incidents</h1><p class="mt-3 max-w-xl text-sm leading-6 incident-muted">From the first leak report to a completed repair. Keep every account, location, and follow-up in one place.</p></div><button class="incident-primary" :disabled="!Object.keys(options.categories).length" @click="openComposer()">＋ Report an incident</button></header>
      <section class="grid gap-3 sm:grid-cols-2 xl:grid-cols-4" aria-label="Incident overview"><button v-for="card in cards" :key="card.key" class="incident-panel p-5 text-left transition hover:ring-2 hover:ring-teal-500/20" @click="chooseCard(card)"><span class="text-xs font-semibold incident-muted">{{ card.label }}</span><strong class="mt-3 block text-3xl font-extrabold" :class="card.key === 'urgent' ? 'text-rose-500' : 'text-teal-600'">{{ summary[card.key] }}</strong><span class="mt-2 block text-xs incident-muted">{{ card.help }}</span></button></section>
      <p v-if="notice" role="status" class="rounded-2xl border border-teal-500/20 bg-teal-500/10 px-5 py-4 text-sm text-teal-600">{{ notice }}</p>
      <section class="incident-panel p-5 sm:p-6"><div class="mb-4 flex flex-wrap items-center justify-between gap-3"><h2 class="font-bold">Find a report</h2><div class="flex gap-3"><button v-if="filters.customer_id" class="text-xs font-bold text-teal-600" @click="clearCustomer">Show all accounts</button><button class="incident-secondary" :disabled="loading" @click="load(pagination.current_page)">{{ loading ? 'Refreshing…' : 'Refresh' }}</button></div></div><div class="grid gap-4 sm:grid-cols-2 xl:grid-cols-[1.6fr_1fr_1fr_1fr]"><label>Search<input v-model.trim="filters.q" type="search" maxlength="120" placeholder="Account, name, location, crew or report number…" class="mt-2 w-full"/></label><label>Status<select v-model="filters.status" class="mt-2 w-full"><option value="open">Open reports</option><option value="all">All reports</option><option value="completed">Resolved / closed</option><option v-for="(label, key) in options.statuses" :key="key" :value="key">{{ label }}</option></select></label><label>Priority<select v-model="filters.priority" class="mt-2 w-full"><option value="">Any priority</option><option v-for="(label, key) in options.priorities" :key="key" :value="key">{{ label }}</option></select></label><label>Issue type<select v-model="filters.category" class="mt-2 w-full"><option value="">Any issue</option><option v-for="(label, key) in options.categories" :key="key" :value="key">{{ label }}</option></select></label></div><p v-if="filters.customer_id" class="mt-4 text-xs incident-muted">Showing reports linked to the selected member account.</p><p class="mt-3 text-xs incident-muted">Overview counts follow your search, issue, priority, and account filters. Status filters apply to the list below.</p></section>
      <section class="incident-panel overflow-hidden"><header class="flex flex-wrap items-center justify-between gap-3 border-b border-inherit px-6 py-5"><div><h2 class="font-bold">{{ filters.status === 'open' ? 'Reports needing attention' : 'Incident register' }}</h2><p class="mt-1 text-xs incident-muted">Urgent reports appear first, followed by the most recent reports in each priority.</p></div><span role="status" class="text-xs incident-muted">{{ loading ? 'Loading reports…' : pagination.total + ' matching reports' }}</span></header>
        <div v-if="loading" role="status" class="p-12 text-center text-sm incident-muted">Loading service reports…</div><div v-else-if="error" role="alert" class="p-8"><p class="text-sm text-rose-500">{{ error }}</p><button class="incident-secondary mt-4" @click="load()">Try again</button></div>
        <div v-else-if="!items.length" class="px-6 py-14 text-center"><span class="mx-auto flex h-16 w-16 items-center justify-center rounded-2xl bg-teal-500/10 text-teal-600"><svg class="h-8 w-8" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" aria-hidden="true"><path d="M12 3 2 21h20L12 3Zm0 6v5m0 3v.01"/></svg></span><h3 class="mt-5 text-lg font-bold">No reports match this view</h3><p class="mx-auto mt-2 max-w-md text-sm leading-6 incident-muted">Adjust your filters, or record a leak, meter concern, or supply problem for a member account.</p><button class="incident-primary mt-5" @click="openComposer()">Report an incident</button></div>
        <ul v-else class="divide-y incident-divide"><li v-for="incident in items" :key="incident.id"><button @click="openDetails(incident.id)" class="incident-row grid w-full gap-4 p-5 text-left sm:p-6 lg:grid-cols-[1.3fr_1fr_auto]"><div><div class="flex flex-wrap items-center gap-2"><span class="text-xs font-bold text-teal-600">{{ incident.reference_number }}</span><span class="incident-badge" :class="priorityColor(incident.priority)">{{ options.priorities[incident.priority] }}</span><span class="incident-badge" :class="statusColor(incident.status)">{{ options.statuses[incident.status] }}</span></div><h3 class="mt-3 text-base font-bold">{{ incident.title }}</h3><p class="mt-2 text-xs incident-muted">{{ options.categories[incident.category] }} · Reported {{ auditDate(incident.created_at) }}</p></div><div class="min-w-0"><p class="text-sm font-bold">{{ incident.customer_name }}</p><p class="mt-1 text-xs text-teal-600">{{ incident.account_no }}<span v-if="incident.barangay"> · {{ incident.barangay }}</span></p><p class="mt-2 line-clamp-2 break-words text-xs leading-5 incident-muted">{{ incident.location }}</p></div><div class="lg:max-w-48 lg:text-right"><p class="text-xs incident-muted">Assigned crew</p><p class="mt-1 break-words text-sm font-semibold">{{ incident.assigned_to || 'Not assigned yet' }}</p><p class="mt-3 text-xs font-bold text-teal-600">Open report →</p></div></button></li></ul>
        <footer v-if="!loading && pagination.total" class="flex items-center justify-between gap-4 border-t border-inherit px-6 py-4"><p class="text-xs incident-muted">Page {{ pagination.current_page }} of {{ pagination.last_page }}</p><div class="flex gap-2"><button class="incident-secondary" :disabled="pagination.current_page <= 1" @click="load(pagination.current_page - 1)">Previous</button><button class="incident-secondary" :disabled="pagination.current_page >= pagination.last_page" @click="load(pagination.current_page + 1)">Next</button></div></footer>
      </section>
      <p class="text-xs leading-5 incident-muted">Service reports keep their own repair history. They do not change a member’s bill, meter reading, or connection status.</p>
      <IncidentComposer v-if="composerOpen" ref="composer" :customer-id="composerCustomer" :options="options" :is-dark="isDark" @close="closeComposer" @created="created" @existing="showExisting"/>
      <IncidentDetails v-if="selectedId" :key="selectedId" ref="details" :incident-id="selectedId" :is-dark="isDark" @close="closeDetails" @saved="saved" @select="openDetails"/>
      <ConfirmDialog :is-open="confirming" :is-dark="isDark" title="Discard this draft?" message="Your unsaved incident changes will be lost. Saved reports and history remain available." confirm-text="Discard draft" @confirm="resolveDiscard(true)" @cancel="resolveDiscard(false)"/>
    </div>
  </AppSidebar>
</template>

<script setup>
import { ref, watch, onMounted, onBeforeUnmount } from 'vue';
import { useRoute, useRouter, onBeforeRouteLeave, onBeforeRouteUpdate } from 'vue-router';
import AppSidebar from '../components/AppSidebar.vue';
import ConfirmDialog from '../components/ConfirmDialog.vue';
import IncidentComposer from '../components/incidents/IncidentComposer.vue';
import IncidentDetails from '../components/incidents/IncidentDetails.vue';
import { useIncidents } from '../composables/useIncidents.js';
import { auditDate } from '../utils/auditPresentation.js';
import { statusColor, priorityColor } from '../utils/incidentPresentation.js';
const route = useRoute(), router = useRouter();
const { filters, items, summary, options, pagination, loading, error, load } = useIncidents();
const composerOpen = ref(false), composerCustomer = ref(null), selectedId = ref(null), notice = ref('');
const composer = ref(null), details = ref(null), confirming = ref(false);
let discardResolve;
const cards = [{ key: 'open', label: 'Open incidents', help: 'Reported and in progress', status: 'open' }, { key: 'urgent', label: 'Urgent attention', help: 'Open reports at urgent priority', status: 'open', priority: 'urgent' }, { key: 'in_progress', label: 'Repairs in progress', help: 'Inspection or repair underway', status: 'in_progress' }, { key: 'resolved', label: 'Resolved / closed', help: 'Repair outcomes recorded', status: 'completed' }];
function pending() { return composer.value?.busy || details.value?.busy; }
function hasDraft() { return composer.value?.dirty || details.value?.dirty; }
function confirmDiscard() { if (pending()) return Promise.resolve(false); if (!hasDraft()) return Promise.resolve(true); if (confirming.value) return Promise.resolve(false); confirming.value = true; return new Promise(resolve => { discardResolve = resolve; }); }
function resolveDiscard(value) { confirming.value = false; discardResolve?.(value); discardResolve = null; }
async function closeComposer() { if (await confirmDiscard()) composerOpen.value = false; }
async function closeDetails() { if (await confirmDiscard()) selectedId.value = null; }
async function openDetails(id) { if (await confirmDiscard()) { composerOpen.value = false; selectedId.value = id; } }
async function openComposer(customerId = Number(filters.customer_id) || null) { if (await confirmDiscard()) { selectedId.value = null; composerCustomer.value = customerId; composerOpen.value = true; } }
function created(incident) { composerOpen.value = false; selectedId.value = incident.id; notice.value = `${incident.reference_number} was recorded.`; load(1); }
function saved(incident) { notice.value = `${incident.reference_number} was updated. Its history has been recorded.`; load(pagination.value.current_page); }
async function showExisting(id) { if (await confirmDiscard()) { composerOpen.value = false; filters.customer_id = id; filters.status = 'open'; } }
function clearCustomer() { const query = { ...route.query }; delete query.customer; delete query.new; router.replace({ query }); filters.customer_id = ''; }
function chooseCard(card) { filters.status = card.status; filters.priority = card.priority || ''; }
watch(() => route.query, query => {
  filters.customer_id = Number(query.customer) > 0 ? String(Number(query.customer)) : '';
  if (query.new === '1') { composerCustomer.value = Number(query.customer) > 0 ? Number(query.customer) : null; composerOpen.value = true; const next = { ...query }; delete next.new; router.replace({ query: next }); }
  if (Number(query.incident) > 0) selectedId.value = Number(query.incident);
}, { immediate: true });
function beforeUnload(event) { if (pending() || hasDraft()) { event.preventDefault(); event.returnValue = ''; } }
onBeforeRouteLeave(confirmDiscard); onBeforeRouteUpdate(confirmDiscard);
onMounted(() => { load(1); window.addEventListener('beforeunload', beforeUnload); });
onBeforeUnmount(() => { window.removeEventListener('beforeunload', beforeUnload); resolveDiscard(false); });
</script>
