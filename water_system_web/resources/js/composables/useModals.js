import { ref } from 'vue'

export function useModals() {
  // Add/Edit modals
  const isAddModalOpen = ref(false)
  const isEditModalOpen = ref(false)
  const editingCustomer = ref(null)

  // Delete modal
  const isDeleteModalOpen = ref(false)
  const deletingCustomer = ref(null)

  // Success modal
  const isSuccessModalOpen = ref(false)
  const successTitle = ref('')
  const successMessage = ref('')

  // Usage modal
  const isUsageModalOpen = ref(false)
  const usageCustomer = ref(null)

  const openAddModal = () => {
    isAddModalOpen.value = true
  }

  const closeAddModal = () => {
    isAddModalOpen.value = false
  }

  const openEditModal = (customer) => {
    editingCustomer.value = customer
    isEditModalOpen.value = true
  }

  const closeEditModal = () => {
    isEditModalOpen.value = false
    editingCustomer.value = null
  }

  const openDeleteModal = (customer) => {
    deletingCustomer.value = customer
    isDeleteModalOpen.value = true
  }

  const closeDeleteModal = () => {
    isDeleteModalOpen.value = false
    deletingCustomer.value = null
  }

  const openUsageModal = (customer) => {
    usageCustomer.value = customer
    isUsageModalOpen.value = true
  }

  const closeUsageModal = () => {
    isUsageModalOpen.value = false
    usageCustomer.value = null
  }

  const showSuccess = (title, message) => {
    successTitle.value = title
    successMessage.value = message
    isSuccessModalOpen.value = true
  }

  const closeSuccess = () => {
    isSuccessModalOpen.value = false
  }

  return {
    // States
    isAddModalOpen,
    isEditModalOpen,
    editingCustomer,
    isDeleteModalOpen,
    deletingCustomer,
    isSuccessModalOpen,
    successTitle,
    successMessage,
    isUsageModalOpen,
    usageCustomer,

    // Methods
    openAddModal,
    closeAddModal,
    openEditModal,
    closeEditModal,
    openDeleteModal,
    closeDeleteModal,
    openUsageModal,
    closeUsageModal,
    showSuccess,
    closeSuccess
  }
}