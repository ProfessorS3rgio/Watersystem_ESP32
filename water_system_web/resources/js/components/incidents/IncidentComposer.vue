<template>
  <Teleport to="body"><div class="incident-workspace incident-overlay dmbc-modal" :data-dark="isDark" :class="{ dark: isDark }" @click.self="!busy && $emit('close')">
    <section ref="dialog" tabindex="-1" role="dialog" aria-modal="true" aria-labelledby="incident-create-title" class="dmbc-dialog w-full max-w-3xl" @keydown.esc.stop="!busy && $emit('close')">
      <header class="dmbc-dialog-header flex items-start justify-between gap-4"><div><p class="incident-eyebrow">DMBC Water · Service care</p><h2 id="incident-create-title" class="mt-1 text-xl font-extrabold">Report a service incident</h2><p class="mt-2 text-sm">Capture the issue, its location, and who needs assistance.</p></div><button :disabled="busy" @click="$emit('close')" aria-label="Close incident form">✕</button></header>
      <form @submit.prevent="submit" @input="dirty = true" class="space-y-6 p-5 sm:p-7">
        <section><h3 class="mb-3 font-bold">1. Member account</h3>
          <div v-if="customer" class="incident-soft rounded-2xl p-4"><div class="flex items-start justify-between gap-4"><div><p class="font-bold">{{ customer.customer_name }}</p><p class="mt-1 text-sm font-semibold text-teal-600">{{ customer.account_no }}</p><p class="mt-1 text-sm incident-muted">{{ customer.address || 'No address on file' }}</p></div><button type="button" :disabled="busy" @click="customer = null; query = ''; dirty = true" class="text-sm font-bold text-teal-600">Change</button></div><p v-if="customer.open_incidents_count" class="mt-3 rounded-xl bg-amber-500/10 p-3 text-sm text-amber-700">{{ customer.open_incidents_count }} open report(s) already exist for this account. <button type="button" class="font-bold underline" @click="$emit('existing', customer.customer_id)">Review them first</button>.</p></div>
          <div v-else><label for="incident-customer-search">Customer name or account number</label><input id="incident-customer-search" v-model="query" :disabled="busy" type="search" maxlength="120" autocomplete="off" placeholder="Start typing to find a member…" class="mt-2 w-full" aria-controls="incident-member-results" :aria-busy="searching"/><p v-if="searching" role="status" class="mt-3 text-sm incident-muted">Searching members…</p><p v-else-if="searchError" role="alert" class="mt-3 text-sm text-rose-500">{{ searchError }}</p><ul v-else id="incident-member-results" class="mt-3 max-h-52 space-y-2 overflow-y-auto"><li v-for="member in matches" :key="member.customer_id"><button type="button" @click="select(member)" class="incident-soft w-full rounded-xl p-3 text-left hover:ring-2 hover:ring-teal-500/30"><strong class="block text-sm">{{ member.customer_name }}</strong><span class="mt-1 block text-xs incident-muted">{{ member.account_no }} · {{ member.address }}<span v-if="member.open_incidents_count"> · {{ member.open_incidents_count }} open reports</span></span></button></li><li v-if="query && !matches.length" class="py-2 text-sm incident-muted">No matching members.</li></ul></div>
        </section>
        <section><h3 class="mb-3 font-bold">2. What happened?</h3><div class="grid gap-4 sm:grid-cols-2"><label>Issue type<select v-model="form.category" required :disabled="busy" class="mt-2 w-full"><option v-for="(label, key) in options.categories" :key="key" :value="key">{{ label }}</option></select></label><label>Priority<select v-model="form.priority" required :disabled="busy" class="mt-2 w-full"><option v-for="(label, key) in options.priorities" :key="key" :value="key">{{ label }}</option></select></label><label class="sm:col-span-2">Short title<input v-model.trim="form.title" required maxlength="180" :disabled="busy" placeholder="Water leaking beside the customer’s gate" class="mt-2 w-full"/></label><label class="sm:col-span-2">What was reported?<textarea v-model.trim="form.description" required minlength="5" maxlength="5000" rows="3" :disabled="busy" placeholder="Describe the leak, supply issue, or visible damage." class="mt-2 w-full"/></label><label class="sm:col-span-2">Exact location / nearby landmark<textarea v-model.trim="form.location" required maxlength="1000" rows="2" :disabled="busy" placeholder="Near the gate, beside the meter, along the road…" class="mt-2 w-full"/></label><label>Line section<select v-model="form.pipe_section" :disabled="busy" class="mt-2 w-full"><option v-for="(label, key) in options.sections" :key="key" :value="key">{{ label }}</option></select></label><label>Assigned crew <span class="incident-muted">(optional)</span><input v-model.trim="form.assigned_to" maxlength="160" :disabled="busy" placeholder="Technician or repair team" class="mt-2 w-full"/></label></div><p class="mt-3 text-xs incident-muted">Use Urgent for an issue needing immediate attention. Reports do not change water readings, charges, or connection status.</p></section>
        <p v-if="error" role="alert" class="rounded-xl bg-rose-500/10 p-4 text-sm text-rose-600">{{ error }}</p>
        <footer class="flex flex-wrap items-center justify-between gap-4 border-t border-inherit pt-5"><p class="max-w-sm text-xs incident-muted">A report number and activity history are created when you save.</p><div class="flex gap-3"><button type="button" class="incident-secondary" :disabled="busy" @click="$emit('close')">Cancel</button><button class="incident-primary" :disabled="busy || !customer">{{ busy ? 'Saving report…' : 'Save incident report' }}</button></div></footer>
      </form>
    </section>
  </div></Teleport>
</template>

<script setup>
import { ref, reactive, watch, onMounted, onBeforeUnmount } from 'vue';
import { incidentApi, incidentToken } from '../../services/incidentApi.js';
import { useIncidentDialog } from '../../composables/useIncidentDialog.js';
const props = defineProps({ options: Object, customerId: Number, isDark: Boolean });
const emit = defineEmits(['close', 'created', 'existing']);
const dialog = ref(null); useIncidentDialog(dialog);
const query = ref(''), customer = ref(null), matches = ref([]), searching = ref(false), searchError = ref('');
const busy = ref(false), dirty = ref(false), error = ref('');
const form = reactive({ client_token: incidentToken(), title: '', description: '', location: '', category: 'leak', priority: 'normal', pipe_section: 'unknown', assigned_to: '' });
let timer, controller, revision = 0;
function select(member, initial = false) { customer.value = member; matches.value = []; if (!form.location) form.location = member.address || ''; if (!initial) dirty.value = true; }
watch(query, value => {
  clearTimeout(timer); controller?.abort(); const current = ++revision; matches.value = []; searchError.value = '';
  if (customer.value || !value.trim()) { searching.value = false; return; }
  searching.value = true;
  timer = setTimeout(async () => {
    controller = new AbortController();
    try { const data = await incidentApi.customers({ q: value.trim() }, controller.signal); if (current === revision) matches.value = data.data; }
    catch (err) { if (current === revision && err.name !== 'AbortError') searchError.value = err.message; }
    finally { if (current === revision) searching.value = false; }
  }, 250);
});
onMounted(async () => {
  if (!props.customerId) return;
  const current = ++revision; controller = new AbortController(); searching.value = true;
  try { const data = await incidentApi.customers({ id: props.customerId }, controller.signal); if (current !== revision) return; if (data.data[0]) select(data.data[0], true); else searchError.value = 'This account is no longer available. Search for another member.'; }
  catch (err) { if (err.name !== 'AbortError') searchError.value = err.message; }
  finally { if (current === revision) searching.value = false; }
});
async function submit() {
  if (busy.value || !customer.value) return;
  busy.value = true; error.value = '';
  try { const result = await incidentApi.create({ ...form, assigned_to: form.assigned_to || null, customer_id: customer.value.customer_id }); dirty.value = false; busy.value = false; emit('created', result.data); }
  catch (err) { error.value = err.message; }
  finally { busy.value = false; }
}
onBeforeUnmount(() => { clearTimeout(timer); controller?.abort(); ++revision; });
defineExpose({ busy, dirty });
</script>
