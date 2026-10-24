<template>
  <section class="sync-panel overflow-hidden">
    <div class="flex flex-wrap items-center justify-between gap-3 p-5"><div><h2 class="text-lg font-bold">Activity log</h2><p class="mt-1 text-xs sync-muted">Connection events and device responses · {{ logs.length }} entries</p></div><div class="flex items-center gap-3 text-xs"><label class="flex items-center gap-2"><input v-model="follow" type="checkbox"/> Follow latest</label><button @click="$emit('clear')" :disabled="busy" class="sync-secondary disabled:opacity-50">Clear log</button></div></div>
    <div ref="output" tabindex="0" aria-label="Device activity log" class="h-64 overflow-auto bg-slate-950 p-5 font-mono text-xs leading-6 text-cyan-100"><p v-for="(log, index) in logs" :key="index" class="break-words" :class="/failed|error|ERR\|/i.test(log.message) ? 'text-rose-300' : ''"><span class="mr-2 text-slate-500">{{ log.time }}</span>{{ log.message }}</p><p v-if="!logs.length" class="text-slate-400">Device activity will appear here.</p></div>
  </section>
</template>

<script setup>
import { ref, watch, nextTick } from 'vue'
const props = defineProps({ logs: { type: Array, default: () => [] }, busy: Boolean })
defineEmits(['clear'])
const follow = ref(true)
const output = ref(null)
watch(() => props.logs.length, async () => {
  if (!follow.value) return
  await nextTick()
  if (output.value) output.value.scrollTop = output.value.scrollHeight
})
</script>
