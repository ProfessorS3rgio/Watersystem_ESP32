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
              <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Type</th>
              <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Barangay</th>
              <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Address</th>
              <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Previous Reading</th>
              <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Current Reading</th>
              <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Latest Bill</th>
              <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Status</th>
              <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Usage</th>
              <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider w-48">Actions</th>
            </tr>
          </thead>
          <tbody class="bg-white divide-y divide-gray-200">
            <tr v-if="isLoading">
              <td class="px-6 py-6 text-sm text-gray-600" colspan="9">Loading customers...</td>
            </tr>
            <tr v-if="errorMessage">
              <td class="px-6 py-6 text-sm text-red-600" colspan="11">{{ errorMessage }}</td>
            </tr>
            <tr v-else-if="filteredCustomers.length === 0">
              <td class="px-6 py-6 text-sm text-gray-600" colspan="11">No customers found.</td>
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
              <td class="px-6 py-4 whitespace-nowrap text-sm text-gray-900">{{ getTypeName(customer.type_id) }}</td>
              <td class="px-6 py-4 whitespace-nowrap text-sm text-gray-900">{{ getBarangayName(customer.brgy_id) }}</td>
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
                <span :class="customer.status === 'active' ? 'bg-green-100 text-green-800' : 'bg-red-100 text-red-800'" class="px-3 py-1 rounded-full text-xs font-medium">
                  {{ customer.status === 'active' ? 'Active' : 'Disconnected' }}
                </span>
              </td>
              <td class="px-6 py-4 whitespace-nowrap text-sm">
                <button @click="openUsageModal(customer)" class="text-indigo-600 hover:text-indigo-900 font-medium">
                  View Usage
                </button>
              </td>
              <td class="px-6 py-4 whitespace-nowrap text-sm">
                <div class="flex flex-wrap gap-2">
                  <button
                    v-if="customer.status === 'active'"
                    @click="disconnectCustomer(customer)"
                    class="text-white bg-red-500 box-border border border-transparent hover:bg-red-600 focus:ring-4 focus:ring-red-300 shadow-xs font-medium leading-5 rounded-full text-sm px-3 py-1.5 focus:outline-none"
                    title="Disconnect this customer"
                  >
                    Disconnect
                  </button>
                  <button
                    v-else
                    @click="reconnectCustomer(customer)"
                    class="text-white bg-green-500 box-border border border-transparent hover:bg-green-600 focus:ring-4 focus:ring-green-300 shadow-xs font-medium leading-5 rounded-full text-sm px-3 py-1.5 focus:outline-none"
                    title="Reconnect this customer"
                  >
                    Reconnect
                  </button>
                  <button @click="openEditModal(customer)" class="text-white bg-blue-500 box-border border border-transparent hover:bg-blue-600 focus:ring-4 focus:ring-blue-300 shadow-xs font-medium leading-5 rounded-full text-sm px-3 py-1.5 focus:outline-none">
                    Edit
                  </button>
                  <button @click="openDeleteModal(customer)" class="text-white bg-red-500 box-border border border-transparent hover:bg-red-600 focus:ring-4 focus:ring-red-300 shadow-xs font-medium leading-5 rounded-full text-sm px-3 py-1.5 focus:outline-none">
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
    }
  }
}
</script>