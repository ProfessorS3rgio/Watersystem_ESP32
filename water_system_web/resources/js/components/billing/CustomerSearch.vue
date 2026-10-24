<template>
  <div class="min-w-0">
    <div class="billing-panel" :class="isDark ? 'bg-gray-800 text-white' : 'bg-white text-gray-900'">
      <div class="billing-panel-header"><h3 class="text-lg font-bold">Find a member</h3><p class="mt-1 text-sm text-slate-500">Search by name or account number.</p></div>
      <div class="space-y-4 p-6">
        <div>
          <label for="billing-member-search" class="mb-2 block text-sm font-semibold">Name or account number</label>
          <input id="billing-member-search" v-model="query" type="search" autocomplete="off" placeholder="Start typing a name or account…" class="w-full" aria-controls="billing-member-results" :aria-busy="searching" />
        </div>
        <div v-if="showResults && query.trim()" id="billing-member-results">
          <p v-if="searching" role="status" class="py-3 text-sm text-teal-600">Searching members…</p>
          <p v-else-if="error" role="alert" class="py-3 text-sm text-red-600">{{ error }}</p>
          <template v-else>
            <p role="status" class="mb-3 text-xs text-slate-500">{{ matches.length ? matches.length + ' matching members — select an account below.' : 'No matching members found.' }}</p>
            <ul class="max-h-80 space-y-2 overflow-y-auto">
              <li v-for="customer in matches.slice(0, 20)" :key="customer.id">
                <button type="button" @click="selectCustomer(customer)" class="w-full rounded-xl border p-3 text-left transition" :class="isDark ? 'border-slate-700 hover:bg-slate-800' : 'border-teal-100 hover:bg-teal-50'">
                  <span class="block text-sm font-bold">{{ customer.customer_name }}</span>
                  <span class="mt-1 block text-xs font-semibold text-teal-600">{{ customer.account_no }}</span>
                  <span v-if="customer.address" class="mt-1 block text-xs text-slate-500">{{ customer.address }}</span>
                </button>
              </li>
            </ul>
            <p v-if="matches.length > 20" class="mt-2 text-xs text-slate-500">Showing the first 20 matches. Keep typing to narrow the list.</p>
          </template>
        </div>
        <div v-if="selectedCustomer" class="rounded-xl border p-4" :class="isDark ? 'border-teal-800 bg-teal-950/40' : 'border-teal-100 bg-teal-50'">
          <p class="mb-2 text-xs font-bold uppercase tracking-wider text-teal-600">Selected member</p>
          <p class="font-bold">{{ selectedCustomer.customer_name }}</p>
          <p class="mt-1 text-sm">{{ selectedCustomer.account_no }}</p>
          <p class="mt-1 text-xs text-slate-500">{{ selectedCustomer.address || 'No address recorded' }}</p>
          <button type="button" @click="clearSelection" class="mt-3 text-sm font-semibold text-teal-600 hover:underline">Clear selection</button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, watch, onBeforeUnmount } from 'vue'

const props = defineProps({
  isDark: { type: Boolean, default: false },
  selectedCustomer: { type: Object, default: null }
})
const emit = defineEmits(['search-customer', 'clear-customer'])
const query = ref('')
const matches = ref([])
const searching = ref(false)
const error = ref('')
const showResults = ref(false)
let timer
let controller
let revision = 0

watch(query, value => {
  clearTimeout(timer)
  controller?.abort()
  const currentRevision = ++revision
  matches.value = []
  error.value = ''
  showResults.value = true
  if (props.selectedCustomer) emit('clear-customer')
  const term = value.trim()
  searching.value = Boolean(term)
  if (!term) return
  timer = setTimeout(async () => {
    controller = new AbortController()
    try {
      const response = await fetch('/customers?' + new URLSearchParams({ search: term }), {
        headers: { Accept: 'application/json' },
        credentials: 'same-origin',
        signal: controller.signal
      })
      if (!response.ok) throw new Error('Unable to search members. Please try again.')
      const data = await response.json()
      if (currentRevision !== revision) return
      matches.value = Array.isArray(data.data) ? data.data : []
    } catch (err) {
      if (currentRevision === revision && err.name !== 'AbortError') error.value = err.message
    } finally {
      if (currentRevision === revision) searching.value = false
    }
  }, 250)
})

function selectCustomer(customer) {
  showResults.value = false
  emit('search-customer', { customer })
}

function clearSelection() {
  query.value = ''
  showResults.value = false
  emit('clear-customer')
}

onBeforeUnmount(() => {
  revision++
  clearTimeout(timer)
  controller?.abort()
})
</script>
