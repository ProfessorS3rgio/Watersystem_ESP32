<template>
  <section class="settings-panel overflow-hidden">
    <header class="settings-panel-heading flex flex-wrap items-start justify-between gap-3"><div><p class="settings-eyebrow">Rates &amp; charges</p><h2>Customer categories</h2><p class="settings-muted mt-2 text-sm">Maintain water rates and minimum charges for each account type.</p></div><span class="settings-soft rounded-full px-3 py-1 text-xs font-bold text-teal-600">{{ form.length }} categories</span></header>
    <div v-if="loading" role="status" class="p-8 text-sm settings-muted">Loading customer categories…</div>
    <div v-else-if="!ready" role="alert" class="p-6"><p class="text-sm text-red-500">{{ error }}</p><button @click="reload" class="settings-secondary mt-4">Try again</button></div>
    <form v-else @submit.prevent="save">
      <fieldset :disabled="saving" class="p-5 sm:p-6">
        <div v-if="!form.length" class="py-10 text-center settings-muted text-sm">No customer categories have been configured.</div>
        <div class="grid gap-5 xl:grid-cols-2"><article v-for="(category, index) in form" :key="category.type_id" class="settings-field-card"><div class="mb-5 flex items-center gap-3"><span class="flex h-10 w-10 items-center justify-center rounded-xl bg-teal-500/10 text-sm font-bold text-teal-600">{{ String(index + 1).padStart(2, '0') }}</span><div><h3 class="font-bold">{{ category.type_name }}</h3><p class="mt-0.5 text-xs settings-muted">Category rate schedule</p></div></div><div class="grid gap-4 sm:grid-cols-2"><div v-for="field in fields" :key="field.key"><label :for="'category-' + category.type_id + '-' + field.key">{{ field.label }}</label><input :id="'category-' + category.type_id + '-' + field.key" v-model.number="category[field.key]" type="number" min="0" max="1000000" :step="field.step" required class="mt-2 w-full"/></div></div></article></div>
        <p class="settings-help mt-5">Save rate changes here, then sync the master to update its category data.</p>
        <p v-if="error" role="alert" class="settings-feedback-error mt-4">{{ error }}</p><p v-if="message" role="status" class="settings-feedback-success mt-4">{{ message }}</p>
      </fieldset>
      <SettingsSaveBar :dirty="dirty" :saving="saving" label="Save category rates" @discard="discard" />
    </form>
  </section>
</template>

<script setup>
import SettingsSaveBar from './SettingsSaveBar.vue'
import { useEditableSettings } from '../../composables/useEditableSettings.js'
import { settingsJson } from '../../services/settingsApi.js'
const emit = defineEmits(['state-change'])
const fields = [{ key: 'rate_per_m3', label: 'Rate per m³ (₱)', step: '0.01' }, { key: 'min_m3', label: 'Minimum volume threshold (m³)', step: '1' }, { key: 'min_charge', label: 'Minimum charge (₱)', step: '0.01' }, { key: 'penalty', label: 'Late penalty (₱)', step: '0.01' }]
const normalize = row => ({ type_id: row.type_id, type_name: row.type_name, rate_per_m3: Number(row.rate_per_m3), min_m3: Number(row.min_m3), min_charge: Number(row.min_charge), penalty: Number(row.penalty) })
const editor = useEditableSettings({
  section: 'categories', initial: [], emit,
  load: async () => ((await settingsJson('/customer-types')).data || []).map(normalize),
  persist: async rows => {
    let saved = 0
    for (const row of rows) {
      const index = editor.baseline.value.findIndex(item => item.type_id === row.type_id)
      if (JSON.stringify(row) === JSON.stringify(editor.baseline.value[index])) continue
      try {
        const { rate_per_m3, min_m3, min_charge, penalty } = row
        const result = await settingsJson('/customer-types/' + row.type_id, { method: 'PUT', body: JSON.stringify({ rate_per_m3, min_m3, min_charge, penalty }) })
        const confirmed = normalize(result.data)
        // Preserve each successful save if a later category fails, so retry sends only remaining edits.
        editor.baseline.value[index] = confirmed
        saved++
      } catch (err) { throw new Error(`${saved ? saved + ' category change(s) saved. ' : ''}${row.type_name}: ${err.message} Your remaining edits are still here.`) }
    }
    return editor.baseline.value
  },
})
const { form, loading, saving, ready, dirty, error, message, reload, discard, save } = editor
</script>
