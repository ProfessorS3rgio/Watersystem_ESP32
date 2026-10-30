<template>
  <AppSidebar v-slot="{ isDark }">
    <div class="min-h-full p-5 sm:p-8 lg:p-10" :class="isDark ? 'bg-slate-950 text-white' : 'bg-[#f5f8f7] text-slate-900'">
      <div class="mb-8 flex flex-col gap-5 sm:flex-row sm:items-end sm:justify-between">
        <div><p class="mb-2 text-xs font-bold uppercase tracking-[0.22em] text-teal-700">Doña Josefa Makilas Buluan Caparan Water System Association</p>
        <h1 class="text-3xl font-extrabold tracking-tight sm:text-4xl transition-colors duration-200" :class="isDark ? 'text-white' : 'text-slate-900'">Community members</h1>
        <p class="mt-2 transition-colors duration-200" :class="isDark ? 'text-slate-400' : 'text-slate-500'">Manage household accounts, service status, and water usage across Makilas, Ipil.</p></div>
        <div class="hidden items-center gap-3 rounded-2xl border px-4 py-3 shadow-sm sm:flex" :class="isDark ? 'border-slate-700 bg-slate-900' : 'border-slate-200 bg-white'"><img :src="'/images/dmbc-seal.png'" alt="DMBC seal" class="h-10 w-10 rounded-full object-contain"/><div><p class="text-xs font-semibold uppercase tracking-wider text-slate-400">Association</p><p class="text-sm font-bold text-teal-700">DMBC Water</p></div></div>
      </div>

      <!-- Add Client Button -->
     

    <!-- Stats Cards -->
    <StatsCards
      :customers="customers"
      :barangays="barangays"
      :selectedBarangay="selectedBarangay"
      :isDark="isDark"
      @select-barangay="selectedBarangay = $event"
    />

      <div class="mb-6 grid grid-cols-1 gap-4 sm:grid-cols-3">
      <div class="rounded-2xl border px-5 py-5 shadow-sm transition hover:shadow-md" :class="isDark ? 'bg-slate-900 border-slate-800' : 'bg-white border-white'">
        <div class="flex items-center gap-4">
            <div class="flex h-11 w-11 shrink-0 items-center justify-center rounded-xl bg-amber-50 text-amber-700">
              <svg class="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 8c-2.21 0-4 1.12-4 2.5S9.79 13 12 13s4 1.12 4 2.5S14.21 18 12 18m0-10V6m0 12v-2m8-4a8 8 0 11-16 0 8 8 0 0116 0z" />
            </svg>
          </div>
          <div>
            <p class="text-xs uppercase tracking-wide" :class="isDark ? 'text-gray-400' : 'text-gray-500'">Accounts with balance</p>
            <p class="mt-1 text-2xl font-bold" :class="isDark ? 'text-amber-300' : 'text-amber-600'">{{ displayedAccountsWithBalance }}</p>
          </div>
        </div>
      </div>
      <div class="rounded-2xl border px-5 py-5 shadow-sm transition hover:shadow-md" :class="isDark ? 'bg-slate-900 border-slate-800' : 'bg-white border-white'">
        <div class="flex items-center gap-4">
            <div class="flex h-11 w-11 shrink-0 items-center justify-center rounded-xl bg-rose-50 text-rose-600">
              <svg class="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 8c-2.21 0-4 1.12-4 2.5S9.79 13 12 13s4 1.12 4 2.5S14.21 18 12 18m0-10V6m0 12v-2m8-4a8 8 0 11-16 0 8 8 0 0116 0z" />
            </svg>
          </div>
          <div>
            <p class="text-xs uppercase tracking-wide" :class="isDark ? 'text-gray-400' : 'text-gray-500'">Outstanding balance</p>
            <p class="mt-1 text-2xl font-bold" :class="isDark ? 'text-red-300' : 'text-red-600'">₱{{ formatWholePesos(displayedOutstandingBalance) }}</p>
          </div>
        </div>
      </div>
      <div class="rounded-2xl border px-5 py-5 shadow-sm transition hover:shadow-md" :class="isDark ? 'bg-slate-900 border-slate-800' : 'bg-white border-white'">
        <div class="flex items-center gap-4">
            <div class="flex h-11 w-11 shrink-0 items-center justify-center rounded-xl bg-teal-50 text-teal-700">
            <svg class="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M17 20h5v-2a3 3 0 00-5.36-1.86M17 20H7m10 0v-2c0-.66-.13-1.28-.36-1.86M7 20H2v-2a3 3 0 015.36-1.86M7 20v-2a5 5 0 019.29 0M15 7a3 3 0 11-6 0 3 3 0 016 0z" />
            </svg>
          </div>
          <div>
            <p class="text-xs uppercase tracking-wide" :class="isDark ? 'text-gray-400' : 'text-gray-500'">Customers shown</p>
            <p class="mt-1 text-2xl font-extrabold" :class="isDark ? 'text-teal-300' : 'text-teal-700'">{{ displayedCustomersShown }}</p>
          </div>
        </div>
      </div>
    </div>

    <div class="mb-6 flex flex-wrap gap-3">
      <button @click="openAddModal" class="bg-gradient-to-r from-teal-700 to-cyan-700 hover:from-teal-800 hover:to-cyan-800 text-white px-6 py-3 rounded-xl font-semibold shadow-lg shadow-teal-700/15 transition-all duration-200 flex items-center space-x-2">
        <svg class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
          <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 6v6m0 0v6m0-6h6m-6 0H6" />
        </svg>
        <span>Add New Client</span>
      </button>
      <button @click="isReadingImportOpen = true" class="bg-white border border-teal-200 hover:bg-teal-50 text-teal-800 px-6 py-3 rounded-xl font-semibold shadow-sm transition-all duration-200">
        Import Readings
      </button>
    </div>
    <!-- Clients Table -->
    <div class="rounded-3xl border shadow-sm overflow-hidden" :class="isDark ? 'bg-slate-900 border-slate-800' : 'bg-white border-white'">
      <div class="px-5 py-5 border-b flex flex-col gap-4 lg:flex-row lg:items-center lg:justify-between" :class="isDark ? 'border-slate-800' : 'border-slate-100'">
        <div><h2 class="text-lg font-bold" :class="isDark ? 'text-white' : 'text-slate-900'">Member directory</h2><p class="mt-1 text-sm" :class="isDark ? 'text-slate-400' : 'text-slate-500'">{{ filteredCustomers.length }} accounts match your filters</p></div>
        <div class="flex flex-wrap items-center gap-3">
          <span class="text-sm" :class="isDark ? 'text-gray-300' : 'text-gray-600'">
            {{ filteredCustomers.length }} found
          </span>
          <input
            type="search"
            placeholder="Search name, account, or meter…"
            v-model="search"
            class="min-w-52 rounded-xl border px-4 py-2.5 text-sm outline-none focus:border-teal-600 focus:ring-4 focus:ring-teal-100"
            :class="isDark ? 'bg-slate-800 border-slate-700 text-white placeholder-slate-400' : 'bg-slate-50 border-slate-200 text-slate-900 placeholder-slate-400'"
          />
          <select
            v-model="statusFilter"
            class="rounded-xl border px-3 py-2.5 text-sm outline-none focus:border-teal-600 focus:ring-4 focus:ring-teal-100"
            :class="isDark ? 'bg-slate-800 border-slate-700 text-white' : 'bg-slate-50 border-slate-200 text-slate-900'"
          >
            <option value="all">All Status</option>
            <option value="active">Active</option>
            <option value="disconnected">Disconnected</option>
          </select>
          <select
            v-model="billFilter"
            class="rounded-xl border px-3 py-2.5 text-sm outline-none focus:border-teal-600 focus:ring-4 focus:ring-teal-100"
            :class="isDark ? 'bg-slate-800 border-slate-700 text-white' : 'bg-slate-50 border-slate-200 text-slate-900'"
          >
            <option value="all">All Bills</option>
            <option value="pending">Pending</option>
            <option value="partial">Partially Paid</option>
            <option value="due">Due</option>
            <option value="paid">Paid</option>
            <option value="none">No Bill</option>
          </select>
        </div>
      </div>
      <div class="overflow-x-auto">
        <table class="w-full">
          <thead :class="isDark ? 'bg-slate-800/80' : 'bg-teal-50/70'">
            <tr>
              <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-300' : 'text-gray-500'">Account No</th>
              <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-300' : 'text-gray-500'">Customer Name</th>
              <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-300' : 'text-gray-500'">Previous</th>
              <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-300' : 'text-gray-500'">Present</th>
              <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-300' : 'text-gray-500'">Latest Bill</th>
              <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-300' : 'text-gray-500'">Balance</th>
              <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-300' : 'text-gray-500'">Status</th>
              <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-300' : 'text-gray-500'">Usage</th>
              <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider w-48" :class="isDark ? 'text-gray-300' : 'text-gray-500'">Actions</th>
            </tr>
          </thead>
          <tbody :class="isDark ? 'bg-slate-900 divide-slate-800' : 'bg-white divide-slate-100'" class="divide-y">
            <tr v-if="isLoading">
              <td class="px-6 py-6 text-sm" :class="isDark ? 'text-gray-300' : 'text-gray-600'" colspan="9">Loading customers...</td>
            </tr>
            <tr v-if="errorMessage">
              <td class="px-6 py-6 text-sm text-red-600" colspan="9">{{ errorMessage }}</td>
            </tr>
            <tr v-else-if="filteredCustomers.length === 0">
              <td class="px-6 py-6 text-sm" :class="isDark ? 'text-gray-300' : 'text-gray-600'" colspan="9">No customers found.</td>
            </tr>
            <tr v-else v-for="customer in filteredCustomers" :key="customer.id" class="transition-colors" :class="isDark ? 'hover:bg-slate-800/70' : 'hover:bg-teal-50/40'">
              <td class="px-4 py-3 whitespace-nowrap text-sm font-bold" :class="isDark ? 'text-teal-300' : 'text-teal-800'">{{ customer.account_no }}</td>
              <td class="px-4 py-2.5 whitespace-nowrap">
                <div class="flex items-center">
                  <div class="h-8 w-8 rounded-full flex items-center justify-center" :class="isDark ? 'bg-teal-900/70' : 'bg-teal-100'">
                    <span class="font-semibold text-xs" :class="isDark ? 'text-teal-200' : 'text-teal-700'">{{ (customer.customer_name || '?').charAt(0) }}</span>
                  </div>
                  <div class="ml-2">
                    <RouterLink :to="`/clients/${customer.id}/profile`" class="text-sm font-semibold hover:text-teal-500 hover:underline" :class="isDark ? 'text-white' : 'text-gray-900'">{{ customer.customer_name }}</RouterLink>
                    <p v-if="customer.meter_number" class="mt-1 text-xs opacity-60">Meter {{ customer.meter_number }}</p>
                  </div>
                </div>
              </td>
              <td class="px-4 py-2.5 whitespace-nowrap text-sm font-semibold" :class="isDark ? 'text-slate-300' : 'text-slate-600'">{{ customer.previous_reading }} m³</td>
              <td class="px-4 py-2.5 whitespace-nowrap text-sm font-semibold" :class="isDark ? 'text-cyan-300' : 'text-cyan-700'">{{ customer.current_reading ?? '-' }}{{ customer.current_reading ? ' m³' : '' }}</td>
              <td class="px-4 py-2.5 whitespace-nowrap text-sm">
                <span v-if="customer.latest_bill_state === 'paid'" class="px-2.5 py-0.5 rounded-full text-xs font-medium" :class="isDark ? 'bg-green-900 text-green-200' : 'bg-green-100 text-green-800'">Paid</span>
                <span v-else-if="customer.latest_bill_state === 'due'" class="px-2.5 py-0.5 rounded-full text-xs font-medium" :class="isDark ? 'bg-red-900 text-red-200' : 'bg-red-100 text-red-800'">Due</span>
                <span v-else-if="customer.latest_bill_state === 'partial'" class="px-2.5 py-0.5 rounded-full text-xs font-medium" :class="isDark ? 'bg-orange-900 text-orange-200' : 'bg-orange-100 text-orange-800'">Partially Paid</span>
                <span v-else-if="customer.latest_bill_state === 'pending'" class="px-2.5 py-0.5 rounded-full text-xs font-medium" :class="isDark ? 'bg-yellow-900 text-yellow-200' : 'bg-yellow-100 text-yellow-800'">Pending</span>
                <span v-else class="px-2.5 py-0.5 rounded-full text-xs font-medium" :class="isDark ? 'bg-gray-700 text-gray-200' : 'bg-gray-100 text-gray-700'">No Bill</span>
              </td>
              <td class="px-4 py-2.5 whitespace-nowrap text-sm font-semibold" :class="getCustomerAmountClass(customer, isDark)">
                ₱{{ formatWholePesos(displayCustomerAmount(customer)) }}
              </td>
              <td class="px-4 py-2.5 whitespace-nowrap">
                <span :class="customer.status === 'active' ? (isDark ? 'bg-green-900 text-green-200' : 'bg-green-100 text-green-800') : (isDark ? 'bg-red-900 text-red-200' : 'bg-red-100 text-red-800')" class="px-2.5 py-0.5 rounded-full text-xs font-medium">
                  {{ customer.status === 'active' ? 'Active' : 'Disconnected' }}
                </span>
              </td>
              <td class="px-4 py-2.5 whitespace-nowrap text-sm">
                <button @click="openUsageModal(customer)" class="font-semibold text-xs hover:underline" :class="isDark ? 'text-cyan-300 hover:text-cyan-200' : 'text-teal-700 hover:text-cyan-700'">
                  View Usage
                </button>
              </td>
              <td class="px-4 py-2.5 whitespace-nowrap text-sm">
                <select :aria-label="`Actions for ${customer.customer_name}`" @change="handleMemberAction($event, customer)" class="w-44 rounded-xl border px-3 py-2 text-xs font-bold outline-none focus:ring-2 focus:ring-teal-500/25" :class="isDark ? 'border-teal-700 bg-slate-900 text-teal-200' : 'border-teal-200 bg-teal-50 text-teal-800'">
                  <option value="">Actions</option>
                  <option value="profile">Member profile</option>
                  <option value="summary">Account summary</option>
                  <option value="edit">Edit account</option>
                  <option value="usage">Usage history</option>
                  <option value="incident">Report service issue</option>
                  <option :value="customer.status === 'active' ? 'disconnect' : 'reconnect'">{{ customer.status === 'active' ? 'Disconnect member' : 'Reconnect member' }}</option>
                  <option value="swap">Swap account numbers</option>
                  <option value="remove">Remove member</option>
                </select>
              </td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>

    <div v-if="isViewModalOpen" :data-dark="isDark" class="dmbc-modal fixed inset-0 z-50">
      <div class="absolute inset-0 bg-black/50" @click="closeViewModal"></div>
      <div class="absolute inset-0 flex items-center justify-center p-4">
        <div class="dmbc-dialog w-full max-w-2xl rounded-xl shadow-2xl" :class="isDark ? 'bg-gray-900 text-white' : 'bg-white text-gray-900'">
          <div class="dmbc-dialog-header px-6 py-4 border-b" :class="isDark ? 'border-gray-800' : 'border-gray-200'">
            <div class="flex items-start justify-between gap-4">
              <div>
                <h3 class="text-lg font-semibold">Client Details</h3>
                <p class="text-sm mt-1" :class="isDark ? 'text-gray-300' : 'text-gray-600'">
                  {{ viewingCustomer?.customer_name || '-' }} • <span class="font-semibold" :class="isDark ? 'text-teal-300' : 'text-teal-700'">{{ viewingCustomer?.account_no || '-' }}</span>
                </p>
              </div>
              <span
                :class="viewingCustomer?.status === 'active' ? (isDark ? 'bg-green-900 text-green-200' : 'bg-green-100 text-green-800') : (isDark ? 'bg-red-900 text-red-200' : 'bg-red-100 text-red-800')"
                class="px-2.5 py-1 rounded-full text-xs font-medium"
              >
                {{ viewingCustomer?.status === 'active' ? 'Active' : 'Disconnected' }}
              </span>
            </div>
          </div>

          <div class="px-6 py-5">
            <div class="grid grid-cols-1 md:grid-cols-2 gap-3">
              <div class="rounded-lg p-3" :class="isDark ? 'bg-gray-800/80' : 'bg-gray-50'">
                <p class="text-xs uppercase tracking-wide" :class="isDark ? 'text-gray-400' : 'text-gray-500'">Type</p>
                <p class="text-sm font-medium mt-1">{{ getTypeName(viewingCustomer?.type_id) }}</p>
              </div>
              <div class="rounded-lg p-3" :class="isDark ? 'bg-gray-800/80' : 'bg-gray-50'">
                <p class="text-xs uppercase tracking-wide" :class="isDark ? 'text-gray-400' : 'text-gray-500'">Barangay</p>
                <p class="text-sm font-medium mt-1">{{ getBarangayName(viewingCustomer?.brgy_id) }}</p>
              </div>
              <div class="rounded-lg p-3" :class="isDark ? 'bg-gray-800/80' : 'bg-gray-50'">
                <p class="text-xs uppercase tracking-wide" :class="isDark ? 'text-gray-400' : 'text-gray-500'">Previous</p>
                <p class="text-sm font-semibold mt-1" :class="isDark ? 'text-blue-300' : 'text-blue-600'">{{ viewingCustomer?.previous_reading ?? '-' }} m³</p>
              </div>
              <div class="rounded-lg p-3" :class="isDark ? 'bg-gray-800/80' : 'bg-gray-50'">
                <p class="text-xs uppercase tracking-wide" :class="isDark ? 'text-gray-400' : 'text-gray-500'">Present</p>
                <p class="text-sm font-semibold mt-1" :class="isDark ? 'text-green-300' : 'text-green-600'">{{ viewingCustomer?.current_reading ?? '-' }}{{ viewingCustomer?.current_reading ? ' m³' : '' }}</p>
              </div>
            </div>

            <div class="grid grid-cols-1 md:grid-cols-2 gap-3 mt-3">
              <div class="rounded-lg p-3" :class="isDark ? 'bg-gray-800/80' : 'bg-gray-50'">
                <p class="text-xs uppercase tracking-wide" :class="isDark ? 'text-gray-400' : 'text-gray-500'">Address</p>
                <p class="text-sm mt-1">{{ viewingCustomer?.address || '-' }}</p>
              </div>
              <div class="rounded-lg p-3" :class="isDark ? 'bg-gray-800/80' : 'bg-gray-50'">
                <p class="text-xs uppercase tracking-wide" :class="isDark ? 'text-gray-400' : 'text-gray-500'">Deduction</p>
                <p class="text-sm mt-1">{{ getDeductionName(viewingCustomer?.deduction_id) }}</p>
              </div>
            </div>
          </div>

          <div class="px-6 py-4 border-t flex justify-end" :class="isDark ? 'border-gray-800' : 'border-gray-200'">
            <button
              @click="closeViewModal"
              class="px-4 py-2 rounded-lg font-medium"
              :class="isDark ? 'bg-gray-700 hover:bg-gray-600 text-white' : 'bg-gray-100 hover:bg-gray-200 text-gray-900'"
            >
              Close
            </button>
          </div>
        </div>
      </div>
    </div>

    <!-- Usage Modal -->
    <UsageModal
      :is-open="isUsageModalOpen"
      :customer="usageCustomer"
      :readings="usageReadings"
      :loading="usageLoading"
      :error="usageError"
      :is-dark="isDark"
      @close="closeUsageModal"
      @mark-paid="markBillPaid"
      @print-receipt="printUsageReceipt"
      @void-bill="voidBill"
    />

    <!-- Client Form -->
    <ReadingImportModal
      :is-open="isReadingImportOpen"
      :is-dark="isDark"
      @close="isReadingImportOpen = false"
      @imported="handleReadingImportCompleted"
    />

    <ClientForm
      :is-add-modal-open="isAddModalOpen"
      :is-edit-modal-open="isEditModalOpen"
      :is-dark="isDark"
      :new-customer="newCustomer"
      :edit-customer="editCustomer"
      :customer-types="customerTypes"
      :barangays="barangays"
      :deductions="deductions"
      :is-submitting="isSubmitting"
      :submit-error="submitError"
      :preview-account-no-new="previewAccountNoNew"
      :field-error="fieldError"
      @close-add-modal="closeAddModal"
      @close-edit-modal="closeEditModal"
      @submit-new-customer="submitNewCustomer"
      @submit-edit-customer="submitEditCustomer"
    />

    <!-- Delete Confirmation Dialog -->
    <ConfirmDialog
      :is-open="isDeleteModalOpen"
      title="Delete Client"
      :message="`You are about to delete <strong>${deletingCustomer?.customer_name}</strong> (${deletingCustomer?.account_no}).<br><br>This action cannot be undone and will permanently remove all associated data.`"
      confirm-text="Delete Client"
      cancel-text="Cancel"
      :is-loading="isSubmitting"
      :is-dark="isDark"
      @confirm="confirmDelete"
      @cancel="closeDeleteModal"
    />

    <!-- Swap Account Number Modal -->
    <div v-if="isSwapModalOpen" :data-dark="isDark" class="dmbc-modal fixed inset-0 z-50">
      <div class="absolute inset-0 bg-black/50" @click="closeSwapModal"></div>
      <div class="absolute inset-0 flex items-center justify-center p-4">
        <div class="dmbc-dialog w-full max-w-xl rounded-lg shadow-lg" :class="isDark ? 'bg-gray-900 text-white' : 'bg-white text-gray-900'">
          <div class="dmbc-dialog-header px-6 py-4 border-b" :class="isDark ? 'border-gray-800' : 'border-gray-200'">
            <div class="flex items-center justify-between">
              <div><h3 class="text-lg font-semibold">Swap Account Numbers</h3><p class="mt-1 text-sm">Exchange account numbers between two members.</p></div>
              <button class="text-sm" :class="isDark ? 'text-gray-300 hover:text-white' : 'text-gray-600 hover:text-gray-900'" @click="closeSwapModal">
                Close
              </button>
            </div>
          </div>

          <div class="px-6 py-5 space-y-4">
            <div class="rounded-lg p-3" :class="isDark ? 'bg-gray-800/80' : 'bg-gray-50'">
              <p class="text-xs uppercase tracking-wide" :class="isDark ? 'text-gray-400' : 'text-gray-500'">From</p>
              <p class="text-sm font-medium mt-1">
                {{ swapSourceCustomer?.customer_name || '-' }}
                <span class="ml-2 text-xs font-semibold" :class="isDark ? 'text-teal-300' : 'text-teal-700'">
                  {{ swapSourceCustomer?.account_no || '-' }}
                </span>
              </p>
            </div>

            <div>
              <label class="block text-sm font-medium mb-1">Swap With</label>
              <select
                v-model="swapTargetCustomerId"
                class="w-full px-4 py-2 rounded-lg border"
                :class="inputClass(isDark)"
              >
                <option :value="null">Select customer</option>
                <option v-for="customer in swapTargets" :key="customer.id" :value="customer.id">
                  {{ customer.customer_name }} ({{ customer.account_no }})
                </option>
              </select>
            </div>

            <p v-if="swapError" class="text-sm text-red-600">{{ swapError }}</p>
          </div>

          <div class="px-6 py-4 border-t flex items-center justify-end gap-3" :class="isDark ? 'border-gray-800' : 'border-gray-200'">
            <button
              type="button"
              @click="closeSwapModal"
              class="px-4 py-2 rounded-lg"
              :class="isDark ? 'bg-gray-700 hover:bg-gray-600 text-white' : 'bg-gray-100 hover:bg-gray-200 text-gray-900'"
            >
              Cancel
            </button>
            <button
              type="button"
              @click="submitSwapAccountNo"
              :disabled="swapLoading"
              class="bg-teal-600 hover:bg-teal-700 text-white px-4 py-2 rounded-lg font-medium disabled:opacity-50"
            >
              {{ swapLoading ? 'Swapping...' : 'Swap Account No' }}
            </button>
          </div>
        </div>
      </div>
    </div>

    <!-- Success Modal -->
    <SuccessModal
      :is-open="isSuccessModalOpen"
      :title="successTitle"
      :message="successMessage"
      :is-dark="isDark"
      @close="isSuccessModalOpen = false"
    />
    </div>
  </AppSidebar>
</template>

<script>
import { ref, computed, watch } from 'vue'
import { useRouter } from 'vue-router'
import { openReceiptWindow, printReceiptDocument } from '../utils/receipt.js'
import AppSidebar from '../components/AppSidebar.vue'
import ConfirmDialog from '../components/ConfirmDialog.vue'
import SuccessModal from '../components/SuccessModal.vue'
import UsageModal from '../components/UsageModal.vue'
import StatsCards from '../components/StatsCards.vue'
import ClientForm from '../components/ClientForm.vue'
import ReadingImportModal from '../components/ReadingImportModal.vue'
import { useCustomers } from '../composables/useCustomers.js'
import { useModals } from '../composables/useModals.js'
import { useCustomerForm } from '../composables/useCustomerForm.js'
import { useUsage } from '../composables/useUsage.js'

export default {
  name: 'Clients',
  components: {
    AppSidebar,
    ConfirmDialog,
    SuccessModal,
    UsageModal,
    StatsCards,
    ClientForm,
    ReadingImportModal
  },
  setup() {
    const router = useRouter()
    const customersComposable = useCustomers()
    const modalsComposable = useModals()
    const formComposable = useCustomerForm()
    const usageComposable = useUsage()
    const isViewModalOpen = ref(false)
    const viewingCustomer = ref(null)
    const selectedBarangay = ref(null)
    const isSwapModalOpen = ref(false)
    const swapSourceCustomer = ref(null)
    const swapTargetCustomerId = ref(null)
    const swapError = ref('')
    const swapLoading = ref(false)
    const isReadingImportOpen = ref(false)

    const statusFilter = ref('all')
    const billFilter = ref('all')

    const filteredCustomers = computed(() => {
      let filtered = customersComposable.filteredCustomers.value
      if (selectedBarangay.value !== null) {
        filtered = filtered.filter(customer => customer.brgy_id === selectedBarangay.value)
      }
      if (statusFilter.value !== 'all') {
        filtered = filtered.filter(customer => customer.status === statusFilter.value)
      }
      if (billFilter.value !== 'all') {
        filtered = filtered.filter(customer => (customer.latest_bill_state || 'none') === billFilter.value)
      }
      return filtered
    })

    const totalOutstandingBalance = computed(() => customersComposable.customers.value.reduce(
      (total, customer) => total + Number(customer.account_balance || 0),
      0
    ))

    const accountsWithBalance = computed(() => customersComposable.customers.value.filter(
      customer => Number(customer.account_balance || 0) > 0
    ).length)

    const displayedAccountsWithBalance = ref(0)
    const displayedCustomersShown = ref(0)
    const displayedOutstandingBalance = ref(0)
    const animationRuns = {}
    const animateMetric = (key, target, setter, round = false) => {
      const numericTarget = Number(target) || 0
      const run = (animationRuns[key] || 0) + 1
      animationRuns[key] = run
      const duration = 1000
      const steps = 60
      const increment = numericTarget / steps
      let current = 0

      if (numericTarget === 0) {
        setter(0)
        return
      }

      const animate = () => {
        if (animationRuns[key] !== run) return
        current += increment
        if (current >= numericTarget) current = numericTarget
        setter(round ? Math.round(current) : current)
        if (current < numericTarget) {
          setTimeout(animate, duration / steps)
        }
      }

      animate()
    }

    watch(accountsWithBalance, target => animateMetric('accounts', target, value => { displayedAccountsWithBalance.value = value }, true), { immediate: true })
    watch(() => filteredCustomers.value.length, target => animateMetric('shown', target, value => { displayedCustomersShown.value = value }, true), { immediate: true })
    watch(totalOutstandingBalance, target => animateMetric('balance', target, value => { displayedOutstandingBalance.value = value }), { immediate: true })

    const handleReadingImportCompleted = async (result) => {
      await fetchCustomersWithFilters()
      modalsComposable.showSuccess(
        'Readings Imported',
        `<strong>${result.applied}</strong> ${result.barangay} workbook rows were applied successfully${result.created ? `, including <strong>${result.created}</strong> first reading ${result.created === 1 ? 'record' : 'records'}` : ''}${result.bill_statuses_updated ? ` and <strong>${result.bill_statuses_updated}</strong> bill status ${result.bill_statuses_updated === 1 ? 'change' : 'changes'}` : ''}.`
      )
    }

    const swapTargets = computed(() => {
      const sourceId = swapSourceCustomer.value?.id
      return customersComposable.customers.value.filter(customer => customer.id !== sourceId)
    })

    const openViewModal = (customer) => {
      viewingCustomer.value = customer
      isViewModalOpen.value = true
    }

    const closeViewModal = () => {
      isViewModalOpen.value = false
      viewingCustomer.value = null
    }
    const handleMemberAction = (event, customer) => {
      const action = event.target.value
      event.target.value = ''
      if (action === 'profile') return router.push(`/clients/${customer.id}/profile`)
      if (action === 'incident') return router.push({ path: '/incidents', query: { customer: String(customer.id), new: '1' } })
      const actions = { summary: openViewModal, edit: openEditModal, usage: openUsageModal, disconnect: disconnectCustomer, reconnect: reconnectCustomer, swap: openSwapModal, remove: openDeleteModal }
      return actions[action]?.(customer)
    }

    // Modal handlers with form integration
    const openAddModal = () => {
      formComposable.resetNewCustomer()
      modalsComposable.openAddModal()
    }

    const closeAddModal = () => {
      if (formComposable.isSubmitting.value) return
      modalsComposable.closeAddModal()
    }

    const openEditModal = (customer) => {
      formComposable.resetEditCustomer()
      Object.assign(formComposable.editCustomer, {
        profile_manages_name: Boolean(Number(customer.profile_manages_name)),
        account_no: customer.account_no || '',
        customer_name: customer.customer_name || '',
        type_id: customer.type_id || null,
        deduction_id: customer.deduction_id || null,
        brgy_id: customer.brgy_id || null,
        address: customer.address || '',
        previous_reading: customer.previous_reading || 0,
        current_reading: customer.current_reading ?? null,
        has_current_reading: customer.current_reading !== null && customer.current_reading !== undefined,
        status: customer.status || 'active',
      })
      modalsComposable.openEditModal(customer)
    }

    const closeEditModal = () => {
      if (formComposable.isSubmitting.value) return
      modalsComposable.closeEditModal()
    }

    const openDeleteModal = (customer) => {
      modalsComposable.openDeleteModal(customer)
    }

    const closeDeleteModal = () => {
      if (formComposable.isSubmitting.value) return
      modalsComposable.closeDeleteModal()
    }

    const openSwapModal = (customer) => {
      swapSourceCustomer.value = customer
      swapTargetCustomerId.value = null
      swapError.value = ''
      isSwapModalOpen.value = true
    }

    const closeSwapModal = () => {
      if (swapLoading.value) return
      isSwapModalOpen.value = false
      swapSourceCustomer.value = null
      swapTargetCustomerId.value = null
      swapError.value = ''
    }

    const openUsageModal = async (customer) => {
      modalsComposable.openUsageModal(customer)
      await refreshUsageCustomer()
      const refreshedCustomer = modalsComposable.usageCustomer.value || customer
      await usageComposable.fetchUsageReadings(refreshedCustomer.id)
    }

    const closeUsageModal = () => {
      if (usageComposable.usageLoading.value) return
      modalsComposable.closeUsageModal()
    }

    const printUsageReceipt = async (reading) => {
      if (!reading?.bill_id) return

      let printWindow
      try {
        printWindow = openReceiptWindow()
        const response = await fetch(`/bills/${reading.bill_id}/payment`, {
          headers: { Accept: 'application/json' },
          credentials: 'same-origin',
        })
        if (!response.ok) throw new Error('No payment transaction was found for this bill.')
        const payment = await response.json()
        const customer = modalsComposable.usageCustomer.value
        await printReceiptDocument(payment.receipt || {
          ...payment,
          reference_number: reading.reference_number || reading.bill_id,
          account_no: customer?.account_no,
        }, customer?.customer_name, printWindow)
      } catch (error) {
        printWindow?.close()
        alert(error?.message || 'Failed to load the bill receipt.')
      }
    }

    // Customer operations
    const disconnectCustomer = async (customer) => {
      const confirmed = window.confirm(`Are you sure you want to disconnect ${customer.customer_name}? They will no longer be able to use the water service.`)
      if (!confirmed) return

      formComposable.isSubmitting.value = true
      try {
        await customersComposable.updateCustomer(customer.id, {
          account_no: customer.account_no,
          customer_name: customer.customer_name,
          type_id: customer.type_id,
          brgy_id: customer.brgy_id,
          address: customer.address,
          previous_reading: customer.previous_reading,
          status: 'disconnected',
        })

        modalsComposable.showSuccess(
          'Customer Disconnected',
          `Customer <strong>${customer.customer_name}</strong> has been successfully disconnected from the water service.`
        )
      } catch (e) {
        alert('Error: ' + (e?.message || 'Failed to disconnect customer'))
      } finally {
        formComposable.isSubmitting.value = false
      }
    }

    const reconnectCustomer = async (customer) => {
      const confirmed = window.confirm(`Are you sure you want to reconnect ${customer.customer_name}? They will be able to use the water service again.`)
      if (!confirmed) return

      formComposable.isSubmitting.value = true
      try {
        await customersComposable.updateCustomer(customer.id, {
          account_no: customer.account_no,
          customer_name: customer.customer_name,
          type_id: customer.type_id,
          brgy_id: customer.brgy_id,
          address: customer.address,
          previous_reading: customer.previous_reading,
          status: 'active',
        })

        modalsComposable.showSuccess(
          'Customer Reconnected',
          `Customer <strong>${customer.customer_name}</strong> has been successfully reconnected to the water service.`
        )
      } catch (e) {
        alert('Error: ' + (e?.message || 'Failed to reconnect customer'))
      } finally {
        formComposable.isSubmitting.value = false
      }
    }

    const confirmDelete = async () => {
      if (!modalsComposable.deletingCustomer.value) return

      formComposable.isSubmitting.value = true
      const customer = modalsComposable.deletingCustomer.value

      try {
        await customersComposable.deleteCustomer(customer.id)

        modalsComposable.closeDeleteModal()
        modalsComposable.showSuccess(
          'Client Deleted',
          `Client <strong>${customer.customer_name}</strong> has been successfully deleted.`
        )
      } catch (e) {
        alert('Error: ' + (e?.message || 'Failed to delete customer'))
      } finally {
        formComposable.isSubmitting.value = false
      }
    }

    const submitEditCustomer = async () => {
      if (!modalsComposable.editingCustomer.value) return

      formComposable.isSubmitting.value = true
      formComposable.clearErrors()

      try {
        await customersComposable.updateCustomer(modalsComposable.editingCustomer.value.id, {
          account_no: formComposable.editCustomer.account_no,
          customer_name: formComposable.editCustomer.customer_name,
          type_id: formComposable.editCustomer.type_id,
          deduction_id: formComposable.editCustomer.deduction_id,
          brgy_id: formComposable.editCustomer.brgy_id,
          address: formComposable.editCustomer.address,
          previous_reading: formComposable.editCustomer.previous_reading,
          current_reading: formComposable.editCustomer.current_reading,
          status: formComposable.editCustomer.status,
        })

        modalsComposable.closeEditModal()
        modalsComposable.showSuccess(
          'Client Updated',
          `Client <strong>${formComposable.editCustomer.customer_name}</strong> has been successfully updated.`
        )
      } catch (e) {
        if (e.validationErrors) {
          formComposable.setFieldErrors(e.validationErrors)
        } else {
          formComposable.submitError.value = e?.message || 'Failed to update customer'
        }
      } finally {
        formComposable.isSubmitting.value = false
      }
    }

    const submitNewCustomer = async () => {
      formComposable.isSubmitting.value = true
      formComposable.clearErrors()

      try {
        await customersComposable.createCustomer({
          customer_name: formComposable.newCustomer.customer_name,
          type_id: formComposable.newCustomer.type_id,
          brgy_id: formComposable.newCustomer.brgy_id,
          address: formComposable.newCustomer.address,
          previous_reading: formComposable.newCustomer.previous_reading,
          status: formComposable.newCustomer.status,
          deduction_id: formComposable.newCustomer.deduction_id,
        })

        modalsComposable.closeAddModal()
        modalsComposable.showSuccess(
          'Client Added',
          `Client <strong>${formComposable.newCustomer.customer_name}</strong> has been successfully added.`
        )
      } catch (e) {
        if (e.message.includes('Validation error')) {
          formComposable.setFieldErrors(JSON.parse(e.message.replace('Validation error: ', '')))
        } else {
          formComposable.submitError.value = e?.message || 'Failed to save customer'
        }
      } finally {
        formComposable.isSubmitting.value = false
      }
    }

    const submitSwapAccountNo = async () => {
      const targetId = Number(swapTargetCustomerId.value)
      if (!swapSourceCustomer.value || !swapTargetCustomerId.value || Number.isNaN(targetId)) {
        swapError.value = 'Select a customer to swap with.'
        return
      }

      swapLoading.value = true
      swapError.value = ''
      const source = swapSourceCustomer.value
      const target = customersComposable.customers.value.find(c => c.id === targetId)

      try {
        await customersComposable.swapAccountNo(source.id, targetId)

        swapLoading.value = false
        closeSwapModal()
        modalsComposable.showSuccess(
          'Account Numbers Swapped',
          `Account numbers for <strong>${source.customer_name}</strong> and <strong>${target?.customer_name || 'selected customer'}</strong> have been swapped.`
        )
      } catch (e) {
        swapError.value = e?.message || 'Failed to swap account numbers'
      } finally {
        swapLoading.value = false
      }
    }

    const refreshUsageCustomer = async () => {
      const currentCustomer = modalsComposable.usageCustomer.value
      if (!currentCustomer) return

      await customersComposable.fetchCustomers()

      const refreshedCustomer = customersComposable.customers.value.find(
        customer => Number(customer.id) === Number(currentCustomer.id)
      )
      if (refreshedCustomer) {
        modalsComposable.usageCustomer.value = refreshedCustomer
      }
    }

    const markBillPaid = async (payload) => {
      const readingRow = payload?.reading
      if (!readingRow?.bill_id) return

      try {
        await usageComposable.markBillPaid(readingRow.bill_id, payload)

        if (modalsComposable.usageCustomer.value) {
          await usageComposable.fetchUsageReadings(modalsComposable.usageCustomer.value.id)
        }
        await refreshUsageCustomer()

        modalsComposable.showSuccess(
          'Bill Marked as Paid',
          'The bill has been successfully marked as paid.'
        )
      } catch (e) {
        alert('Error: ' + (e?.message || 'Failed to mark bill as paid'))
      }
    }

    const voidBill = async (readingRow, reason) => {
      if (!readingRow?.bill_id) return

      try {
        await usageComposable.voidBill(readingRow.bill_id, reason)

        if (modalsComposable.usageCustomer.value) {
          await usageComposable.fetchUsageReadings(modalsComposable.usageCustomer.value.id)
        }
        await refreshUsageCustomer()

        modalsComposable.showSuccess(
          'Bill Voided',
          'The bill has been successfully voided.'
        )
      } catch (e) {
        alert('Error: ' + (e?.message || 'Failed to void bill'))
      }
    }

    const inputClass = (isDark) => {
      return isDark
        ? 'bg-gray-800 border-gray-700 text-white placeholder:text-gray-400 focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500'
        : 'bg-white border-gray-300 text-gray-900 placeholder:text-gray-400 focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500'
    }

    const getTypeName = (typeId) => {
      const type = customersComposable.customerTypes.value.find(t => t.type_id === typeId)
      return type ? type.type_name : 'Unknown'
    }

    const getBarangayName = (brgyId) => {
      const brgy = customersComposable.barangays.value.find(b => b.brgy_id === brgyId)
      return brgy ? brgy.barangay : 'Unknown'
    }

    const getDeductionName = (deductionId) => {
      if (!deductionId) return ''
      const deduction = customersComposable.deductions.value.find(d => d.deduction_id === deductionId)
      return deduction ? deduction.name : ''
    }

    const displayCustomerAmount = (customer) => {
      return customer.latest_bill_state === 'paid'
        ? (customer.latest_bill_total_due ?? customer.account_balance ?? 0)
        : (customer.account_balance ?? 0)
    }

    const formatWholePesos = (value) => {
      const amount = Number(value) || 0
      return Math.round(amount).toLocaleString('en-PH', {
        minimumFractionDigits: 0,
        maximumFractionDigits: 0,
      })
    }

    const getCustomerAmountClass = (customer, isDark) => {
      if (customer.latest_bill_state === 'paid') return isDark ? 'text-green-300' : 'text-green-600'
      return Number(customer.account_balance || 0) > 0
        ? (isDark ? 'text-red-300' : 'text-red-600')
        : (isDark ? 'text-gray-300' : 'text-gray-600')
    }

    const previewAccountNoNew = computed(() => {
      const brgyId = formComposable.newCustomer.brgy_id
      if (!brgyId) return ''
      const brgy = customersComposable.barangays.value.find(b => b.brgy_id === brgyId)
      if (!brgy) return ''
      const num = String(brgy.next_number ?? 0).padStart(3, '0')
      return `${brgy.prefix}-${num}`
    })

    const fetchCustomersWithFilters = () => {
      customersComposable.fetchCustomers({
        search: customersComposable.search.value,
        status: statusFilter.value,
        bill_state: billFilter.value,
      })
    }

    let fetchTimer = null
    watch(
      [customersComposable.search, statusFilter, billFilter, selectedBarangay],
      () => {
        if (fetchTimer) clearTimeout(fetchTimer)
        fetchTimer = setTimeout(fetchCustomersWithFilters, 250)
      },
      { immediate: true }
    )

    // Initialize data
    customersComposable.fetchCustomerTypes()
    customersComposable.fetchBarangays()
    customersComposable.fetchDeductions()

    return {
      // Customers composable
      ...customersComposable,

      // Modals composable
      ...modalsComposable,

      // Form composable
      ...formComposable,

      // Usage composable
      ...usageComposable,

      // Methods
      openAddModal,
      closeAddModal,
      openEditModal,
      closeEditModal,
      openDeleteModal,
      closeDeleteModal,
      openUsageModal,
      closeUsageModal,
      printUsageReceipt,
      openViewModal,
      handleMemberAction,
      closeViewModal,
      openSwapModal,
      closeSwapModal,
      disconnectCustomer,
      reconnectCustomer,
      confirmDelete,
      submitEditCustomer,
      submitNewCustomer,
      submitSwapAccountNo,
      markBillPaid,
      voidBill,
      inputClass,
      getTypeName,
      getBarangayName,
      getDeductionName,
      displayCustomerAmount,
      formatWholePesos,
      getCustomerAmountClass,
      previewAccountNoNew,
      isViewModalOpen,
      viewingCustomer,
      selectedBarangay,
      isSwapModalOpen,
      swapSourceCustomer,
      swapTargetCustomerId,
      swapTargets,
      swapError,
      swapLoading,
      statusFilter,
      billFilter,
      filteredCustomers,
      totalOutstandingBalance,
      displayedOutstandingBalance,
      displayedAccountsWithBalance,
      displayedCustomersShown,
      accountsWithBalance,
      isReadingImportOpen,
      handleReadingImportCompleted,
    }
  }
}
</script>
