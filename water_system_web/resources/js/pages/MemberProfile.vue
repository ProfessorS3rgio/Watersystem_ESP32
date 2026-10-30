<template>
  <AppSidebar v-slot="{ isDark }">
    <main class="min-h-full p-5 sm:p-8 lg:p-10" :class="isDark ? 'bg-slate-950 text-slate-100' : 'bg-[#f5f8f7] text-slate-900'">
      <RouterLink to="/clients" class="inline-flex items-center gap-2 text-sm font-bold text-teal-600 hover:text-teal-500"><span aria-hidden="true">←</span> Community members</RouterLink>
      <header class="mb-7 mt-6 flex flex-wrap items-start justify-between gap-4"><div><p class="text-xs font-bold uppercase tracking-[.2em] text-teal-600">DMBC Water · Member records</p><h1 class="mt-2 text-3xl font-extrabold tracking-tight">Member profile</h1><p class="mt-2 text-sm opacity-65">Personal details, contact information, and the water connection in one place.</p></div><span v-if="dirty" role="status" class="rounded-full border border-amber-500/30 bg-amber-500/10 px-4 py-2 text-xs font-bold" :class="isDark ? 'text-amber-200' : 'text-amber-800'">Unsaved changes</span></header>
      <div v-if="loading" role="status" class="flex items-center gap-3 rounded-2xl border border-teal-500/20 bg-teal-500/5 p-8"><span class="h-6 w-6 animate-spin rounded-full border-2 border-teal-500/25 border-t-teal-600 motion-reduce:animate-none" aria-hidden="true"></span>Loading member profile…</div>
      <div v-if="error" role="alert" class="mb-5 rounded-xl border border-red-400/30 bg-red-500/10 p-4 text-sm" :class="isDark ? 'text-red-300' : 'text-red-800'">{{ error }}<button type="button" @click="reload" :disabled="saving" class="ml-3 font-bold underline disabled:opacity-50">Reload profile</button></div>
      <p v-if="saved && !dirty" role="status" class="mb-5 rounded-xl border border-teal-500/30 bg-teal-500/10 p-4 text-sm font-semibold">Profile saved successfully.</p>
      <form v-if="record" class="grid items-start gap-6 lg:grid-cols-[280px_minmax(0,1fr)]" @submit.prevent="save">
        <aside class="space-y-5 lg:sticky lg:top-6">
          <section class="overflow-hidden rounded-3xl border shadow-sm" :class="panel(isDark)">
            <div class="h-20 bg-gradient-to-br from-teal-800 via-teal-700 to-cyan-600"></div>
            <div class="px-6 pb-6"><div class="-mt-9 flex h-20 w-20 items-center justify-center rounded-2xl border-4 bg-teal-50 text-2xl font-extrabold text-teal-800 shadow-sm" :class="isDark ? 'border-slate-900' : 'border-white'">{{ initials }}</div><h2 class="mt-4 break-words text-xl font-extrabold">{{ record.customer.full_name }}</h2><p class="mt-1 text-sm font-bold text-teal-600">{{ record.customer.account_no }}</p><span class="mt-4 inline-flex rounded-full px-3 py-1 text-xs font-bold" :class="record.customer.status === 'active' ? 'bg-teal-100 text-teal-800' : 'bg-red-100 text-red-800'">{{ record.customer.status === 'active' ? 'Active connection' : 'Disconnected' }}</span>
              <dl class="mt-5 space-y-4 border-t border-teal-500/15 pt-5 text-sm"><div><dt class="text-xs opacity-55">Barangay</dt><dd class="mt-1 font-semibold">{{ record.customer.barangay || '—' }}</dd></div><div><dt class="text-xs opacity-55">Customer category</dt><dd class="mt-1 font-semibold">{{ record.customer.customer_type || '—' }}</dd></div><div><dt class="text-xs opacity-55">Meter number</dt><dd class="mt-1 break-words font-semibold">{{ record.profile?.meter_number || 'Not recorded' }}</dd></div><div><dt class="text-xs opacity-55">Birthdate</dt><dd class="mt-1 font-semibold">{{ record.profile?.birthdate || 'Not recorded' }}<span v-if="age !== null" class="ml-1 text-xs opacity-65">· {{ age }} years old</span></dd></div></dl>
            </div>
          </section>
          <RouterLink :to="{ path: '/incidents', query: { customer: String(record.customer.id), new: '1' } }" class="block rounded-2xl border border-teal-500/25 bg-teal-500/5 px-5 py-4 text-sm font-bold text-teal-600 hover:bg-teal-500/10">Report a service issue <span class="float-right" aria-hidden="true">↗</span></RouterLink>
          <p class="px-1 text-xs leading-5 opacity-60">Account numbers are managed from the community members list. This profile stays with the member when account numbers are swapped.</p>
        </aside>
        <div class="min-w-0 space-y-6">
          <section class="rounded-3xl border p-5 shadow-sm sm:p-7" :class="panel(isDark)">
            <div class="mb-6"><p class="text-xs font-bold uppercase tracking-widest text-teal-600">01 · Personal details</p><h2 class="mt-2 text-lg font-bold">Know your member</h2><p class="mt-1 text-sm opacity-60">Name parts are optional. Enter both first and last name to build the full name.</p></div>
            <div class="grid gap-5 sm:grid-cols-2">
              <label v-for="field in nameFields" :key="field.key" class="block text-sm font-semibold">{{ field.label }}<input v-model="draft[field.key]" :disabled="saving" :maxlength="field.max" :autocomplete="field.autocomplete" :class="input(isDark)" class="mt-2 block w-full rounded-xl border px-4 py-3 font-normal disabled:opacity-60" /><span v-if="fieldError(field.key)" class="mt-1 block text-xs text-red-500">{{ fieldError(field.key) }}</span></label>
              <label class="block text-sm font-semibold sm:col-span-2">Full name <span class="ml-1 font-normal opacity-50">· shown on bills and receipts</span><input v-model="fullName" :readonly="structured" :disabled="saving" required maxlength="255" autocomplete="name" :class="input(isDark)" class="mt-2 block w-full rounded-xl border px-4 py-3 font-normal read-only:opacity-70" /><span class="mt-2 block text-xs font-normal opacity-60">{{ structured ? 'Built from the name parts above.' : 'Your existing full name is kept as entered. We do not guess how to split it.' }}</span><span v-if="fieldError('full_name')" class="mt-1 block text-xs text-red-500">{{ fieldError('full_name') }}</span></label>
              <label class="block text-sm font-semibold">Birthdate <span class="font-normal opacity-50">· optional</span><input v-model="draft.birthdate" :disabled="saving" type="date" min="1900-01-01" :max="today" autocomplete="bday" :class="input(isDark)" class="mt-2 block w-full rounded-xl border px-4 py-3 font-normal" /><span v-if="fieldError('birthdate')" class="mt-1 block text-xs text-red-500">{{ fieldError('birthdate') }}</span></label>
              <label class="block text-sm font-semibold">Account number<input :value="record.customer.account_no" readonly :class="input(isDark)" class="mt-2 block w-full rounded-xl border px-4 py-3 font-normal opacity-60" /></label>
            </div>
          </section>
          <section class="rounded-3xl border p-5 shadow-sm sm:p-7" :class="panel(isDark)">
            <div class="mb-6"><p class="text-xs font-bold uppercase tracking-widest text-teal-600">02 · Contact &amp; location</p><h2 class="mt-2 text-lg font-bold">Keep in touch</h2></div>
            <div class="grid gap-5 sm:grid-cols-2"><label v-for="field in contactFields" :key="field.key" class="block text-sm font-semibold">{{ field.label }}<input v-model="draft[field.key]" :disabled="saving" :type="field.type" :maxlength="field.max" :autocomplete="field.autocomplete" :class="input(isDark)" class="mt-2 block w-full rounded-xl border px-4 py-3 font-normal" /><span v-if="fieldError(field.key)" class="mt-1 block text-xs text-red-500">{{ fieldError(field.key) }}</span></label><label class="block text-sm font-semibold sm:col-span-2">Service address<input v-model="draft.address" :disabled="saving" required maxlength="255" autocomplete="street-address" :class="input(isDark)" class="mt-2 block w-full rounded-xl border px-4 py-3 font-normal" /><span v-if="fieldError('address')" class="mt-1 block text-xs text-red-500">{{ fieldError('address') }}</span></label></div>
          </section>
          <section class="rounded-3xl border p-5 shadow-sm sm:p-7" :class="panel(isDark)">
            <div class="mb-6"><p class="text-xs font-bold uppercase tracking-widest text-teal-600">03 · Water connection</p><h2 class="mt-2 text-lg font-bold">Meter &amp; service record</h2></div>
            <div class="grid gap-5 sm:grid-cols-2"><label v-for="field in meterFields" :key="field.key" class="block text-sm font-semibold" :class="field.wide ? 'sm:col-span-2' : ''">{{ field.label }}<input v-model="draft[field.key]" :disabled="saving" :type="field.type || 'text'" :maxlength="field.max" :max="field.type === 'date' ? today : undefined" :class="input(isDark)" class="mt-2 block w-full rounded-xl border px-4 py-3 font-normal" /><span v-if="fieldError(field.key)" class="mt-1 block text-xs text-red-500">{{ fieldError(field.key) }}</span></label><label class="block text-sm font-semibold sm:col-span-2">Member notes<textarea v-model="draft.notes" :disabled="saving" maxlength="2000" rows="4" placeholder="Helpful service instructions or account details…" :class="input(isDark)" class="mt-2 block w-full resize-y rounded-xl border px-4 py-3 font-normal"></textarea><span v-if="fieldError('notes')" class="mt-1 block text-xs text-red-500">{{ fieldError('notes') }}</span></label></div>
          </section>
          <div class="sticky bottom-3 flex flex-wrap items-center justify-between gap-4 rounded-2xl border p-4 shadow-lg backdrop-blur-xl" :class="isDark ? 'border-slate-700 bg-slate-900/95' : 'border-slate-200 bg-white/95'"><p class="text-xs opacity-65">{{ dirty ? 'Save your changes before leaving this profile.' : 'Member details are up to date.' }}</p><div class="flex gap-3"><button type="button" @click="discard" :disabled="saving || !dirty" class="rounded-xl border border-teal-500/25 px-4 py-3 text-sm font-bold disabled:opacity-40">Discard</button><button type="submit" :disabled="saving || !dirty" class="inline-flex items-center gap-2 rounded-xl bg-gradient-to-r from-teal-700 to-cyan-700 px-5 py-3 text-sm font-bold text-white shadow-sm disabled:opacity-50"><span v-if="saving" class="h-4 w-4 animate-spin rounded-full border-2 border-white/30 border-t-white motion-reduce:animate-none" aria-hidden="true"></span>{{ saving ? 'Saving profile…' : 'Save profile' }}</button></div></div>
        </div>
      </form>
    </main>
  </AppSidebar>
</template>
<script setup>
import { computed, watch, onBeforeUnmount } from 'vue'
import { useRoute, onBeforeRouteLeave, onBeforeRouteUpdate } from 'vue-router'
import AppSidebar from '../components/AppSidebar.vue'
import { useMemberProfile } from '../composables/useMemberProfile.js'
import { fullMemberName, manilaToday, memberAge } from '../utils/memberProfile.js'
const route = useRoute()
const { record, draft, loading, saving, error, errors, saved, dirty, load, save, discard, dispose } = useMemberProfile()
const today = manilaToday()
const structured = computed(() => Boolean(fullMemberName(draft.value)))
const fullName = computed({ get: () => fullMemberName(draft.value) || draft.value.full_name || '', set: value => { draft.value.full_name = value } })
const initials = computed(() => (record.value?.customer.full_name || '?').split(/\s+/).filter(Boolean).slice(0, 2).map(value => value[0]).join('').toUpperCase())
const age = computed(() => memberAge(record.value?.profile?.birthdate, today))
const nameFields = [{ key:'first_name',label:'First name',max:80,autocomplete:'given-name' },{ key:'middle_name',label:'Middle name',max:80,autocomplete:'additional-name' },{ key:'last_name',label:'Last name',max:80,autocomplete:'family-name' },{ key:'name_suffix',label:'Suffix',max:20,autocomplete:'honorific-suffix' }]
const contactFields = [{key:'phone',label:'Contact number',type:'tel',max:40,autocomplete:'tel'},{key:'email',label:'Email address',type:'email',max:254,autocomplete:'email'}]
const meterFields = [{key:'meter_number',label:'Meter number',max:80},{key:'meter_model',label:'Meter brand / model',max:80},{key:'meter_location',label:'Meter location',max:255,wide:true},{key:'meter_installed_on',label:'Current meter installed on',type:'date'},{key:'service_started_on',label:'Service connection date',type:'date'}]
const fieldError = field => errors.value[field]?.[0] || ''
const panel = dark => dark ? 'border-slate-800 bg-slate-900' : 'border-slate-200 bg-white'
const input = dark => (dark ? 'border-slate-700 bg-slate-950 text-slate-100' : 'border-slate-200 bg-white text-slate-900') + ' outline-none focus:border-teal-500 focus:ring-2 focus:ring-teal-500/20'
function mayLeave() { return !saving.value && (!dirty.value || window.confirm('Discard unsaved member profile changes?')) }
function reload() { if (!saving.value && (!dirty.value || window.confirm('Reload this member and discard your unsaved changes?'))) return load(route.params.id) }
function beforeUnload(event) { if (dirty.value || saving.value) { event.preventDefault(); event.returnValue = '' } }
watch(() => route.params.id, id => load(id), { immediate:true })
onBeforeRouteLeave(mayLeave)
onBeforeRouteUpdate(mayLeave)
window.addEventListener('beforeunload', beforeUnload)
onBeforeUnmount(() => { dispose(); window.removeEventListener('beforeunload', beforeUnload) })
</script>
