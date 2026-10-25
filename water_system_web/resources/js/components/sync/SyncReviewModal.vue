<template>
  <Teleport to="body">
    <div class="dmbc-modal fixed inset-0 z-[70] flex items-center justify-center bg-slate-950/60 p-3 backdrop-blur-sm sm:p-6" :data-dark="isDark" @keydown="handleKeys">
      <section ref="dialog" role="dialog" aria-modal="true" aria-labelledby="sync-review-title" :aria-busy="busy" tabindex="-1" class="dmbc-dialog flex max-h-[92vh] w-full max-w-6xl flex-col overflow-hidden rounded-3xl border shadow-2xl" :class="isDark ? 'border-slate-700 bg-slate-900 text-slate-100' : 'border-white bg-white text-slate-900'">
        <header class="border-b border-teal-500/20 bg-teal-500/5 p-5 sm:p-6">
          <p class="text-xs font-bold uppercase tracking-widest text-teal-600">DMBC · Sync verification</p>
          <h2 id="sync-review-title" class="mt-2 text-2xl font-extrabold">Review before saving</h2>
          <p class="mt-2 text-sm opacity-75">Master {{ review.device_uid }} · {{ summary.readings }} readings · {{ summary.bills }} bills · {{ summary.payments }} transactions</p>
          <p class="mt-2 text-sm opacity-75">No incoming records have been saved. Approve the complete batch, or cancel to leave records pending on the master.</p>
        </header>
        <div class="min-h-0 flex-1 overflow-y-auto p-5 sm:p-6">
          <div class="mb-5 grid grid-cols-2 gap-3 sm:grid-cols-4">
            <div v-for="card in cards" :key="card.label" class="rounded-2xl border p-4" :class="card.color"><p class="text-2xl font-extrabold">{{ card.count }}</p><p class="mt-1 text-xs font-semibold">{{ card.label }}</p></div>
          </div>
          <div v-if="review.preview.issues.length || summary.blocked" class="mb-4 rounded-xl border border-red-400/40 bg-red-500/10 p-4 text-sm" role="alert">
            <p class="font-bold">This batch needs attention before it can be saved.</p>
            <p v-for="issue in review.preview.issues" :key="issue" class="mt-1">{{ issue }}</p>
            <p v-if="summary.blocked" class="mt-1">Review the flagged rows below. Cancel, correct the source records, and sync again.</p>
          </div>
          <div class="mb-4 flex flex-wrap gap-3">
            <input v-model="search" type="search" aria-label="Search batch records" placeholder="Search name, account, bill, or reading…" class="min-w-48 flex-1 rounded-xl border border-teal-500/30 bg-transparent px-4 py-2.5 text-sm" />
            <select v-model="filter" aria-label="Filter verification results" class="rounded-xl border border-teal-500/30 px-3 py-2.5 text-sm" :class="isDark ? 'bg-slate-800' : 'bg-white'"><option value="all">All records</option><option value="blocked">Needs attention</option><option value="warnings">With warnings</option><option value="ready">Ready to save</option><option value="already_saved">Already saved</option></select>
          </div>
          <div class="overflow-x-auto rounded-2xl border border-teal-500/20">
            <table class="w-full min-w-[760px] text-left text-sm">
              <thead class="bg-teal-500/10 text-xs uppercase"><tr><th class="px-4 py-3">Record / customer</th><th class="px-4 py-3">Device values</th><th class="px-4 py-3">Verification</th></tr></thead>
              <tbody>
                <tr v-for="row in pageRows" :key="row.kind + ':' + row.key" class="border-t border-teal-500/15 align-top">
                  <td class="px-4 py-4"><p class="font-bold">{{ row.label }}</p><p class="mt-1">{{ row.customer_name }}</p><p class="mt-1 text-xs opacity-65">{{ row.account }} · {{ row.kind === 'payment' ? 'Transaction' : row.kind === 'reading' ? 'Reading' : 'Bill' }}</p></td>
                  <td class="px-4 py-4">
                    <template v-if="row.kind === 'reading'"><p class="font-semibold">{{ row.incoming.previous_reading }} → {{ row.incoming.current_reading }} <span class="text-teal-600">({{ row.incoming.usage_m3 }} m³)</span></p><p class="mt-1 text-xs opacity-70">{{ new Date(row.incoming.reading_at * 1000).toLocaleString('en-PH') }}</p></template>
                    <template v-else-if="row.kind === 'bill'"><p class="font-semibold">₱{{ money(row.incoming.total_due) }} · {{ row.incoming.status }}</p><p class="mt-1 text-xs opacity-70">Charges ₱{{ money(row.incoming.charges) }} + penalty ₱{{ money(row.incoming.penalty) }}</p><p class="mt-1 text-xs opacity-70">{{ row.incoming.bill_date }} · Due {{ row.due_date || '—' }}</p><p v-if="row.planned_status" class="mt-1 text-xs">After save: {{ row.planned_status }} · Balance ₱{{ money(row.balance_due) }}</p></template>
                    <template v-else><p class="font-semibold">₱{{ money(row.incoming.amount) }} · {{ row.incoming.type }}</p><p class="mt-1 text-xs opacity-70">{{ row.incoming.bill_reference_number }}</p><p class="mt-1 text-xs opacity-70">{{ row.incoming.transaction_date }}</p><p class="mt-1 text-xs opacity-70">Cash ₱{{ money(row.incoming.cash_received) }} · Change ₱{{ money(row.incoming.change) }}</p></template>
                    <p v-if="row.before" class="mt-2 text-xs opacity-75">Website: {{ Object.entries(row.before).map(([key, value]) => `${fieldLabel(key)}: ${value}`).join(' · ') }}</p>
                  </td>
                  <td class="px-4 py-4"><span class="inline-flex rounded-full px-2.5 py-1 text-xs font-bold" :class="row.status === 'blocked' ? 'bg-red-100 text-red-800' : row.status === 'already_saved' ? 'bg-slate-100 text-slate-700' : 'bg-teal-100 text-teal-800'">{{ { blocked: 'Needs attention', already_saved: 'Already saved', ready: 'Ready to save' }[row.status] }}</span><p v-for="issue in row.issues" :key="issue" class="mt-2 text-xs text-red-500">{{ issue }}</p><p v-for="warning in row.warnings" :key="warning" class="mt-2 text-xs" :class="isDark ? 'text-amber-300' : 'text-amber-800'">{{ warning }}</p><p v-if="row.status === 'already_saved'" class="mt-2 text-xs opacity-65">Matching record will be skipped.</p></td>
                </tr>
                <tr v-if="!filtered.length"><td colspan="3" class="p-8 text-center opacity-70">No records match this filter.</td></tr>
              </tbody>
            </table>
          </div>
          <div v-if="filtered.length > pageSize" class="mt-3 flex items-center justify-between gap-3 text-xs"><button type="button" :disabled="page === 0" @click="page--" class="rounded-lg border px-3 py-2 disabled:opacity-40">Previous</button><span>Page {{ page + 1 }} of {{ Math.ceil(filtered.length / pageSize) }} · {{ filtered.length }} records</span><button type="button" :disabled="(page + 1) * pageSize >= filtered.length" @click="page++" class="rounded-lg border px-3 py-2 disabled:opacity-40">Next</button></div>
        </div>
        <footer class="border-t border-teal-500/20 p-5 sm:p-6">
          <p v-if="error" role="alert" class="mb-3 rounded-xl bg-red-500/10 p-3 text-sm text-red-500">{{ error }}</p>
          <label v-if="summary.warnings" class="mb-4 flex items-start gap-3 text-sm"><input v-model="accepted" :disabled="busy" type="checkbox" class="mt-1 accent-teal-600" /><span>I reviewed the {{ summary.warnings }} warning(s) and approve these values.</span></label>
          <p class="mb-4 text-xs opacity-65">Review expires after 30 minutes. Device and website records are checked again before saving. Receipt records your name and accepted warnings.</p>
          <div class="flex flex-wrap items-center justify-between gap-3"><button ref="cancelButton" type="button" :disabled="busy" @click="$emit('cancel')" class="rounded-xl border border-teal-500/30 px-4 py-3 text-sm font-bold disabled:opacity-50">Cancel sync</button><div class="flex flex-wrap gap-3"><button type="button" :disabled="busy" @click="$emit('refresh')" class="rounded-xl border border-teal-500/30 px-4 py-3 text-sm font-bold disabled:opacity-50">Refresh checks</button><button type="button" :disabled="busy || !review.preview.can_approve || (summary.warnings > 0 && !accepted)" @click="$emit('approve', accepted)" class="inline-flex items-center gap-2 rounded-xl bg-teal-700 px-5 py-3 text-sm font-bold text-white hover:bg-teal-800 disabled:opacity-40"><span v-if="busy" class="h-4 w-4 animate-spin rounded-full border-2 border-white/30 border-t-white motion-reduce:animate-none" aria-hidden="true"></span>{{ busy ? 'Checking and saving…' : 'Approve & save batch' }}</button></div></div>
        </footer>
      </section>
    </div>
  </Teleport>
</template>
<script setup>
import { ref, computed, watch, onMounted, onBeforeUnmount, nextTick } from 'vue'
const props = defineProps({ review: { type: Object, required: true }, busy: Boolean, error: String, isDark: Boolean })
const emit = defineEmits(['approve', 'cancel', 'refresh'])
const dialog = ref(null), cancelButton = ref(null), accepted = ref(false), search = ref(''), filter = ref('all'), page = ref(0)
const pageSize = 50
const summary = computed(() => props.review.preview.summary)
const cards = computed(() => [
  { label: 'Ready to save', count: summary.value.ready, color: 'border-teal-500/30 bg-teal-500/10' },
  { label: 'Already saved', count: summary.value.already_saved, color: 'border-slate-500/20 bg-slate-500/5' },
  { label: 'Needs attention', count: summary.value.blocked, color: 'border-red-500/30 bg-red-500/10' },
  { label: 'Warnings to review', count: summary.value.warnings, color: 'border-amber-500/30 bg-amber-500/10' },
])
const filtered = computed(() => props.review.preview.rows.filter(row => (filter.value === 'all' || (filter.value === 'warnings' ? row.warnings.length : row.status === filter.value)) && `${row.label} ${row.customer_name} ${row.account} ${row.incoming.bill_reference_number || ''}`.toLowerCase().includes(search.value.toLowerCase())))
const pageRows = computed(() => filtered.value.slice(page.value * pageSize, (page.value + 1) * pageSize))
watch([search, filter], () => { page.value = 0 })
watch(() => props.review.id, () => { accepted.value = false; page.value = 0 })
const money = value => Number(value || 0).toLocaleString('en-PH', { minimumFractionDigits: 2, maximumFractionDigits: 2 })
const fieldLabel = key => ({ status: 'Status', total_due: 'Total', amount_paid: 'Paid', previous_reading: 'Previous', current_reading: 'Current', usage_m3: 'Usage' }[key] || key)
let previousFocus, previousOverflow
onMounted(async () => { previousFocus = document.activeElement; previousOverflow = document.body.style.overflow; document.body.style.overflow = 'hidden'; await nextTick(); dialog.value?.focus() })
onBeforeUnmount(() => { document.body.style.overflow = previousOverflow; previousFocus?.focus?.() })
function handleKeys(event) {
  if (event.key === 'Escape') { event.preventDefault(); if (!props.busy) emit('cancel') }
  if (event.key !== 'Tab') return
  const elements = [...dialog.value.querySelectorAll('button:not(:disabled), input:not(:disabled), select:not(:disabled), [tabindex="0"]')]
  const first = elements[0], last = elements.at(-1)
  if (!first) { event.preventDefault(); return }
  if (event.shiftKey && (document.activeElement === first || document.activeElement === dialog.value)) { event.preventDefault(); last.focus() }
  else if (!event.shiftKey && (document.activeElement === last || document.activeElement === dialog.value)) { event.preventDefault(); first.focus() }
}
</script>
