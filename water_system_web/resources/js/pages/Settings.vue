<template>
  <AppSidebar v-slot="{ isDark }">
    <div class="settings-workspace min-h-full p-5 sm:p-8 lg:p-10" :data-dark="isDark">
      <header class="mb-7 flex flex-wrap items-center justify-between gap-5"><div><p class="settings-eyebrow mb-2">DMBC Water · Administration</p><h1 class="text-3xl font-extrabold tracking-tight sm:text-4xl">Association settings</h1><p class="mt-2 text-sm settings-muted">Manage service policies, water rates, and data protection.</p></div><div class="settings-panel hidden items-center gap-3 px-4 py-3 sm:flex"><img :src="'/images/dmbc-seal.png'" alt="DMBC seal" class="h-11 w-11 rounded-full bg-white p-0.5"/><div><p class="text-sm font-bold">DMBC Water</p><p class="mt-1 text-xs settings-muted">Makilas, Ipil, Zamboanga Sibugay</p></div></div></header>
      <nav aria-label="Settings sections" class="settings-panel mb-6 grid gap-2 p-2 md:grid-cols-3"><RouterLink v-for="section in settingsSections" :key="section.key" :to="section.to" :aria-current="activeSection === section.key ? 'page' : undefined" class="flex items-center gap-3 rounded-xl px-4 py-4 transition-colors" :class="activeSection === section.key ? 'bg-teal-700 text-white shadow-sm' : 'settings-muted hover:bg-teal-500/10'"><svg class="h-5 w-5 shrink-0" fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="1.6" aria-hidden="true"><path :d="section.icon" stroke-linecap="round" stroke-linejoin="round"/></svg><div><span class="block text-sm font-bold">{{ section.label }}</span><span class="mt-1 block text-xs opacity-75">{{ section.description }}</span></div></RouterLink></nav>
      <p v-if="notice" role="status" class="settings-feedback-success mb-6">{{ notice }}</p>
      <p v-if="navigationNotice" role="status" class="settings-feedback-error mb-6">{{ navigationNotice }}</p>
      <div class="grid items-start gap-6 2xl:grid-cols-[minmax(0,1fr)_260px]">
        <KeepAlive :key="cacheVersion" :max="3"><component :is="panels[activeSection]" :key="activeSection" :is-dark="isDark" @state-change="updateState" @restored="afterRestore" /></KeepAlive>
        <aside class="settings-panel p-6"><span class="mb-4 flex h-10 w-10 items-center justify-center rounded-xl bg-teal-500/10 text-teal-600"><svg class="h-5 w-5" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" aria-hidden="true"><path d="M12 8h.01M11 11h1v6m9-5a9 9 0 1 1-18 0 9 9 0 0 1 18 0Z"/></svg></span><h2 class="text-sm font-bold">{{ activeSection === 'backup' ? 'Recovery, with context' : 'From office to field' }}</h2><p class="mt-3 text-sm leading-6 settings-muted">{{ activeSection === 'backup' ? 'Downloads keep an archive on your laptop. Restore points stay on the web server and can replace its records with an earlier snapshot.' : 'Saved rate and device-policy changes are sent to your field master on its next sync. Your drafts stay here while you move between Settings sections.' }}</p><RouterLink v-if="activeSection !== 'backup'" to="/sync" class="mt-5 inline-block text-sm font-bold text-teal-600 hover:underline">Open device sync →</RouterLink><RouterLink v-else to="/audit-logs" class="mt-5 inline-block text-sm font-bold text-teal-600 hover:underline">View audit history →</RouterLink></aside>
      </div>
      <ConfirmDialog :is-open="showLeaveConfirm" :is-dark="isDark" title="Leave without saving?" message="Your unsaved Settings edits will be discarded when you leave this workspace." confirm-text="Discard and leave" cancel-text="Keep editing" @confirm="resolveLeave(true)" @cancel="resolveLeave(false)" />
    </div>
  </AppSidebar>
</template>

<script setup>
import { computed, ref, reactive, onMounted, onBeforeUnmount } from 'vue'
import { useRoute, onBeforeRouteLeave } from 'vue-router'
import AppSidebar from '../components/AppSidebar.vue'
import ConfirmDialog from '../components/ConfirmDialog.vue'
import BillingRulesPanel from '../components/settings/BillingRulesPanel.vue'
import CustomerCategoriesPanel from '../components/settings/CustomerCategoriesPanel.vue'
import DataBackupPanel from '../components/settings/DataBackupPanel.vue'
import { settingsSections, settingsSectionFromPath } from '../config/settingsSections.js'

const route = useRoute()
const panels = { billing: BillingRulesPanel, categories: CustomerCategoriesPanel, backup: DataBackupPanel }
const activeSection = computed(() => settingsSectionFromPath(route.path))
const states = reactive({})
const cacheVersion = ref(0)
const notice = ref('')
const navigationNotice = ref('')
const showLeaveConfirm = ref(false)
let leaveResolver
const dirty = computed(() => Object.values(states).some(state => state.dirty))
const busy = computed(() => Object.values(states).some(state => state.busy))
function updateState(state) { states[state.section] = state; if (!busy.value) navigationNotice.value = '' }
function afterRestore() {
  for (const key of Object.keys(states)) delete states[key]
  cacheVersion.value++
  notice.value = 'Web data restored successfully. Settings panels will load the restored values.'
}
function resolveLeave(allowed) {
  showLeaveConfirm.value = false
  const resolve = leaveResolver
  leaveResolver = null
  resolve?.(allowed)
}
onBeforeRouteLeave(to => {
  if (to.path.startsWith('/settings/')) return true
  if (busy.value) { navigationNotice.value = 'Wait for the current save or backup operation to finish before leaving.'; return false }
  if (!dirty.value) return true
  resolveLeave(false)
  showLeaveConfirm.value = true
  return new Promise(resolve => { leaveResolver = resolve })
})
function beforeUnload(event) { if (dirty.value || busy.value) { event.preventDefault(); event.returnValue = '' } }
onMounted(() => window.addEventListener('beforeunload', beforeUnload))
onBeforeUnmount(() => { window.removeEventListener('beforeunload', beforeUnload); resolveLeave(false) })
</script>
