<template>
  <section class="settings-panel overflow-hidden">
    <header class="settings-panel-heading"><p class="settings-eyebrow">Service policy</p><h2>Billing rules</h2><p class="settings-muted mt-2 text-sm">Set payment deadlines and manage your association’s overdue-charge policy.</p></header>
    <div v-if="loading" role="status" class="p-8 text-sm settings-muted">Loading billing rules…</div>
    <div v-else-if="!ready" role="alert" class="p-6"><p class="text-sm text-red-500">{{ error }}</p><button @click="reload" class="settings-secondary mt-4">Try again</button></div>
    <form v-else @submit.prevent="save">
      <fieldset :disabled="saving" class="space-y-6 p-5 sm:p-6">
        <div class="grid gap-4 md:grid-cols-2">
          <div class="settings-field-card"><label for="bill-due-days">Days until payment is due</label><div class="mt-3 flex items-center gap-3"><input id="bill-due-days" v-model.number="form.bill_due_days" type="number" min="0" max="365" required class="w-full"/><span class="text-xs settings-muted">days</span></div><p class="settings-help">Counted from the bill date.</p></div>
          <div class="settings-field-card"><label for="disconnect-days">Disconnection review period</label><div class="mt-3 flex items-center gap-3"><input id="disconnect-days" v-model.number="form.disconnection_days" type="number" min="0" max="365" required class="w-full"/><span class="text-xs settings-muted">days</span></div><p class="settings-help">Days after the due date before service may be disconnected.</p></div>
        </div>
        <div class="settings-soft flex flex-col gap-4 rounded-2xl p-5 sm:flex-row sm:items-center sm:justify-between"><div><p class="text-sm font-bold">Payment window at a glance</p><p class="mt-2 text-sm settings-muted">Bill issued <span aria-hidden="true">→</span> due after <strong>{{ form.bill_due_days || 0 }} days</strong> <span aria-hidden="true">→</span> service review after a further <strong>{{ form.disconnection_days || 0 }} days</strong>.</p></div><svg class="hidden h-9 w-9 shrink-0 text-teal-600 sm:block" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" aria-hidden="true"><path d="M7 3v4m10-4v4M3 10h18M5 5h14a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V7a2 2 0 0 1 2-2Z"/></svg></div>
        <div class="settings-field-card flex items-center justify-between gap-5"><div><p id="penalty-switch-label" class="text-sm font-bold">Enable overdue penalties</p><p class="mt-2 text-sm settings-muted">Allow additional charges on overdue bills. Set category-specific amounts in Customer categories.</p></div><button type="button" role="switch" aria-labelledby="penalty-switch-label" :aria-checked="form.penalty_enabled" @click="form.penalty_enabled = !form.penalty_enabled" class="relative inline-flex h-7 w-12 shrink-0 items-center rounded-full transition-colors" :class="form.penalty_enabled ? 'bg-teal-600' : 'bg-slate-400'"><span class="h-5 w-5 rounded-full bg-white shadow-sm transition-transform" :class="form.penalty_enabled ? 'translate-x-6' : 'translate-x-1'"></span></button></div>
        <details class="settings-field-card"><summary class="cursor-pointer text-sm font-bold">Additional policy defaults</summary><p class="mt-2 text-xs settings-muted">Maintain the saved default charge and grace period alongside category-specific penalties.</p><div class="mt-4 grid gap-4 sm:grid-cols-2"><div><label for="penalty-days">Additional-charge grace period (days)</label><input id="penalty-days" v-model.number="form.penalty_after_days" type="number" min="0" max="365" required class="mt-2 w-full"/></div><div><label for="penalty-amount">Default additional charge (₱)</label><input id="penalty-amount" v-model.number="form.penalty_amount" type="number" min="0" max="1000000" step="0.01" required class="mt-2 w-full"/></div></div></details>
        <p v-if="error" role="alert" class="settings-feedback-error">{{ error }}</p><p v-if="message" role="status" class="settings-feedback-success">{{ message }} Sync the master to send updated device policies.</p>
      </fieldset>
      <SettingsSaveBar :dirty="dirty" :saving="saving" label="Save billing rules" @discard="discard" />
    </form>
  </section>
</template>

<script setup>
import SettingsSaveBar from './SettingsSaveBar.vue'
import { useEditableSettings } from '../../composables/useEditableSettings.js'
import { settingsJson } from '../../services/settingsApi.js'
const emit = defineEmits(['state-change'])
const normalize = data => ({ bill_due_days: Number(data.bill_due_days ?? 5), disconnection_days: Number(data.disconnection_days ?? 8), penalty_after_days: Number(data.penalty_after_days ?? 0), penalty_amount: Number(data.penalty_amount ?? 0), penalty_enabled: data.penalty_enabled !== false && data.penalty_enabled !== 0 })
const { form, loading, saving, ready, dirty, error, message, reload, discard, save } = useEditableSettings({
  section: 'billing', initial: normalize({}), emit,
  load: async () => normalize((await settingsJson('/settings')).data || {}),
  persist: async payload => normalize((await settingsJson('/settings', { method: 'PUT', body: JSON.stringify(payload) })).data),
})
</script>
