<template>
  <AppSidebar v-slot="{ isDark }">
    <div class="p-8">
      <div class="mb-8">
        <h1 class="text-3xl font-bold mb-2 transition-colors duration-200" :class="isDark ? 'text-white' : 'text-gray-900'">Client Management</h1>
        <p class="transition-colors duration-200" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Manage your water system clients and their accounts</p>
      </div>

      <!-- Add Client Button -->
      <div class="mb-6">
      <button @click="openAddModal" class="bg-indigo-600 hover:bg-indigo-700 text-white px-6 py-3 rounded-lg font-medium shadow-lg transition-all duration-200 flex items-center space-x-2">
        <svg class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
          <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 6v6m0 0v6m0-6h6m-6 0H6" />
        </svg>
        <span>Add New Client</span>
      </button>
    </div>

    <!-- Stats Cards -->
    <div class="grid grid-cols-1 md:grid-cols-4 gap-6 mb-8">
      <div class="bg-white rounded-lg shadow p-6">
        <div class="flex items-center">
          <div class="h-12 w-12 bg-blue-100 rounded-lg flex items-center justify-center">
            <svg class="h-6 w-6 text-blue-600" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M17 20h5v-2a3 3 0 00-5.356-1.857M17 20H7m10 0v-2c0-.656-.126-1.283-.356-1.857M7 20H2v-2a3 3 0 015.356-1.857M7 20v-2c0-.656.126-1.283.356-1.857m0 0a5.002 5.002 0 019.288 0M15 7a3 3 0 11-6 0 3 3 0 016 0zm6 3a2 2 0 11-4 0 2 2 0 014 0zM7 10a2 2 0 11-4 0 2 2 0 014 0z" />
            </svg>
          </div>
          <div class="ml-4">
            <p class="text-sm text-gray-600">Total Clients</p>
            <p class="text-2xl font-bold text-gray-900">{{ customers.length }}</p>
          </div>
        </div>
      </div>

      <div class="bg-white rounded-lg shadow p-6">
        <div class="flex items-center">
          <div class="h-12 w-12 bg-green-100 rounded-lg flex items-center justify-center">
            <svg class="h-6 w-6 text-green-600" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 12l2 2 4-4m6 2a9 9 0 11-18 0 9 9 0 0118 0z" />
            </svg>
          </div>
          <div class="ml-4">
            <p class="text-sm text-gray-600">Active</p>
            <p class="text-2xl font-bold text-gray-900">{{ activeCount }}</p>
          </div>
        </div>
      </div>

      <div class="bg-white rounded-lg shadow p-6">
        <div class="flex items-center">
          <div class="h-12 w-12 bg-yellow-100 rounded-lg flex items-center justify-center">
            <svg class="h-6 w-6 text-yellow-600" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 8v4l3 3m6-3a9 9 0 11-18 0 9 9 0 0118 0z" />
            </svg>
          </div>
          <div class="ml-4">
            <p class="text-sm text-gray-600">New (Prev = 0)</p>
            <p class="text-2xl font-bold text-gray-900">{{ newCount }}</p>
          </div>
        </div>
      </div>

      <div class="bg-white rounded-lg shadow p-6">
        <div class="flex items-center">
          <div class="h-12 w-12 bg-red-100 rounded-lg flex items-center justify-center">
            <svg class="h-6 w-6 text-red-600" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M18.364 18.364A9 9 0 005.636 5.636m12.728 12.728A9 9 0 015.636 5.636m12.728 12.728L5.636 5.636" />
            </svg>
          </div>
          <div class="ml-4">
            <p class="text-sm text-gray-600">Inactive</p>
            <p class="text-2xl font-bold text-gray-900">{{ inactiveCount }}</p>
          </div>
        </div>
      </div>
    </div>

    <!-- Clients Table -->
    <div class="bg-white rounded-lg shadow overflow-hidden">
      <div class="px-6 py-4 border-b border-gray-200 flex items-center justify-between">
        <h2 class="text-lg font-semibold text-gray-900">Client List</h2>
        <div class="flex items-center space-x-4">
          <input
            type="search"
            placeholder="Search clients..."
            v-model="search"
            class="px-4 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500"
          />
        </div>
      </div>
      <div class="overflow-x-auto">
        <table class="w-full">
          <thead class="bg-gray-50">
            <tr>
              <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Account No</th>
              <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Customer Name</th>
              <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Address</th>
              <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Previous Reading</th>
              <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Current Reading</th>
              <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Latest Bill</th>
              <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Status</th>
              <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Usage</th>
              <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Actions</th>
            </tr>
          </thead>
          <tbody class="bg-white divide-y divide-gray-200">
            <tr v-if="isLoading">
              <td class="px-6 py-6 text-sm text-gray-600" colspan="9">Loading customers...</td>
            </tr>
            <tr v-else-if="errorMessage">
              <td class="px-6 py-6 text-sm text-red-600" colspan="9">{{ errorMessage }}</td>
            </tr>
            <tr v-else-if="filteredCustomers.length === 0">
              <td class="px-6 py-6 text-sm text-gray-600" colspan="9">No customers found.</td>
            </tr>
            <tr v-else v-for="customer in filteredCustomers" :key="customer.id" class="hover:bg-gray-50">
              <td class="px-6 py-4 whitespace-nowrap text-sm text-gray-900">{{ customer.account_no }}</td>
              <td class="px-6 py-4 whitespace-nowrap">
                <div class="flex items-center">
                  <div class="h-8 w-8 bg-indigo-100 rounded-full flex items-center justify-center">
                    <span class="text-indigo-600 font-medium text-sm">{{ (customer.customer_name || '?').charAt(0) }}</span>
                  </div>
                  <div class="ml-3">
                    <p class="text-sm font-medium text-gray-900">{{ customer.customer_name }}</p>
                  </div>
                </div>
              </td>
              <td class="px-6 py-4 whitespace-nowrap text-sm text-gray-600">{{ customer.address }}</td>
              <td class="px-6 py-4 whitespace-nowrap text-sm text-gray-900">{{ customer.previous_reading }}</td>
              <td class="px-6 py-4 whitespace-nowrap text-sm text-gray-900">{{ customer.current_reading ?? '-' }}</td>
              <td class="px-6 py-4 whitespace-nowrap text-sm">
                <span v-if="customer.latest_bill_state === 'paid'" class="px-3 py-1 rounded-full text-xs font-medium bg-green-100 text-green-800">Paid</span>
                <span v-else-if="customer.latest_bill_state === 'due'" class="px-3 py-1 rounded-full text-xs font-medium bg-red-100 text-red-800">Due</span>
                <span v-else-if="customer.latest_bill_state === 'pending'" class="px-3 py-1 rounded-full text-xs font-medium bg-yellow-100 text-yellow-800">Pending</span>
                <span v-else class="px-3 py-1 rounded-full text-xs font-medium bg-gray-100 text-gray-700">No Bill</span>
              </td>
              <td class="px-6 py-4 whitespace-nowrap">
                <span :class="customer.is_active ? 'bg-green-100 text-green-800' : 'bg-red-100 text-red-800'" class="px-3 py-1 rounded-full text-xs font-medium">
                  {{ customer.is_active ? 'Active' : 'Inactive' }}
                </span>
              </td>
              <td class="px-6 py-4 whitespace-nowrap text-sm">
                <button @click="openUsageModal(customer)" class="text-indigo-600 hover:text-indigo-900 font-medium">
                  View Usage
                </button>
              </td>
              <td class="px-6 py-4 whitespace-nowrap text-sm">
                <div class="flex space-x-2">
                  <button @click="openEditModal(customer)" class="text-blue-600 hover:text-blue-900 font-medium">
                    Edit
                  </button>
                  <button @click="openDeleteModal(customer)" class="text-red-600 hover:text-red-900 font-medium">
                    Remove
                  </button>
                </div>
              </td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>

    <!-- Usage Modal -->
    <div v-if="isUsageModalOpen" class="fixed inset-0 z-50">
      <div class="absolute inset-0 bg-black/50" @click="closeUsageModal"></div>
      <div class="absolute inset-0 flex items-center justify-center p-4">
        <div class="w-full max-w-3xl rounded-lg shadow-lg" :class="isDark ? 'bg-gray-900 text-white' : 'bg-white text-gray-900'">
          <div class="px-6 py-4 border-b" :class="isDark ? 'border-gray-800' : 'border-gray-200'">
            <div class="flex items-center justify-between">
              <div>
                <h3 class="text-lg font-semibold">Usage History</h3>
                <p class="text-sm" :class="isDark ? 'text-gray-300' : 'text-gray-600'">
                  {{ usageCustomer?.customer_name }} ({{ usageCustomer?.account_no }})
                </p>
              </div>
              <button class="text-sm" :class="isDark ? 'text-gray-300 hover:text-white' : 'text-gray-600 hover:text-gray-900'" @click="closeUsageModal">
                Close
              </button>
            </div>
          </div>

          <div class="px-6 py-4">
            <div v-if="usageLoading" class="text-sm" :class="isDark ? 'text-gray-300' : 'text-gray-600'">Loading readings...</div>
            <div v-else-if="usageError" class="text-sm text-red-600">{{ usageError }}</div>
            <div v-else-if="usageReadings.length === 0" class="text-sm" :class="isDark ? 'text-gray-300' : 'text-gray-600'">
              No readings yet for this customer.
            </div>
            <div v-else class="overflow-x-auto">
              <table class="w-full">
                <thead :class="isDark ? 'bg-gray-800' : 'bg-gray-50'">
                  <tr>
                    <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-200' : 'text-gray-500'">Date</th>
                    <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-200' : 'text-gray-500'">Previous</th>
                    <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-200' : 'text-gray-500'">Current</th>
                    <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-200' : 'text-gray-500'">Usage (m³)</th>
                    <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-200' : 'text-gray-500'">Bill</th>
                    <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-200' : 'text-gray-500'">Action</th>
                  </tr>
                </thead>
                <tbody :class="isDark ? 'divide-y divide-gray-800' : 'divide-y divide-gray-200'">
                  <tr v-for="r in usageReadings" :key="r.id" :class="isDark ? 'hover:bg-gray-800/60' : 'hover:bg-gray-50'">
                    <td class="px-4 py-2 text-sm">{{ formatReadingAt(r.reading_at) }}</td>
                    <td class="px-4 py-2 text-sm">{{ r.previous_reading }}</td>
                    <td class="px-4 py-2 text-sm">{{ r.current_reading }}</td>
                    <td class="px-4 py-2 text-sm font-medium">{{ r.usage_m3 }}</td>
                    <td class="px-4 py-2 text-sm">
                      <span v-if="!r.bill_id" class="px-2 py-1 rounded-full text-xs font-medium bg-gray-100 text-gray-700">No Bill</span>
                      <span v-else-if="String(r.status || '').toLowerCase() === 'paid'" class="px-2 py-1 rounded-full text-xs font-medium bg-green-100 text-green-800">Paid</span>
                      <span v-else-if="isBillDue(r)" class="px-2 py-1 rounded-full text-xs font-medium bg-red-100 text-red-800">Due</span>
                      <span v-else class="px-2 py-1 rounded-full text-xs font-medium bg-yellow-100 text-yellow-800">Pending</span>
                      <span v-if="r.bill_id" class="ml-2 text-xs" :class="isDark ? 'text-gray-300' : 'text-gray-600'">
                        ₱{{ formatMoney(r.total_due) }}
                      </span>
                    </td>
                    <td class="px-4 py-2 text-sm">
                      <button
                        v-if="r.bill_id && String(r.status || '').toLowerCase() !== 'paid'"
                        @click="markBillPaid(r)"
                        class="text-indigo-600 hover:text-indigo-900 font-medium"
                      >
                        Mark Paid
                      </button>
                      <span v-else class="text-xs" :class="isDark ? 'text-gray-400' : 'text-gray-500'">-</span>
                    </td>
                  </tr>
                </tbody>
              </table>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- Add Client Modal -->
    <div v-if="isAddModalOpen" class="fixed inset-0 z-50">
      <div class="absolute inset-0 bg-black/50" @click="closeAddModal"></div>
      <div class="absolute inset-0 flex items-center justify-center p-4">
        <div class="w-full max-w-lg rounded-lg shadow-lg" :class="isDark ? 'bg-gray-900 text-white' : 'bg-white text-gray-900'">
          <div class="px-6 py-4 border-b" :class="isDark ? 'border-gray-800' : 'border-gray-200'">
            <div class="flex items-center justify-between">
              <h3 class="text-lg font-semibold">Add New Client</h3>
              <button class="text-sm" :class="isDark ? 'text-gray-300 hover:text-white' : 'text-gray-600 hover:text-gray-900'" @click="closeAddModal">
                Close
              </button>
            </div>
          </div>

          <form class="px-6 py-4 space-y-4" @submit.prevent="submitNewCustomer">
            <div>
              <label class="block text-sm font-medium mb-1">Account No</label>
              <input
                v-model.trim="newCustomer.account_no"
                type="text"
                class="w-full px-4 py-2 rounded-lg border"
                :class="inputClass(isDark)"
                placeholder="e.g. ACC-2026-001"
                required
              />
              <p v-if="fieldError('account_no')" class="text-sm text-red-600 mt-1">{{ fieldError('account_no') }}</p>
            </div>

            <div>
              <label class="block text-sm font-medium mb-1">Customer Name</label>
              <input
                v-model.trim="newCustomer.customer_name"
                type="text"
                class="w-full px-4 py-2 rounded-lg border"
                :class="inputClass(isDark)"
                placeholder="Full name"
                required
              />
              <p v-if="fieldError('customer_name')" class="text-sm text-red-600 mt-1">{{ fieldError('customer_name') }}</p>
            </div>

            <div>
              <label class="block text-sm font-medium mb-1">Address</label>
              <input
                v-model.trim="newCustomer.address"
                type="text"
                class="w-full px-4 py-2 rounded-lg border"
                :class="inputClass(isDark)"
                placeholder="Customer address"
                required
              />
              <p v-if="fieldError('address')" class="text-sm text-red-600 mt-1">{{ fieldError('address') }}</p>
            </div>

            <div>
              <label class="block text-sm font-medium mb-1">Previous Reading (0 for new customer)</label>
              <input
                v-model.number="newCustomer.previous_reading"
                type="number"
                min="0"
                class="w-full px-4 py-2 rounded-lg border"
                :class="inputClass(isDark)"
              />
              <p v-if="fieldError('previous_reading')" class="text-sm text-red-600 mt-1">{{ fieldError('previous_reading') }}</p>
            </div>

            <div class="flex items-center gap-2">
              <input id="is_active" v-model="newCustomer.is_active" type="checkbox" class="h-4 w-4" />
              <label for="is_active" class="text-sm">Active</label>
            </div>

            <p v-if="submitError" class="text-sm text-red-600">{{ submitError }}</p>

            <div class="pt-2 flex items-center justify-end gap-3">
              <button type="button" @click="closeAddModal" class="px-4 py-2 rounded-lg" :class="isDark ? 'bg-gray-800 hover:bg-gray-700 text-white' : 'bg-gray-100 hover:bg-gray-200 text-gray-900'">
                Cancel
              </button>
              <button type="submit" :disabled="isSubmitting" class="bg-indigo-600 hover:bg-indigo-700 text-white px-4 py-2 rounded-lg font-medium disabled:opacity-50">
                {{ isSubmitting ? 'Saving...' : 'Save Client' }}
              </button>
            </div>
          </form>
        </div>
      </div>
    </div>

    <!-- Edit Client Modal -->
    <div v-if="isEditModalOpen" class="fixed inset-0 z-50">
      <div class="absolute inset-0 bg-black/50" @click="closeEditModal"></div>
      <div class="absolute inset-0 flex items-center justify-center p-4">
        <div class="w-full max-w-lg rounded-lg shadow-lg" :class="isDark ? 'bg-gray-900 text-white' : 'bg-white text-gray-900'">
          <div class="px-6 py-4 border-b" :class="isDark ? 'border-gray-800' : 'border-gray-200'">
            <div class="flex items-center justify-between">
              <h3 class="text-lg font-semibold">Edit Client</h3>
              <button class="text-sm" :class="isDark ? 'text-gray-300 hover:text-white' : 'text-gray-600 hover:text-gray-900'" @click="closeEditModal">
                Close
              </button>
            </div>
          </div>

          <form class="px-6 py-4 space-y-4" @submit.prevent="submitEditCustomer">
            <div>
              <label class="block text-sm font-medium mb-1">Account No</label>
              <input
                v-model.trim="editCustomer.account_no"
                type="text"
                class="w-full px-4 py-2 rounded-lg border"
                :class="inputClass(isDark)"
                placeholder="e.g. ACC-2026-001"
                required
              />
              <p v-if="fieldError('account_no')" class="text-sm text-red-600 mt-1">{{ fieldError('account_no') }}</p>
            </div>

            <div>
              <label class="block text-sm font-medium mb-1">Customer Name</label>
              <input
                v-model.trim="editCustomer.customer_name"
                type="text"
                class="w-full px-4 py-2 rounded-lg border"
                :class="inputClass(isDark)"
                placeholder="Full name"
                required
              />
              <p v-if="fieldError('customer_name')" class="text-sm text-red-600 mt-1">{{ fieldError('customer_name') }}</p>
            </div>

            <div>
              <label class="block text-sm font-medium mb-1">Address</label>
              <input
                v-model.trim="editCustomer.address"
                type="text"
                class="w-full px-4 py-2 rounded-lg border"
                :class="inputClass(isDark)"
                placeholder="Customer address"
                required
              />
              <p v-if="fieldError('address')" class="text-sm text-red-600 mt-1">{{ fieldError('address') }}</p>
            </div>

            <div>
              <label class="block text-sm font-medium mb-1">Previous Reading</label>
              <input
                v-model.number="editCustomer.previous_reading"
                type="number"
                min="0"
                class="w-full px-4 py-2 rounded-lg border"
                :class="inputClass(isDark)"
              />
              <p v-if="fieldError('previous_reading')" class="text-sm text-red-600 mt-1">{{ fieldError('previous_reading') }}</p>
            </div>

            <div class="flex items-center gap-2">
              <input id="edit_is_active" v-model="editCustomer.is_active" type="checkbox" class="h-4 w-4" />
              <label for="edit_is_active" class="text-sm">Active</label>
            </div>

            <p v-if="submitError" class="text-sm text-red-600">{{ submitError }}</p>

            <div class="pt-2 flex items-center justify-end gap-3">
              <button type="button" @click="closeEditModal" class="px-4 py-2 rounded-lg" :class="isDark ? 'bg-gray-800 hover:bg-gray-700 text-white' : 'bg-gray-100 hover:bg-gray-200 text-gray-900'">
                Cancel
              </button>
              <button type="submit" :disabled="isSubmitting" class="bg-indigo-600 hover:bg-indigo-700 text-white px-4 py-2 rounded-lg font-medium disabled:opacity-50">
                {{ isSubmitting ? 'Saving...' : 'Update Client' }}
              </button>
            </div>
          </form>
        </div>
      </div>
    </div>

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
import AppSidebar from '../components/AppSidebar.vue'
import ConfirmDialog from '../components/ConfirmDialog.vue'
import SuccessModal from '../components/SuccessModal.vue'

export default {
  name: 'Clients',
  components: {
    AppSidebar,
    ConfirmDialog,
    SuccessModal
  },
  data() {
    return {
      customers: [],
      isLoading: false,
      errorMessage: '',
      search: '',

      isAddModalOpen: false,
      isSubmitting: false,
      submitError: '',
      submitFieldErrors: {},
      newCustomer: {
        account_no: '',
        customer_name: '',
        address: '',
        previous_reading: 0,
        is_active: true,
      },

      isEditModalOpen: false,
      editingCustomer: null,
      editCustomer: {
        account_no: '',
        customer_name: '',
        address: '',
        previous_reading: 0,
        is_active: true,
      },

      isDeleteModalOpen: false,
      deletingCustomer: null,

      isSuccessModalOpen: false,
      successTitle: '',
      successMessage: '',

      isUsageModalOpen: false,
      usageCustomer: null,
      usageReadings: [],
      usageLoading: false,
      usageError: '',
    }
  },
  computed: {
    activeCount() {
      return this.customers.filter(c => !!c.is_active).length
    },
    inactiveCount() {
      return this.customers.filter(c => !c.is_active).length
    },
    newCount() {
      return this.customers.filter(c => Number(c.previous_reading) === 0).length
    },
    filteredCustomers() {
      const q = (this.search || '').trim().toLowerCase()
      if (!q) return this.customers
      return this.customers.filter(c => {
        return (
          String(c.account_no || '').toLowerCase().includes(q) ||
          String(c.customer_name || '').toLowerCase().includes(q) ||
          String(c.address || '').toLowerCase().includes(q)
        )
      })
    }
  },
  mounted() {
    void this.fetchCustomers()
  },
  methods: {
    inputClass(isDark) {
      return isDark
        ? 'bg-gray-800 border-gray-700 text-white placeholder:text-gray-400 focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500'
        : 'bg-white border-gray-300 text-gray-900 placeholder:text-gray-400 focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500'
    },
    async fetchCustomers() {
      this.isLoading = true
      this.errorMessage = ''
      try {
        const res = await fetch('/customers', {
          headers: { 'Accept': 'application/json' },
          credentials: 'same-origin',
        })

        if (!res.ok) {
          if (res.status === 401) {
            this.errorMessage = 'Unauthorized. Please login again.'
            return
          }
          throw new Error('Failed to load customers')
        }

        const json = await res.json()
        this.customers = Array.isArray(json?.data) ? json.data : []
      } catch (e) {
        this.errorMessage = e?.message || 'Failed to load customers'
      } finally {
        this.isLoading = false
      }
    },
    openAddModal() {
      this.isAddModalOpen = true
      this.submitError = ''
      this.submitFieldErrors = {}
      this.newCustomer = {
        account_no: '',
        customer_name: '',
        address: '',
        previous_reading: 0,
        is_active: true,
      }
    },
    closeAddModal() {
      if (this.isSubmitting) return
      this.isAddModalOpen = false
    },
    openEditModal(customer) {
      this.editingCustomer = customer
      this.editCustomer = {
        account_no: customer.account_no || '',
        customer_name: customer.customer_name || '',
        address: customer.address || '',
        previous_reading: customer.previous_reading || 0,
        is_active: !!customer.is_active,
      }
      this.isEditModalOpen = true
      this.submitError = ''
      this.submitFieldErrors = {}
    },
    closeEditModal() {
      if (this.isSubmitting) return
      this.isEditModalOpen = false
      this.editingCustomer = null
    },
    openDeleteModal(customer) {
      this.deletingCustomer = customer
      this.isDeleteModalOpen = true
    },
    closeDeleteModal() {
      if (this.isSubmitting) return
      this.isDeleteModalOpen = false
      this.deletingCustomer = null
    },
    async confirmDelete() {
      if (!this.deletingCustomer) return

      this.isSubmitting = true
      const customer = this.deletingCustomer

      const csrf = document.querySelector('meta[name="csrf-token"]')?.getAttribute('content')
      try {
        const res = await fetch(`/customers/${customer.id}`, {
          method: 'DELETE',
          headers: {
            'Accept': 'application/json',
            ...(csrf ? { 'X-CSRF-TOKEN': csrf } : {}),
          },
          credentials: 'same-origin',
        })

        if (!res.ok) {
          if (res.status === 401) throw new Error('Unauthorized. Please login again.')
          const text = await res.text()
          throw new Error('Failed to delete customer: ' + text)
        }

        this.isDeleteModalOpen = false
        this.deletingCustomer = null
        await this.fetchCustomers()

        // Show success modal
        this.successTitle = 'Client Deleted'
        this.successMessage = `Client <strong>${customer.customer_name}</strong> has been successfully deleted.`
        this.isSuccessModalOpen = true
      } catch (e) {
        alert('Error: ' + (e?.message || 'Failed to delete customer'))
      } finally {
        this.isSubmitting = false
      }
    },
    async submitEditCustomer() {
      if (!this.editingCustomer) return

      this.isSubmitting = true
      this.submitError = ''
      this.submitFieldErrors = {}

      const csrf = document.querySelector('meta[name="csrf-token"]')?.getAttribute('content')
      try {
        const res = await fetch(`/customers/${this.editingCustomer.id}`, {
          method: 'PUT',
          headers: {
            'Accept': 'application/json',
            'Content-Type': 'application/json',
            ...(csrf ? { 'X-CSRF-TOKEN': csrf } : {}),
          },
          credentials: 'same-origin',
          body: JSON.stringify({
            account_no: this.editCustomer.account_no,
            customer_name: this.editCustomer.customer_name,
            address: this.editCustomer.address,
            previous_reading: this.editCustomer.previous_reading,
            is_active: this.editCustomer.is_active,
          })
        })

        if (res.status === 422) {
          const json = await res.json()
          this.submitFieldErrors = json?.errors || {}
          return
        }

        if (!res.ok) {
          throw new Error('Failed to update customer')
        }

        this.isEditModalOpen = false
        this.editingCustomer = null
        await this.fetchCustomers()

        // Show success modal
        this.successTitle = 'Client Updated'
        this.successMessage = `Client <strong>${this.editCustomer.customer_name}</strong> has been successfully updated.`
        this.isSuccessModalOpen = true
      } catch (e) {
        this.submitError = e?.message || 'Failed to update customer'
      } finally {
        this.isSubmitting = false
      }
    },
    fieldError(field) {
      const v = this.submitFieldErrors?.[field]
      if (!v) return ''
      return Array.isArray(v) ? v[0] : String(v)
    },
    async submitNewCustomer() {
      this.isSubmitting = true
      this.submitError = ''
      this.submitFieldErrors = {}

      const csrf = document.querySelector('meta[name="csrf-token"]')?.getAttribute('content')
      try {
        const res = await fetch('/customers', {
          method: 'POST',
          headers: {
            'Accept': 'application/json',
            'Content-Type': 'application/json',
            ...(csrf ? { 'X-CSRF-TOKEN': csrf } : {}),
          },
          credentials: 'same-origin',
          body: JSON.stringify({
            account_no: this.newCustomer.account_no,
            customer_name: this.newCustomer.customer_name,
            address: this.newCustomer.address,
            previous_reading: this.newCustomer.previous_reading,
            is_active: this.newCustomer.is_active,
          })
        })

        if (res.status === 422) {
          const json = await res.json()
          this.submitFieldErrors = json?.errors || {}
          return
        }

        if (!res.ok) {
          throw new Error('Failed to save customer')
        }

        this.isAddModalOpen = false
        await this.fetchCustomers()

        // Show success modal
        this.successTitle = 'Client Added'
        this.successMessage = `Client <strong>${this.newCustomer.customer_name}</strong> has been successfully added.`
        this.isSuccessModalOpen = true
      } catch (e) {
        this.submitError = e?.message || 'Failed to save customer'
      } finally {
        this.isSubmitting = false
      }
    }
    ,
    formatReadingAt(value) {
      if (!value) return '-'
      const d = new Date(value)
      if (Number.isNaN(d.getTime())) return String(value)
      return d.toLocaleString()
    },
    formatMoney(value) {
      const n = Number(value)
      if (Number.isNaN(n)) return '0.00'
      return n.toFixed(2)
    },
    isBillDue(r) {
      if (!r?.due_date) return false
      const due = new Date(r.due_date)
      if (Number.isNaN(due.getTime())) return false
      const today = new Date()
      today.setHours(0, 0, 0, 0)
      due.setHours(0, 0, 0, 0)
      return due < today
    },
    async markBillPaid(readingRow) {
      if (!readingRow?.bill_id) return
      const ok = window.confirm('Confirm payment? This will mark the bill as PAID.')
      if (!ok) return

      const csrf = document.querySelector('meta[name="csrf-token"]')?.getAttribute('content')
      try {
        const res = await fetch(`/bills/${readingRow.bill_id}/mark-paid`, {
          method: 'POST',
          headers: {
            'Accept': 'application/json',
            ...(csrf ? { 'X-CSRF-TOKEN': csrf } : {}),
          },
          credentials: 'same-origin',
        })

        if (!res.ok) {
          if (res.status === 401) throw new Error('Unauthorized. Please login again.')
          const text = await res.text()
          throw new Error('Failed to mark paid: ' + text)
        }

        // Refresh modal + list badges
        if (this.usageCustomer) {
          await this.openUsageModal(this.usageCustomer)
        }
        await this.fetchCustomers()

        // Show success modal
        this.successTitle = 'Bill Marked as Paid'
        this.successMessage = `The bill has been successfully marked as paid.`
        this.isSuccessModalOpen = true
      } catch (e) {
        this.usageError = e?.message || 'Failed to mark bill as paid'
      }
    },
    async openUsageModal(customer) {
      this.isUsageModalOpen = true
      this.usageCustomer = customer
      this.usageReadings = []
      this.usageError = ''
      this.usageLoading = true

      try {
        const res = await fetch(`/customers/${customer.id}/readings`, {
          headers: { 'Accept': 'application/json' },
          credentials: 'same-origin',
        })

        if (!res.ok) {
          if (res.status === 401) {
            throw new Error('Unauthorized. Please login again.')
          }
          throw new Error('Failed to load readings')
        }

        const json = await res.json()
        this.usageReadings = Array.isArray(json?.data) ? json.data : []
      } catch (e) {
        this.usageError = e?.message || 'Failed to load readings'
      } finally {
        this.usageLoading = false
      }
    },
    closeUsageModal() {
      if (this.usageLoading) return
      this.isUsageModalOpen = false
    }
  }
}
</script>
