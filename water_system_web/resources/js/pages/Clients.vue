<template>
  <AppSidebar v-slot="{ isDark }">
    <div class="p-8">
      <div class="mb-8">
        <h1 class="text-3xl font-bold mb-2 transition-colors duration-200" :class="isDark ? 'text-white' : 'text-gray-900'">Client Management</h1>
        <p class="transition-colors duration-200" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Manage your water system clients and their accounts</p>
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

     <div class="mb-6">
      <button @click="openAddModal" class="bg-indigo-600 hover:bg-indigo-700 text-white px-6 py-3 rounded-lg font-medium shadow-lg transition-all duration-200 flex items-center space-x-2">
        <svg class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
          <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 6v6m0 0v6m0-6h6m-6 0H6" />
        </svg>
        <span>Add New Client</span>
      </button>
    </div>
    <!-- Clients Table -->
    <div class="rounded-lg shadow overflow-hidden" :class="isDark ? 'bg-gray-800' : 'bg-white'">
      <div class="px-6 py-4 border-b flex items-center justify-between" :class="isDark ? 'border-gray-700' : 'border-gray-200'">
        <h2 class="text-lg font-semibold" :class="isDark ? 'text-white' : 'text-gray-900'">Client List</h2>
        <div class="flex items-center space-x-4">
          <input
            type="search"
            placeholder="Search clients..."
            v-model="search"
            class="px-4 py-2 border rounded-lg focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500"
            :class="isDark ? 'bg-gray-700 border-gray-600 text-white placeholder-gray-400' : 'bg-white border-gray-300 text-gray-900 placeholder-gray-500'"
          />
        </div>
      </div>
      <div class="overflow-x-auto">
        <table class="w-full">
          <thead :class="isDark ? 'bg-gray-700' : 'bg-gray-50'">
            <tr>
              <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-300' : 'text-gray-500'">Account No</th>
              <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-300' : 'text-gray-500'">Customer Name</th>
              <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-300' : 'text-gray-500'">Previous</th>
              <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-300' : 'text-gray-500'">Present</th>
              <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-300' : 'text-gray-500'">Latest Bill</th>
              <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-300' : 'text-gray-500'">Status</th>
              <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider" :class="isDark ? 'text-gray-300' : 'text-gray-500'">Usage</th>
              <th class="px-4 py-2 text-left text-xs font-medium uppercase tracking-wider w-48" :class="isDark ? 'text-gray-300' : 'text-gray-500'">Actions</th>
            </tr>
          </thead>
          <tbody :class="isDark ? 'bg-gray-800 divide-gray-700' : 'bg-white divide-gray-200'" class="divide-y">
            <tr v-if="isLoading">
              <td class="px-6 py-6 text-sm" :class="isDark ? 'text-gray-300' : 'text-gray-600'" colspan="8">Loading customers...</td>
            </tr>
            <tr v-if="errorMessage">
              <td class="px-6 py-6 text-sm text-red-600" colspan="8">{{ errorMessage }}</td>
            </tr>
            <tr v-else-if="filteredCustomers.length === 0">
              <td class="px-6 py-6 text-sm" :class="isDark ? 'text-gray-300' : 'text-gray-600'" colspan="8">No customers found.</td>
            </tr>
            <tr v-else v-for="customer in filteredCustomers" :key="customer.id" :class="isDark ? 'hover:bg-gray-700' : 'hover:bg-gray-50'">
              <td class="px-4 py-2.5 whitespace-nowrap text-sm font-semibold" :class="isDark ? 'text-purple-300' : 'text-purple-600'">{{ customer.account_no }}</td>
              <td class="px-4 py-2.5 whitespace-nowrap">
                <div class="flex items-center">
                  <div class="h-7 w-7 rounded-full flex items-center justify-center" :class="isDark ? 'bg-indigo-900' : 'bg-indigo-100'">
                    <span class="font-medium text-xs" :class="isDark ? 'text-indigo-200' : 'text-indigo-600'">{{ (customer.customer_name || '?').charAt(0) }}</span>
                  </div>
                  <div class="ml-2">
                    <p class="text-sm font-medium" :class="isDark ? 'text-white' : 'text-gray-900'">{{ customer.customer_name }}</p>
                  </div>
                </div>
              </td>
              <td class="px-4 py-2.5 whitespace-nowrap text-sm font-semibold" :class="isDark ? 'text-blue-300' : 'text-blue-600'">{{ customer.previous_reading }} m³</td>
              <td class="px-4 py-2.5 whitespace-nowrap text-sm font-semibold" :class="isDark ? 'text-green-300' : 'text-green-600'">{{ customer.current_reading ?? '-' }}{{ customer.current_reading ? ' m³' : '' }}</td>
              <td class="px-4 py-2.5 whitespace-nowrap text-sm">
                <span v-if="customer.latest_bill_state === 'paid'" class="px-2.5 py-0.5 rounded-full text-xs font-medium" :class="isDark ? 'bg-green-900 text-green-200' : 'bg-green-100 text-green-800'">Paid</span>
                <span v-else-if="customer.latest_bill_state === 'due'" class="px-2.5 py-0.5 rounded-full text-xs font-medium" :class="isDark ? 'bg-red-900 text-red-200' : 'bg-red-100 text-red-800'">Due</span>
                <span v-else-if="customer.latest_bill_state === 'pending'" class="px-2.5 py-0.5 rounded-full text-xs font-medium" :class="isDark ? 'bg-yellow-900 text-yellow-200' : 'bg-yellow-100 text-yellow-800'">Pending</span>
                <span v-else class="px-2.5 py-0.5 rounded-full text-xs font-medium" :class="isDark ? 'bg-gray-700 text-gray-200' : 'bg-gray-100 text-gray-700'">No Bill</span>
              </td>
              <td class="px-4 py-2.5 whitespace-nowrap">
                <span :class="customer.status === 'active' ? (isDark ? 'bg-green-900 text-green-200' : 'bg-green-100 text-green-800') : (isDark ? 'bg-red-900 text-red-200' : 'bg-red-100 text-red-800')" class="px-2.5 py-0.5 rounded-full text-xs font-medium">
                  {{ customer.status === 'active' ? 'Active' : 'Disconnected' }}
                </span>
              </td>
              <td class="px-4 py-2.5 whitespace-nowrap text-sm">
                <button @click="openUsageModal(customer)" class="font-medium text-xs hover:underline" :class="isDark ? 'text-indigo-400 hover:text-indigo-300' : 'text-indigo-600 hover:text-indigo-900'">
                  View Usage
                </button>
              </td>
              <td class="px-4 py-2.5 whitespace-nowrap text-sm">
                <div class="flex flex-wrap gap-1.5">
                  <button @click="openViewModal(customer)" class="text-white box-border border border-transparent hover:bg-indigo-700 focus:ring-4 focus:ring-indigo-300 shadow-xs font-medium leading-5 rounded-full text-xs px-2.5 py-1 focus:outline-none bg-indigo-600">
                    View
                  </button>
                  <button
                    v-if="customer.status === 'active'"
                    @click="disconnectCustomer(customer)"
                    class="text-white box-border border border-transparent hover:bg-red-700 focus:ring-4 focus:ring-red-300 shadow-xs font-medium leading-5 rounded-full text-xs px-2.5 py-1 focus:outline-none bg-red-600"
                    title="Disconnect this customer"
                  >
                    Disconnect
                  </button>
                  <button
                    v-else
                    @click="reconnectCustomer(customer)"
                    class="text-white box-border border border-transparent hover:bg-green-700 focus:ring-4 focus:ring-green-300 shadow-xs font-medium leading-5 rounded-full text-xs px-2.5 py-1 focus:outline-none bg-green-600"
                    title="Reconnect this customer"
                  >
                    Reconnect
                  </button>
                  <button @click="openEditModal(customer)" class="text-white box-border border border-transparent hover:bg-blue-700 focus:ring-4 focus:ring-blue-300 shadow-xs font-medium leading-5 rounded-full text-xs px-2.5 py-1 focus:outline-none bg-blue-600">
                    Edit
                  </button>
                  <button @click="openDeleteModal(customer)" class="text-white box-border border border-transparent hover:bg-red-700 focus:ring-4 focus:ring-red-300 shadow-xs font-medium leading-5 rounded-full text-xs px-2.5 py-1 focus:outline-none bg-red-600">
                    Remove
                  </button>
                </div>
              </td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>

    <div v-if="isViewModalOpen" class="fixed inset-0 z-50">
      <div class="absolute inset-0 bg-black/50" @click="closeViewModal"></div>
      <div class="absolute inset-0 flex items-center justify-center p-4">
        <div class="w-full max-w-2xl rounded-xl shadow-2xl" :class="isDark ? 'bg-gray-900 text-white' : 'bg-white text-gray-900'">
          <div class="px-6 py-4 border-b" :class="isDark ? 'border-gray-800' : 'border-gray-200'">
            <div class="flex items-start justify-between gap-4">
              <div>
                <h3 class="text-lg font-semibold">Client Details</h3>
                <p class="text-sm mt-1" :class="isDark ? 'text-gray-300' : 'text-gray-600'">
                  {{ viewingCustomer?.customer_name || '-' }} • <span class="font-semibold" :class="isDark ? 'text-purple-300' : 'text-purple-600'">{{ viewingCustomer?.account_no || '-' }}</span>
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

            <div class="rounded-lg p-3 mt-3" :class="isDark ? 'bg-gray-800/80' : 'bg-gray-50'">
              <p class="text-xs uppercase tracking-wide" :class="isDark ? 'text-gray-400' : 'text-gray-500'">Address</p>
              <p class="text-sm mt-1">{{ viewingCustomer?.address || '-' }}</p>
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
      @void-bill="voidBill"
    />

    <!-- Client Form -->
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
import { ref, computed } from 'vue'
import AppSidebar from '../components/AppSidebar.vue'
import ConfirmDialog from '../components/ConfirmDialog.vue'
import SuccessModal from '../components/SuccessModal.vue'
import UsageModal from '../components/UsageModal.vue'
import StatsCards from '../components/StatsCards.vue'
import ClientForm from '../components/ClientForm.vue'
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
    ClientForm
  },
  setup() {
    const customersComposable = useCustomers()
    const modalsComposable = useModals()
    const formComposable = useCustomerForm()
    const usageComposable = useUsage()
    const isViewModalOpen = ref(false)
    const viewingCustomer = ref(null)
    const selectedBarangay = ref(null)

    const filteredCustomers = computed(() => {
      let filtered = customersComposable.filteredCustomers.value
      if (selectedBarangay.value !== null) {
        filtered = filtered.filter(customer => customer.brgy_id === selectedBarangay.value)
      }
      return filtered
    })

    const openViewModal = (customer) => {
      viewingCustomer.value = customer
      isViewModalOpen.value = true
    }

    const closeViewModal = () => {
      isViewModalOpen.value = false
      viewingCustomer.value = null
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
        account_no: customer.account_no || '',
        customer_name: customer.customer_name || '',
        type_id: customer.type_id || null,
        deduction_id: customer.deduction_id || null,
        brgy_id: customer.brgy_id || null,
        address: customer.address || '',
        previous_reading: customer.previous_reading || 0,
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

    const openUsageModal = async (customer) => {
      modalsComposable.openUsageModal(customer)
      await usageComposable.fetchUsageReadings(customer.id)
    }

    const closeUsageModal = () => {
      if (usageComposable.usageLoading.value) return
      modalsComposable.closeUsageModal()
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
          status: formComposable.editCustomer.status,
        })

        modalsComposable.closeEditModal()
        modalsComposable.showSuccess(
          'Client Updated',
          `Client <strong>${formComposable.editCustomer.customer_name}</strong> has been successfully updated.`
        )
      } catch (e) {
        if (e.message.includes('Validation error')) {
          formComposable.setFieldErrors(JSON.parse(e.message.replace('Validation error: ', '')))
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

    const markBillPaid = async (readingRow) => {
      if (!readingRow?.bill_id) return
      const ok = window.confirm('Confirm payment? This will mark the bill as PAID.')
      if (!ok) return

      try {
        await usageComposable.markBillPaid(readingRow.bill_id)

        // Refresh usage data and customer list
        if (modalsComposable.usageCustomer.value) {
          await usageComposable.fetchUsageReadings(modalsComposable.usageCustomer.value.id)
        }
        await customersComposable.fetchCustomers()

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

        // Refresh usage data and customer list
        if (modalsComposable.usageCustomer.value) {
          await usageComposable.fetchUsageReadings(modalsComposable.usageCustomer.value.id)
        }
        await customersComposable.fetchCustomers()

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

    const previewAccountNoNew = computed(() => {
      const brgyId = formComposable.newCustomer.brgy_id
      if (!brgyId) return ''
      const brgy = customersComposable.barangays.value.find(b => b.brgy_id === brgyId)
      if (!brgy) return ''
      const num = String(brgy.next_number ?? 0).padStart(3, '0')
      return `${brgy.prefix}-${num}`
    })

    // Initialize data
    customersComposable.fetchCustomers()
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
      openViewModal,
      closeViewModal,
      disconnectCustomer,
      reconnectCustomer,
      confirmDelete,
      submitEditCustomer,
      submitNewCustomer,
      markBillPaid,
      voidBill,
      inputClass,
      getTypeName,
      getBarangayName,
      previewAccountNoNew,
      isViewModalOpen,
      viewingCustomer,
      selectedBarangay,
      filteredCustomers,
    }
  }
}
</script>