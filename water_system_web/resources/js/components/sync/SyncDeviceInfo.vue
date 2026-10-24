<template>
  <section class="sync-panel p-6">
    <div class="mb-5 flex flex-wrap items-center justify-between gap-2"><div><h2 class="text-lg font-bold">Device overview</h2><p class="mt-1 text-sm sync-muted">{{ connected ? 'Information reported by the connected device.' : 'Connect a device to read its information.' }}</p></div><span class="text-xs font-bold text-teal-600">ESP32</span></div>
    <dl class="grid gap-4 sm:grid-cols-2 xl:grid-cols-4"><div v-for="item in details" :key="item.label" class="sync-soft rounded-2xl p-4"><dt class="text-xs sync-muted">{{ item.label }}</dt><dd class="mt-2 break-words text-sm font-bold">{{ connected ? item.value : '—' }}</dd></div></dl>
  </section>
</template>

<script setup>
defineProps({ connected: Boolean, details: { type: Array, default: () => [] } })
</script>
