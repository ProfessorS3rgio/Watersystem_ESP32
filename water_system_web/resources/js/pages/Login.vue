<template>
  <div class="relative min-h-screen overflow-hidden bg-[#edf7f6] px-4 py-10 sm:px-6 lg:px-8">
    <div class="absolute -left-24 -top-24 h-80 w-80 rounded-full bg-teal-200/60 blur-3xl"></div><div class="absolute -bottom-24 -right-20 h-96 w-96 rounded-full bg-cyan-200/60 blur-3xl"></div>
    <div class="relative mx-auto flex min-h-[calc(100vh-5rem)] w-full max-w-5xl items-center justify-center">
      <div class="grid w-full overflow-hidden rounded-[2rem] bg-white shadow-2xl shadow-teal-950/10 lg:grid-cols-[.9fr_1.1fr]">
        <aside class="relative hidden overflow-hidden bg-gradient-to-br from-teal-900 via-teal-800 to-cyan-700 p-10 text-white lg:flex lg:flex-col lg:justify-between"><div class="absolute -right-20 -top-20 h-64 w-64 rounded-full border-[26px] border-white/10"></div><div class="relative"><div class="flex items-center gap-3"><img :src="'/images/dmbc-seal.png'" alt="DMBC seal" class="h-12 w-12 rounded-full bg-white p-1 object-contain" /><div><p class="text-lg font-extrabold">DMBC Water</p><p class="text-[11px] font-semibold uppercase tracking-[0.15em] text-teal-100">Water System Association</p></div></div><div class="mt-20"><p class="text-sm font-semibold uppercase tracking-[0.2em] text-teal-100">Doña Josefa Makilas Buluan Caparan</p><h1 class="mt-4 text-4xl font-extrabold leading-tight">Serving every household, one drop at a time.</h1><p class="mt-5 max-w-sm text-sm leading-6 text-teal-50">A simple, secure workspace for your association’s member records, billing, and service monitoring.</p></div></div><p class="relative text-xs text-teal-100">Makilas, Ipil, Zamboanga Sibugay</p></aside>
        <main class="p-7 sm:p-10">
        <div class="text-center lg:text-left"><div class="mx-auto mb-5 flex h-16 w-16 items-center justify-center rounded-full bg-teal-50 p-1 ring-4 ring-teal-100 lg:hidden"><img :src="'/images/dmbc-seal.png'" alt="DMBC seal" class="h-full w-full rounded-full object-contain" /></div><p class="text-xs font-bold uppercase tracking-[0.2em] text-teal-700">Secure administrator access</p><h2 class="mt-3 text-3xl font-extrabold tracking-tight text-slate-900">Welcome back</h2><p class="mt-2 text-sm text-slate-500">Sign in to manage the DMBC water system.</p></div>

        <form class="mt-8 space-y-6" @submit.prevent="handleLogin">
          <div class="space-y-4">
            <div>
              <label for="email" class="block text-sm font-medium text-gray-700 mb-1">
                Email Address
              </label>
              <div class="relative">
                <input
                  id="email"
                  name="email"
                  type="email"
                  autocomplete="email"
                  required
                  class="block w-full rounded-xl border border-slate-200 bg-slate-50 px-4 py-3 text-slate-900 outline-none transition placeholder:text-slate-400 focus:border-teal-600 focus:bg-white focus:ring-4 focus:ring-teal-100"
                  placeholder="Enter your email"
                  v-model="form.email"
                />
                <svg class="absolute right-3 top-3.5 h-5 w-5 text-teal-600" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                  <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M16 12a4 4 0 10-8 0 4 4 0 008 0zm0 0v1.5a2.5 2.5 0 005 0V12a9 9 0 10-9 9m4.5-1.206a8.959 8.959 0 01-4.5 1.207" />
                </svg>
              </div>
            </div>

            <div>
              <label for="password" class="block text-sm font-medium text-gray-700 mb-1">
                Password
              </label>
              <div class="relative">
                <input
                  id="password"
                  name="password"
                  type="password"
                  autocomplete="current-password"
                  required
                  class="block w-full rounded-xl border border-slate-200 bg-slate-50 px-4 py-3 text-slate-900 outline-none transition placeholder:text-slate-400 focus:border-teal-600 focus:bg-white focus:ring-4 focus:ring-teal-100"
                  placeholder="Enter your password"
                  v-model="form.password"
                />
                <svg class="absolute right-3 top-3.5 h-5 w-5 text-teal-600" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                  <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 15v2m-6 4h12a2 2 0 002-2v-6a2 2 0 00-2-2H6a2 2 0 00-2 2v6a2 2 0 002 2zm10-10V7a4 4 0 00-8 0v4h8z" />
                </svg>
              </div>
            </div>
          </div>

          <div v-if="error" class="bg-red-50 border border-red-200 rounded-lg p-4">
            <div class="flex">
              <svg class="h-5 w-5 text-red-400 mr-2" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 8v4m0 4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
              </svg>
              <p class="text-sm text-red-700">{{ error }}</p>
            </div>
          </div>

          <div>
            <button
              type="submit"
              :disabled="loading"
              class="group relative flex w-full justify-center rounded-xl border border-transparent bg-gradient-to-r from-teal-700 to-cyan-700 px-4 py-3 text-sm font-bold text-white shadow-lg shadow-teal-700/20 transition hover:from-teal-800 hover:to-cyan-800 focus:outline-none focus:ring-4 focus:ring-teal-200 disabled:cursor-not-allowed disabled:opacity-50"
            >
              <span v-if="loading" class="animate-spin rounded-full h-4 w-4 border-b-2 border-white mr-2"></span>
              {{ loading ? 'Signing in...' : 'Sign In' }}
            </button>
          </div>

          <div class="text-center">
            <p class="text-sm text-slate-500">
              <template v-if="desktop">Desktop account: <strong>admin@dmbc.local</strong><br>Use the password in <strong>initial-login.txt</strong> in your DMBC data folder.</template>
              <template v-else>Use your association’s administrator account to sign in.</template>
            </p>
          </div>
        </form>
        </main>
      </div>
    </div>
  </div>
</template>

<script>
import { signIn } from '../services/authApi.js'

export default {
  name: 'Login',
  data() {
    return {
      desktop: Boolean(globalThis.__TAURI_INTERNALS__),
      form: {
        email: '',
        password: ''
      },
      error: null,
      loading: false
    }
  },
  methods: {
    async handleLogin() {
      this.loading = true
      this.error = null

      try {
        await signIn(this.form)
        await this.$router.push('/')
      } catch (error) {
        this.error = error instanceof TypeError ? 'Cannot reach DMBC. Close and reopen the application, then try again.' : error.message || 'Unable to sign in. Please try again.'
      } finally {
        this.loading = false
      }
    }
  }
}
</script>
