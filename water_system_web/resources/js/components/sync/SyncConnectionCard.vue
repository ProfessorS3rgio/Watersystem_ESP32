<template>
  <section class="sync-panel overflow-hidden">
    <div class="flex flex-col gap-6 p-6 sm:flex-row sm:items-center sm:justify-between">
      <div class="flex items-start gap-4"><div class="flex h-14 w-14 shrink-0 items-center justify-center rounded-2xl bg-teal-500/10 text-teal-600"><svg class="h-7 w-7" fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="1.7" aria-hidden="true"><path d="M8 3h8v18H8V3Zm3 14h2M3 8h2m14 0h2M3 12h2m14 0h2" stroke-linecap="round"/></svg></div><div><div class="flex flex-wrap items-center gap-3"><h2 class="text-lg font-bold">{{ connected ? 'Device connected' : 'Connect your field device' }}</h2><span class="rounded-full px-2.5 py-1 text-xs font-bold" :class="connected ? 'bg-teal-100 text-teal-800' : 'bg-slate-100 text-slate-600'">{{ connected ? 'USB connected' : 'Offline' }}</span></div><p class="mt-1 text-sm sync-muted">{{ connected ? port : 'Connect the ESP32 using USB, then select its serial port.' }}</p><p v-if="!supported" class="mt-2 text-sm text-amber-600">Use a browser with Web Serial support, such as Chrome or Edge.</p><p v-if="error" role="alert" class="mt-2 text-sm text-red-500">{{ error }}</p></div></div>
      <div class="flex shrink-0 flex-wrap gap-3"><button v-if="connected" @click="$emit('disconnect')" :disabled="busy" class="sync-secondary disabled:opacity-50">Disconnect</button><button v-else @click="$emit('connect')" :disabled="connecting || !supported" class="sync-primary disabled:opacity-50">{{ connecting ? 'Connecting…' : 'Connect device' }}</button><button v-if="connected" @click="$emit('sync')" :disabled="busy" class="sync-primary disabled:opacity-50">{{ syncing ? 'Synchronizing…' : 'Start sync' }}</button></div>
    </div>
    <div class="sync-soft border-t border-inherit p-6">
      <div class="mb-4 flex flex-wrap justify-between gap-2 text-sm"><p class="font-semibold">{{ syncing ? 'Synchronization in progress' : status === 'completed' ? 'Last sync completed' : status === 'failed' ? 'Sync stopped — review the activity log' : 'Sync workflow' }}</p><span class="sync-muted">Web ↔ ESP32</span></div>
      <ol class="grid gap-3 sm:grid-cols-5"><li v-for="(step, index) in steps" :key="step" class="flex items-center gap-2 text-xs" :class="stage === index && syncing ? 'font-bold text-teal-600' : 'sync-muted'"><span class="flex h-7 w-7 shrink-0 items-center justify-center rounded-full border" :class="status === 'completed' || index < stage ? 'border-teal-600 bg-teal-600 text-white' : stage === index && syncing ? 'border-teal-500 text-teal-600 animate-pulse' : 'border-inherit'">{{ status === 'completed' || index < stage ? '✓' : index + 1 }}</span>{{ step }}</li></ol>
      <p v-if="syncError" role="alert" class="mt-4 text-sm text-red-500">{{ syncError }}</p>
    </div>
  </section>
</template>

<script setup>
defineProps({ connected: Boolean, connecting: Boolean, busy: Boolean, syncing: Boolean, supported: Boolean, port: String, error: String, status: String, syncError: String, stage: Number })
defineEmits(['connect', 'disconnect', 'sync'])
const steps = ['Restore point', 'Reference data', 'Readings & bills', 'Member accounts', 'Finalize']
</script>
