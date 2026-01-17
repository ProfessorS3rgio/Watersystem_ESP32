<template>
  <!-- Add Client Modal -->
  <div v-if="isAddModalOpen" class="fixed inset-0 z-50">
    <div class="absolute inset-0 bg-black/50" @click="$emit('close-add-modal')"></div>
    <div class="absolute inset-0 flex items-center justify-center p-4">
      <div class="w-full max-w-4xl rounded-lg shadow-lg" :class="isDark ? 'bg-gray-900 text-white' : 'bg-white text-gray-900'">
        <div class="px-6 py-4 border-b" :class="isDark ? 'border-gray-800' : 'border-gray-200'">
          <div class="flex items-center justify-between">
            <h3 class="text-lg font-semibold">Add New Client</h3>
            <button class="text-sm" :class="isDark ? 'text-gray-300 hover:text-white' : 'text-gray-600 hover:text-gray-900'" @click="$emit('close-add-modal')">
              Close
            </button>
          </div>
        </div>

        <form class="px-6 py-4 space-y-6" @submit.prevent="$emit('submit-new-customer')">
          <!-- Row 1: Customer Name -->
          <div class="grid grid-cols-1 gap-4">
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
            
              <div class="mt-3">
                <label class="block text-sm font-medium mb-1">Account No (preview)</label>
                <input type="text" :value="previewAccountNoNew" readonly class="w-full px-4 py-2 rounded-lg border bg-gray-50 text-gray-700" :class="isDark ? 'bg-gray-800 text-white' : ''" />
              </div>
            </div>
          </div>

          <!-- Row 2: Type and Barangay -->
          <div class="grid grid-cols-2 gap-4">
            <div>
              <label class="block text-sm font-medium mb-1">Type</label>
              <select
                v-model="newCustomer.type_id"
                class="w-full px-4 py-2 rounded-lg border"
                :class="inputClass(isDark)"
                required
              >
                <option :value="null">Select Type</option>
                <option v-for="type in customerTypes" :key="type.type_id" :value="type.type_id">
                  {{ type.type_name }}
                </option>
              </select>
              <p v-if="fieldError('type_id')" class="text-sm text-red-600 mt-1">{{ fieldError('type_id') }}</p>
            </div>

            <div>
              <label class="block text-sm font-medium mb-1">Barangay</label>
              <select
                v-model="newCustomer.brgy_id"
                class="w-full px-4 py-2 rounded-lg border"
                :class="inputClass(isDark)"
                required
              >
                <option :value="null">Select Barangay</option>
                <option v-for="brgy in barangays" :key="brgy.brgy_id" :value="brgy.brgy_id">
                  {{ brgy.barangay }}
                </option>
              </select>
              <p v-if="fieldError('brgy_id')" class="text-sm text-red-600 mt-1">{{ fieldError('brgy_id') }}</p>
            </div>
          </div>

          <!-- Row 3: Address (full width) -->
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

          <!-- Row 4: Previous Reading and Status -->
          <div class="grid grid-cols-2 gap-4">
            <div>
              <label class="block text-sm font-medium mb-1">Previous Reading</label>
              <input
                v-model.number="newCustomer.previous_reading"
                type="number"
                min="0"
                class="w-full px-4 py-2 rounded-lg border"
                :class="inputClass(isDark)"
              />
              <p v-if="fieldError('previous_reading')" class="text-sm text-red-600 mt-1">{{ fieldError('previous_reading') }}</p>
            </div>

            <div>
              <label class="block text-sm font-medium mb-1">Status</label>
              <select
                v-model="newCustomer.status"
                class="w-full px-4 py-2 rounded-lg border"
                :class="inputClass(isDark)"
                required
              >
                <option value="active">Active</option>
                <option value="disconnected">Disconnected</option>
              </select>
              <p v-if="fieldError('status')" class="text-sm text-red-600 mt-1">{{ fieldError('status') }}</p>
            </div>
          </div>

          <!-- Row 5: Benefits (custom dropdown) -->
          <div>
            <label class="block text-sm font-medium mb-1">Benefits</label>
            <div class="relative">
              <button type="button" @click="toggleBenefitsNew" class="w-full text-left px-4 py-2 rounded-lg border flex items-center justify-between" :class="inputClass(isDark)">
                <span class="truncate">
                  <template v-if="newCustomer.benefits.length">{{ getDeductionNames(newCustomer.benefits).join(', ') }}</template>
                  <template v-else class="text-gray-400">Select benefits</template>
                </span>
                <svg class="h-4 w-4 text-gray-600 ml-2" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M19 9l-7 7-7-7"/></svg>
              </button>

              <div v-if="showBenefitsNew" class="absolute mt-1 w-full bg-white border rounded shadow max-h-48 overflow-auto z-40">
                <div v-for="deduction in deductions" :key="deduction.deduction_id" class="px-3 py-2 hover:bg-gray-50 flex items-center">
                  <input type="checkbox" :id="`new-benefit-${deduction.deduction_id}`" class="mr-2" :checked="newCustomer.benefits.includes(deduction.deduction_id)" @change="toggleBenefit(newCustomer.benefits, deduction.deduction_id)" />
                  <label :for="`new-benefit-${deduction.deduction_id}`" class="flex-1">{{ deduction.name }}</label>
                </div>
              </div>
            </div>
            <p v-if="fieldError('benefits')" class="text-sm text-red-600 mt-1">{{ fieldError('benefits') }}</p>
          </div>

          <p v-if="submitError" class="text-sm text-red-600">{{ submitError }}</p>

          <div class="pt-2 flex items-center justify-end gap-3">
            <button type="button" @click="$emit('close-add-modal')" class="px-4 py-2 rounded-lg" :class="isDark ? 'bg-gray-800 hover:bg-gray-700 text-white' : 'bg-gray-100 hover:bg-gray-200 text-gray-900'">
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
    <div class="absolute inset-0 bg-black/50" @click="$emit('close-edit-modal')"></div>
    <div class="absolute inset-0 flex items-center justify-center p-4">
      <div class="w-full max-w-4xl rounded-lg shadow-lg" :class="isDark ? 'bg-gray-900 text-white' : 'bg-white text-gray-900'">
        <div class="px-6 py-4 border-b" :class="isDark ? 'border-gray-800' : 'border-gray-200'">
          <div class="flex items-center justify-between">
            <h3 class="text-lg font-semibold">Edit Client</h3>
            <button class="text-sm" :class="isDark ? 'text-gray-300 hover:text-white' : 'text-gray-600 hover:text-gray-900'" @click="$emit('close-edit-modal')">
              Close
            </button>
          </div>
        </div>

        <form class="px-6 py-4 space-y-6" @submit.prevent="$emit('submit-edit-customer')">
          <!-- Row 1: Account No and Customer Name -->
          <div class="grid grid-cols-2 gap-4">
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
          </div>

          <!-- Row 2: Type and Barangay -->
          <div class="grid grid-cols-2 gap-4">
            <div>
              <label class="block text-sm font-medium mb-1">Type</label>
              <select
                v-model="editCustomer.type_id"
                class="w-full px-4 py-2 rounded-lg border"
                :class="inputClass(isDark)"
                required
              >
                <option :value="null">Select Type</option>
                <option v-for="type in customerTypes" :key="type.type_id" :value="type.type_id">
                  {{ type.type_name }}
                </option>
              </select>
              <p v-if="fieldError('type_id')" class="text-sm text-red-600 mt-1">{{ fieldError('type_id') }}</p>
            </div>

            <div>
              <label class="block text-sm font-medium mb-1">Barangay</label>
              <select
                v-model="editCustomer.brgy_id"
                class="w-full px-4 py-2 rounded-lg border"
                :class="inputClass(isDark)"
                required
              >
                <option :value="null">Select Barangay</option>
                <option v-for="brgy in barangays" :key="brgy.brgy_id" :value="brgy.brgy_id">
                  {{ brgy.barangay }}
                </option>
              </select>
              <p v-if="fieldError('brgy_id')" class="text-sm text-red-600 mt-1">{{ fieldError('brgy_id') }}</p>
            </div>
          </div>

          <!-- Row 3: Address (full width) -->
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

          <!-- Row 4: Previous Reading and Status -->
          <div class="grid grid-cols-2 gap-4">
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

            <div>
              <label class="block text-sm font-medium mb-1">Status</label>
              <select
                v-model="editCustomer.status"
                class="w-full px-4 py-2 rounded-lg border"
                :class="inputClass(isDark)"
                required
              >
                <option value="active">Active</option>
                <option value="disconnected">Disconnected</option>
              </select>
              <p v-if="fieldError('status')" class="text-sm text-red-600 mt-1">{{ fieldError('status') }}</p>
            </div>
          </div>

          <!-- Row 5: Benefits (custom dropdown) -->
          <div>
            <label class="block text-sm font-medium mb-1">Benefits</label>
            <div class="relative">
              <button type="button" @click="toggleBenefitsEdit" class="w-full text-left px-4 py-2 rounded-lg border flex items-center justify-between" :class="inputClass(isDark)">
                <span class="truncate">
                  <template v-if="editCustomer.benefits.length">{{ getDeductionNames(editCustomer.benefits).join(', ') }}</template>
                  <template v-else class="text-gray-400">Select benefits</template>
                </span>
                <svg class="h-4 w-4 text-gray-600 ml-2" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M19 9l-7 7-7-7"/></svg>
              </button>

              <div v-if="showBenefitsEdit" class="absolute mt-1 w-full bg-white border rounded shadow max-h-48 overflow-auto z-40">
                <div v-for="deduction in deductions" :key="deduction.deduction_id" class="px-3 py-2 hover:bg-gray-50 flex items-center">
                  <input type="checkbox" :id="`edit-benefit-${deduction.deduction_id}`" class="mr-2" :checked="editCustomer.benefits.includes(deduction.deduction_id)" @change="toggleBenefit(editCustomer.benefits, deduction.deduction_id)" />
                  <label :for="`edit-benefit-${deduction.deduction_id}`" class="flex-1">{{ deduction.name }}</label>
                </div>
              </div>
            </div>
            <p v-if="fieldError('benefits')" class="text-sm text-red-600 mt-1">{{ fieldError('benefits') }}</p>
          </div>

          <p v-if="submitError" class="text-sm text-red-600">{{ submitError }}</p>

          <div class="pt-2 flex items-center justify-end gap-3">
            <button type="button" @click="$emit('close-edit-modal')" class="px-4 py-2 rounded-lg" :class="isDark ? 'bg-gray-800 hover:bg-gray-700 text-white' : 'bg-gray-100 hover:bg-gray-200 text-gray-900'">
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
</template>

<script>
import { ref } from 'vue'

export default {
  name: 'ClientForm',
  props: {
    isAddModalOpen: {
      type: Boolean,
      default: false
    },
    isEditModalOpen: {
      type: Boolean,
      default: false
    },
    isDark: {
      type: Boolean,
      default: false
    },
    newCustomer: {
      type: Object,
      default: () => ({})
    },
    editCustomer: {
      type: Object,
      default: () => ({})
    },
    customerTypes: {
      type: Array,
      default: () => []
    },
    barangays: {
      type: Array,
      default: () => []
    },
    deductions: {
      type: Array,
      default: () => []
    },
    isSubmitting: {
      type: Boolean,
      default: false
    },
    submitError: {
      type: String,
      default: ''
    },
    previewAccountNoNew: {
      type: String,
      default: ''
    },
    fieldError: {
      type: Function,
      default: () => ''
    }
  },
  emits: ['close-add-modal', 'close-edit-modal', 'submit-new-customer', 'submit-edit-customer'],
  setup(props) {
    const showBenefitsNew = ref(false)
    const showBenefitsEdit = ref(false)

    const toggleBenefitsNew = () => {
      showBenefitsNew.value = !showBenefitsNew.value
      if (showBenefitsNew.value) showBenefitsEdit.value = false
    }

    const toggleBenefitsEdit = () => {
      showBenefitsEdit.value = !showBenefitsEdit.value
      if (showBenefitsEdit.value) showBenefitsNew.value = false
    }

    const toggleBenefit = (list, id) => {
      const idx = list.indexOf(id)
      if (idx === -1) list.push(id)
      else list.splice(idx, 1)
    }

    const getDeductionNames = (ids) => {
      return ids.map(i => {
        const d = props.deductions.find(x => x.deduction_id === i)
        return d ? d.name : ''
      }).filter(Boolean)
    }

    const inputClass = (isDark) => {
      return isDark
        ? 'bg-gray-800 border-gray-700 text-white placeholder:text-gray-400 focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500'
        : 'bg-white border-gray-300 text-gray-900 placeholder:text-gray-400 focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500'
    }

    return {
      showBenefitsNew,
      showBenefitsEdit,
      toggleBenefitsNew,
      toggleBenefitsEdit,
      toggleBenefit,
      getDeductionNames,
      inputClass
    }
  }
}
</script>