<template>
  <div class="dmbc-shell flex h-dvh overflow-hidden" :data-dark="isDark">
    <button v-if="mobileOpen" class="fixed inset-0 z-30 bg-slate-950/60 backdrop-blur-sm lg:hidden" aria-label="Close navigation" @click="mobileOpen = false"></button>
    <aside id="main-navigation" class="dmbc-sidebar fixed inset-y-0 left-0 z-40 flex w-72 shrink-0 flex-col border-r transition-transform duration-200 lg:static lg:translate-x-0" :class="mobileOpen ? 'translate-x-0' : '-translate-x-full'" @keydown.esc="mobileOpen = false">
      <div class="flex items-center gap-3 border-b border-inherit px-6 py-6">
        <img :src="'/images/dmbc-seal.png'" alt="DMBC seal" class="h-12 w-12 shrink-0 rounded-full bg-white p-0.5 shadow-sm" />
        <div class="min-w-0"><p class="text-xl font-extrabold tracking-tight">DMBC Water</p><p class="mt-0.5 text-[10px] font-semibold uppercase tracking-[.13em] text-teal-600">Water System Association</p></div>
        <button class="ml-auto lg:hidden" aria-label="Close navigation" @click="mobileOpen = false">×</button>
      </div>
      <nav aria-label="Main navigation" class="min-h-0 flex-1 space-y-7 overflow-y-auto px-4 py-6">
        <section v-for="section in navigation" :key="section.label">
          <p class="mb-3 px-3 text-[10px] font-bold uppercase tracking-[.2em] text-slate-400">{{ section.label }}</p>
          <div class="space-y-1.5">
            <div v-for="item in section.items" :key="item.label">
              <button v-if="item.children" type="button" class="dmbc-nav-item w-full" :class="{ 'is-section-active': isSectionActive(item) }" :aria-expanded="openGroups[item.label]" :aria-controls="'nav-' + item.key" @click="openGroups[item.label] = !openGroups[item.label]">
                <svg class="h-5 w-5 shrink-0" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" aria-hidden="true"><path :d="item.icon" stroke-linecap="round" stroke-linejoin="round"/></svg><span class="flex-1 text-left">{{ item.label }}</span>
                <svg class="h-4 w-4 transition-transform" :class="openGroups[item.label] ? 'rotate-90' : ''" viewBox="0 0 24 24" fill="none" stroke="currentColor" aria-hidden="true"><path d="m9 5 7 7-7 7" stroke-width="2"/></svg>
              </button>
              <RouterLink v-else :to="item.to" class="dmbc-nav-item" :class="{ 'is-active': isNavActive(item) }" :aria-current="route.path === item.to ? 'page' : undefined"><svg class="h-5 w-5 shrink-0" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" aria-hidden="true"><path :d="item.icon" stroke-linecap="round" stroke-linejoin="round"/></svg>{{ item.label }}</RouterLink>
              <div v-if="item.children && openGroups[item.label]" :id="'nav-' + item.key" class="ml-5 mt-2 space-y-1 border-l border-teal-500/20 pl-4">
                <RouterLink v-for="child in item.children" :key="child.to" :to="child.to" class="dmbc-nav-child" :class="{ 'is-active': route.path === child.to }" :aria-current="route.path === child.to ? 'page' : undefined"><span class="flex items-center gap-2"><svg v-if="child.icon" class="h-4 w-4 shrink-0 text-teal-600" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6" aria-hidden="true"><path :d="child.icon" stroke-linecap="round" stroke-linejoin="round"/></svg><span>{{ child.label }}</span></span><span v-if="child.description" class="mt-1 block text-[10px] font-normal leading-4 opacity-70">{{ child.description }}</span></RouterLink>
              </div>
            </div>
          </div>
        </section>
      </nav>
      <div class="space-y-3 border-t border-inherit p-4">
        <button @click="toggleTheme" class="dmbc-nav-item w-full" :aria-pressed="isDark"><svg class="h-5 w-5" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" aria-hidden="true"><path v-if="isDark" d="M12 3v2m0 14v2M3 12h2m14 0h2M5.6 5.6 7 7m10 10 1.4 1.4m0-12.8L17 7M7 17l-1.4 1.4M16 12a4 4 0 1 1-8 0 4 4 0 0 1 8 0Z"/><path v-else d="M20 15a8.5 8.5 0 0 1-11-11A8.5 8.5 0 1 0 20 15Z"/></svg><span>{{ isDark ? 'Light appearance' : 'Dark appearance' }}</span></button>
        <div class="flex items-center gap-3 rounded-2xl bg-teal-500/10 p-3">
          <img :src="'/images/dmbc-seal.png'" alt="" class="h-9 w-9 shrink-0 rounded-full bg-white p-0.5"/>
          <div class="min-w-0 flex-1"><p class="truncate text-sm font-bold">{{ user?.name || 'Admin' }}</p><p class="text-xs text-slate-400">Administrator</p></div>
          <button @click="logout" :disabled="loggingOut" class="rounded-lg p-2 text-teal-600 hover:bg-teal-500/10 disabled:opacity-50" aria-label="Sign out" title="Sign out"><svg class="h-5 w-5" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><path d="M9 4H5v16h4m5-12 4 4-4 4m-5-4h10" stroke-linecap="round" stroke-linejoin="round"/></svg></button>
        </div>
        <p v-if="logoutError" role="alert" class="text-xs text-red-500">{{ logoutError }}</p>
      </div>
    </aside>
    <div class="flex min-w-0 flex-1 flex-col">
      <header class="dmbc-mobile-header flex shrink-0 items-center gap-3 border-b p-4 lg:hidden"><button @click="mobileOpen = true" aria-controls="main-navigation" :aria-expanded="mobileOpen" aria-label="Open navigation" class="rounded-lg p-1 text-teal-600"><svg class="h-6 w-6" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M4 6h16M4 12h16M4 18h16"/></svg></button><span class="font-bold">DMBC Water</span></header>
      <main class="min-h-0 min-w-0 flex-1 overflow-y-auto"><slot :isDark="isDark" /></main>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, watch, onMounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { settingsSections } from '../config/settingsSections.js'

const route = useRoute()
const router = useRouter()
const user = ref(null)
const isDark = ref(localStorage.getItem('theme') === 'dark')
const mobileOpen = ref(false)
const loggingOut = ref(false)
const logoutError = ref('')
const openGroups = reactive({})
const navigation = [
  { label: 'Workspace', items: [
    { label: 'Dashboard', to: '/', icon: 'm3 10 9-7 9 7v10H3V10Zm6 10v-7h6v7' },
    { label: 'Community members', to: '/clients', icon: 'M16 21v-2a4 4 0 0 0-4-4H6a4 4 0 0 0-4 4v2m20 0v-2a4 4 0 0 0-3-3.87M13 3.13a4 4 0 0 1 0 7.75M13 7a4 4 0 1 1-8 0 4 4 0 0 1 8 0' },
    { label: 'Billing & collections', key: 'billing', prefix: '/billing', icon: 'M4 3h16v18l-4-2-4 2-4-2-4 2V3Zm4 5h8m-8 4h8', children: [
      { label: 'Overview', to: '/billing/summary' }, { label: 'Collect payment', to: '/billing/customer' }, { label: 'Bill register', to: '/billing/all-bills' }
    ] }
  ] },
  { label: 'Operations', items: [
    { label: 'Service incidents', to: '/incidents', icon: 'M12 3 2 21h20L12 3Zm0 6v5m0 3v.01' },
    { label: 'Device sync', key: 'sync', prefix: '/sync', icon: 'M20 7h-9m9 0-3-3m3 3-3 3M4 17h9m-9 0 3-3m-3 3 3 3M4 7a8 8 0 0 1 13-4M20 17a8 8 0 0 1-13 4', children: [{ label: 'Sync workspace', to: '/sync' }, { label: 'Date normalization', to: '/sync/normalize-dates' }] },
    { label: 'Settings', key: 'settings', prefix: '/settings', icon: 'M4 7h16M4 17h16M8 4v6m8 4v6', children: settingsSections },
    { label: 'Audit logs', to: '/audit-logs', icon: 'M5 3h14v18H5V3Zm4 5h6m-6 4h6m-6 4h4' }
  ] }
]
const isNavActive = item => route.path === item.to || (item.to === '/clients' && route.path.startsWith('/clients/'))
const isSectionActive = item => route.path.startsWith(item.prefix)
watch(() => route.path, () => {
  mobileOpen.value = false
  for (const section of navigation) for (const item of section.items) if (item.children && isSectionActive(item)) openGroups[item.label] = true
}, { immediate: true })
function toggleTheme() {
  isDark.value = !isDark.value
  localStorage.setItem('theme', isDark.value ? 'dark' : 'light')
}
async function logout() {
  if (loggingOut.value) return
  loggingOut.value = true
  logoutError.value = ''
  try {
    const response = await fetch('/logout', { method: 'POST', headers: { 'X-CSRF-TOKEN': document.querySelector('meta[name="csrf-token"]')?.getAttribute('content') || '' } })
    if (!response.ok) throw new Error('Sign out failed. Please try again.')
    router.push('/login')
  } catch (error) { logoutError.value = error.message } finally { loggingOut.value = false }
}
onMounted(async () => {
  try {
    const response = await fetch('/user')
    if (response.ok) user.value = (await response.json()).user
  } catch (error) { console.error('Failed to fetch user:', error) }
})
</script>

<style scoped>
.dmbc-shell { --nav-bg:#fff; --nav-text:#21443f; --nav-line:#e0ece8; background:#f5f8f7; color:var(--nav-text); }
.dmbc-shell[data-dark="true"] { --nav-bg:#10222c; --nav-text:#d0e7e4; --nav-line:#24414a; background:#020617; }
.dmbc-sidebar, .dmbc-mobile-header { background:var(--nav-bg); border-color:var(--nav-line); }
.dmbc-sidebar { box-shadow:4px 0 24px rgb(15 70 70 / 3%); }
.dmbc-nav-item { display:flex; align-items:center; gap:12px; border-radius:12px; padding:12px; font-size:14px; font-weight:600; color:var(--nav-text); transition:background .15s,color .15s; }
.dmbc-nav-item svg { color:#13968d; }
.dmbc-nav-item:hover, .dmbc-nav-child:hover { background:rgb(20 184 166 / 8%); }
.dmbc-nav-item.is-active { background:linear-gradient(110deg,#0f766e,#0e7490); color:white; box-shadow:0 4px 12px rgb(15 118 110 / 15%); }
.dmbc-nav-item.is-active svg { color:#bafff5; }
.dmbc-nav-item.is-section-active { background:rgb(20 184 166 / 8%); color:#0c9a8b; }
.dmbc-nav-child { display:block; border-radius:9px; padding:10px 12px; font-size:13px; color:var(--nav-text); }
.dmbc-nav-child.is-active { background:rgb(20 184 166 / 13%); color:#0d9488; font-weight:750; }
button:focus-visible, a:focus-visible { outline:3px solid #5eead4; outline-offset:2px; }
</style>
