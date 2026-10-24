<template>
  <AppSidebar v-slot="{ isDark }">
    <div class="billing-workspace min-h-full p-5 sm:p-8 lg:p-10" :data-dark="isDark" :class="isDark ? 'bg-slate-950 text-white' : 'bg-[#f5f8f7] text-slate-900'">
      <header class="mb-7 flex flex-wrap items-start justify-between gap-4">
        <div><p class="mb-2 text-xs font-bold uppercase tracking-[0.2em] text-teal-600">DMBC Water · Finance workspace</p><h1 class="text-3xl font-extrabold tracking-tight sm:text-4xl">Billing &amp; collections</h1><p class="mt-2 text-sm" :class="isDark ? 'text-slate-400' : 'text-slate-500'">A clearer view of collections, member accounts, and every payment.</p></div>
        <RouterLink to="/billing/customer" class="billing-primary inline-flex items-center gap-2 rounded-xl px-5 py-3 text-sm font-semibold text-white"><span aria-hidden="true">＋</span> Collect payment</RouterLink>
      </header>
      <nav aria-label="Billing workspace" class="billing-tabs mb-7 flex flex-wrap gap-2 rounded-2xl border p-2" :class="isDark ? 'bg-slate-900 border-slate-800' : 'bg-white border-slate-200'">
        <RouterLink v-for="tab in billingTabs" :key="tab.path" :to="tab.path" :aria-current="activeView === tab.view ? 'page' : undefined" class="flex-1 min-w-40 rounded-xl px-4 py-3 transition" :class="activeView === tab.view ? 'bg-teal-700 text-white shadow-sm' : (isDark ? 'text-slate-300 hover:bg-slate-800' : 'text-slate-600 hover:bg-teal-50')"><span class="block text-sm font-bold">{{ tab.label }}</span><span class="mt-0.5 block text-xs opacity-75">{{ tab.description }}</span></RouterLink>
      </nav>
      <!-- Billing Summary View -->
      <div v-if="activeView === 'summary'" class="mb-5 flex flex-wrap items-center justify-between gap-3 text-sm">
        <p v-if="statsError" role="alert" class="text-red-600">{{ statsError }} Displayed figures may be out of date.</p>
        <p v-else class="text-slate-500">{{ loadingStats ? 'Refreshing collection figures…' : 'Monthly collection overview and latest transactions' }}</p>
        <button type="button" @click="loadBillingStats" :disabled="loadingStats" class="rounded-xl border border-teal-200 px-4 py-2 font-semibold text-teal-600 disabled:opacity-50">{{ loadingStats ? 'Refreshing…' : 'Refresh overview' }}</button>
      </div>
      <BillingSummary
        v-if="activeView === 'summary'"
        :isDark="isDark"
        :todaysPayments="todaysPayments"
        :totalCollected="totalCollected"
        :pendingBillsCount="pendingBillsCount"
        :overdueBillsCount="overdueBillsCount"
        :transactions="transactions"
      />

      <!-- Customer Billing View -->
      <CustomerBilling
        v-if="activeView === 'customer'"
        :isDark="isDark"
        :selectedCustomer="selectedCustomer"
        :pendingBills="pendingBills"
        :loadingBills="loadingBills"
        :selectedBill="selectedBill"
        :processingPayment="processingPayment"
        :recentPayments="recentPayments"
        :loadingPayments="loadingPayments"
        @search-customer="handleSearchCustomer"
        @clear-customer="clearCustomer"
        @select-bill="selectBillForPayment"
        @process-payment="processPayment"
        @cancel-payment="cancelPayment"
      />

      <!-- All Bills View -->
      <AllBills
        v-if="activeView === 'all-bills'"
        :isDark="isDark"
        :statusFilter="statusFilter"
        :customerFilter="customerFilter"
        :loadingAllBills="loadingAllBills"
        :allBills="allBills"
        :pagination="pagination"
        :barangays="barangays"
        @filter-change="handleFilterChange"
        @pay-bill="payBill"
        @void-bill="voidBill"
        @print-bill="printBill"
        @download-monthly-report="downloadMonthlyReport"
        @go-to-page="goToPage"
      />
    </div>

    <!-- Payment Success Modal -->
    <PaymentReceipt
      :isDark="isDark"
      :showModal="showPaymentSuccess"
      :customerName="paymentReceipt?.customer_name || 'Customer'"
      :receipt="paymentReceipt"
      @close="closePaymentSuccess"
      @print="printReceipt"
    />

    <!-- One shared checkout form for both payment entry points. -->
    <div v-if="showPaymentModal" class="dmbc-modal fixed inset-0 z-50" :data-dark="isDark">
      <div class="absolute inset-0 bg-black/50" @click="closePaymentModal"></div>
      <div class="absolute inset-0 flex items-center justify-center p-4">
        <div role="dialog" aria-modal="true" aria-label="Record bill payment" class="dmbc-dialog w-full max-w-xl">
          <PaymentForm :is-dark="isDark" :selected-bill="selectedBill" :customer-name="selectedBill?.customer?.customer_name || 'Customer'" :processing-payment="processingPayment" @process-payment="processPayment" @cancel-payment="closePaymentModal" />
        </div>
      </div>
    </div>

    <!-- Void Bill Dialog -->
    <VoidBillDialog
      :is-open="showVoidDialog"
      message="Are you sure you want to void this bill? This action cannot be undone."
      :is-loading="voidLoading"
      :is-dark="isDark"
      @confirm="confirmVoid"
      @cancel="cancelVoid"
    />
  </AppSidebar>
</template>

<script>
import AppSidebar from '../components/AppSidebar.vue'
import BillingSummary from '../components/billing/BillingSummary.vue'
import CustomerBilling from '../components/billing/CustomerBilling.vue'
import AllBills from '../components/billing/AllBills.vue'
import PaymentForm from '../components/billing/PaymentForm.vue'
import PaymentReceipt from '../components/billing/PaymentReceipt.vue'
import VoidBillDialog from '../components/VoidBillDialog.vue'
import { useBilling } from '../composables/billing.js'
import { useRoute, useRouter } from 'vue-router'
import { computed, watch } from 'vue'

export default {
  name: 'Billing',
  components: {
    AppSidebar,
    BillingSummary,
    CustomerBilling,
    AllBills,
    PaymentForm,
    PaymentReceipt,
    VoidBillDialog
  },
  setup() {
    const route = useRoute()
    const router = useRouter()
    const billingComposable = useBilling()

    // Override activeTab with activeView for dropdown
    const activeView = computed({
      get: () => route.path.split('/')[2] || 'summary',
      set: (value) => router.push(`/billing/${value}`)
    })

    // Redirect /billing to /billing/summary
    if (route.path === '/billing') {
      router.replace('/billing/summary')
    }

    // Destructure initial loaders used by this page.
    const { loadAllBills, loadBarangays } = billingComposable

    // Watch for activeView changes to load data
    watch(activeView, (newView) => {
      if (newView === 'all-bills') {
        loadAllBills()
        loadBarangays()
      }
    })

    // Load all bills initially if on all-bills view
    if (activeView.value === 'all-bills') {
      loadAllBills()
      loadBarangays()
    }

    return {
      ...billingComposable,
      activeView,
      billingTabs: [
        { path: '/billing/summary', view: 'summary', label: 'Overview', description: 'Collections & recent activity' },
        { path: '/billing/customer', view: 'customer', label: 'Collect payment', description: 'Find member · select bill · pay' },
        { path: '/billing/all-bills', view: 'all-bills', label: 'Bill register', description: 'Search, review & export' },
      ]
    }
  }
}
</script>
