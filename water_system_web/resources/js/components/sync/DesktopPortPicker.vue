<template>
  <Teleport to="body"><div v-if="request" class="dmbc-modal fixed inset-0 z-[100] flex items-center justify-center bg-slate-950/50 p-4" @click.self="cancel">
    <section role="dialog" aria-modal="true" aria-labelledby="usb-picker-title" class="dmbc-dialog w-full max-w-md rounded-3xl bg-white p-6 shadow-xl" @keydown.esc="cancel">
      <p class="text-xs font-bold uppercase tracking-widest text-teal-600">DMBC device connection</p>
      <h2 id="usb-picker-title" class="mt-2 text-xl font-bold text-slate-900">Choose your ESP32</h2>
      <p class="mt-2 text-sm text-slate-500">Select the USB port shown for your master device.</p>
      <label class="mt-5 block text-sm font-semibold text-slate-700">USB port<select ref="select" v-model="selected" class="mt-2 w-full rounded-xl border border-teal-200 bg-white p-3 text-slate-900"><option v-for="port in request.ports" :key="port" :value="port">{{ port }}</option></select></label>
      <div class="mt-6 flex justify-end gap-3"><button class="rounded-xl border border-slate-200 px-4 py-2 text-sm font-semibold text-slate-600" @click="cancel">Cancel</button><button class="rounded-xl bg-teal-700 px-4 py-2 text-sm font-semibold text-white" :disabled="!selected" @click="confirm">Connect device</button></div>
    </section>
  </div></Teleport>
</template>
<script setup>
import { ref, onMounted, onBeforeUnmount, nextTick } from 'vue';
const request = ref(null), selected = ref(''), select = ref(null);
function cancel() { request.value?.reject(new Error('USB connection cancelled.')); request.value = null; }
function confirm() { request.value?.resolve(selected.value); request.value = null; }
async function choose(event) { cancel(); request.value = event.detail; selected.value = event.detail.ports[0] || ''; await nextTick(); select.value?.focus(); }
onMounted(() => globalThis.addEventListener('dmbc:choose-port', choose));
onBeforeUnmount(() => { cancel(); globalThis.removeEventListener('dmbc:choose-port', choose); });
</script>
