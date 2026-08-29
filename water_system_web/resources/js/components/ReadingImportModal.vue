<template>
  <div v-if="isOpen" class="fixed inset-0 z-50">
    <div class="absolute inset-0 bg-black/50" @click="closeModal"></div>
    <div class="absolute inset-0 flex items-center justify-center p-4">
      <div class="flex max-h-[92vh] w-full max-w-6xl flex-col rounded-lg shadow-xl" :class="isDark ? 'bg-gray-900 text-white' : 'bg-white text-gray-900'">
        <div class="flex items-center justify-between border-b px-6 py-4" :class="isDark ? 'border-gray-800' : 'border-gray-200'">
          <div>
            <h3 class="text-lg font-semibold">Import Meter Readings</h3>
            <p class="mt-1 text-sm" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Upload an exported monthly billing workbook and review every row before applying it.</p>
          </div>
          <button type="button" class="text-2xl leading-none" :class="isDark ? 'text-gray-400 hover:text-white' : 'text-gray-500 hover:text-gray-900'" aria-label="Close" @click="closeModal">&times;</button>
        </div>

        <div class="min-h-0 flex-1 overflow-y-auto px-6 py-5">
          <div v-if="!preview" class="mx-auto max-w-2xl space-y-5">
            <div
              class="border-2 border-dashed p-8 text-center transition-colors"
              :class="dropZoneClass"
              @dragenter.prevent="isDragging = true"
              @dragover.prevent="isDragging = true"
              @dragleave.prevent="isDragging = false"
              @drop.prevent="dropFile"
            >
              <input id="reading-import-file" ref="fileInput" type="file" accept=".xlsx" class="sr-only" @change="selectFile" />
              <p class="text-base font-semibold">{{ isDragging ? 'Drop the workbook here' : 'Drag and drop the workbook here' }}</p>
              <p class="my-2 text-sm" :class="mutedClass">or</p>
              <label for="reading-import-file" class="inline-block cursor-pointer bg-indigo-600 px-4 py-2 font-medium text-white hover:bg-indigo-700">
                Browse XLSX File
              </label>
              <p v-if="selectedFile" class="mt-4 font-medium text-green-600">Selected: {{ selectedFile.name }}</p>
              <p class="mt-3 text-xs" :class="mutedClass">Expected columns: Account Number, Previous Reading, and Present Reading. Maximum file size: 5 MB.</p>
            </div>

            <div class="border-l-4 border-amber-500 px-4 py-3 text-sm" :class="isDark ? 'bg-amber-950/40 text-amber-100' : 'bg-amber-50 text-amber-900'">
              Reviewing a file does not change the database. A separate confirmation is required after validation.
            </div>
          </div>

          <div v-else class="space-y-5">
            <div class="grid grid-cols-2 gap-3 md:grid-cols-6">
              <div class="border p-3" :class="panelClass">
                <p class="text-xs uppercase" :class="mutedClass">Barangay</p>
                <p class="mt-1 font-semibold">{{ preview.barangay }}</p>
              </div>
              <div class="border p-3" :class="panelClass">
                <p class="text-xs uppercase" :class="mutedClass">Billing Month</p>
                <p class="mt-1 font-semibold">{{ preview.billing_month_label }}</p>
              </div>
              <div class="border p-3" :class="panelClass">
                <p class="text-xs uppercase" :class="mutedClass">Changes</p>
                <p class="mt-1 text-lg font-semibold text-blue-600">{{ importableCount }}</p>
              </div>
              <div class="border p-3" :class="panelClass">
                <p class="text-xs uppercase" :class="mutedClass">Unchanged</p>
                <p class="mt-1 text-lg font-semibold">{{ preview.summary.unchanged }}</p>
              </div>
              <div class="border p-3" :class="panelClass">
                <p class="text-xs uppercase" :class="mutedClass">Skipped</p>
                <p class="mt-1 text-lg font-semibold text-amber-600">{{ preview.summary.skipped }}</p>
              </div>
              <div class="border p-3" :class="panelClass">
                <p class="text-xs uppercase" :class="mutedClass">Errors</p>
                <p class="mt-1 text-lg font-semibold" :class="preview.summary.errors ? 'text-red-600' : 'text-green-600'">{{ preview.summary.errors }}</p>
              </div>
            </div>

            <div class="flex flex-wrap items-center justify-between gap-3 text-sm">
              <div>
                <p class="font-medium">{{ preview.file_name }}</p>
                <p :class="mutedClass">Sheet: {{ preview.sheet_name }}</p>
              </div>
              <button type="button" class="px-3 py-2 font-medium" :class="isDark ? 'text-indigo-300 hover:text-indigo-200' : 'text-indigo-700 hover:text-indigo-900'" @click="resetPreview">Choose another file</button>
            </div>

            <div v-if="preview.summary.errors" class="border-l-4 border-red-500 px-4 py-3 text-sm" :class="isDark ? 'bg-red-950/40 text-red-100' : 'bg-red-50 text-red-900'">
              Fix the error rows in the workbook and upload it again. No rows can be applied while errors remain.
            </div>
            <div v-else-if="importableCount === 0" class="border-l-4 border-green-500 px-4 py-3 text-sm" :class="isDark ? 'bg-green-950/40 text-green-100' : 'bg-green-50 text-green-900'">
              All populated readings already match the database. There is nothing to update.
            </div>

            <div class="max-h-[420px] overflow-auto border" :class="isDark ? 'border-gray-700' : 'border-gray-200'">
              <table class="min-w-full text-sm">
                <thead class="sticky top-0 z-10" :class="isDark ? 'bg-gray-800' : 'bg-gray-100'">
                  <tr>
                    <th class="px-3 py-2 text-left">Row</th>
                    <th class="px-3 py-2 text-left">Account</th>
                    <th class="px-3 py-2 text-left">Customer</th>
                    <th class="px-3 py-2 text-right">DB Prev</th>
                    <th class="px-3 py-2 text-right">File Prev</th>
                    <th class="px-3 py-2 text-right">DB Present</th>
                    <th class="px-3 py-2 text-right">File Present</th>
                    <th class="px-3 py-2 text-right">Usage</th>
                    <th class="px-3 py-2 text-left">Review</th>
                  </tr>
                </thead>
                <tbody :class="isDark ? 'divide-gray-800' : 'divide-gray-200'" class="divide-y">
                  <tr v-for="row in preview.rows" :key="`${row.row}-${row.account_no}`" :class="row.status === 'error' ? (isDark ? 'bg-red-950/20' : 'bg-red-50/60') : ''">
                    <td class="px-3 py-2" :class="mutedClass">{{ row.row }}</td>
                    <td class="whitespace-nowrap px-3 py-2 font-semibold">{{ row.account_no }}</td>
                    <td class="min-w-48 px-3 py-2">{{ row.customer_name }}</td>
                    <td class="px-3 py-2 text-right">{{ valueOrDash(row.database_previous) }}</td>
                    <td class="px-3 py-2 text-right font-medium">{{ valueOrDash(row.file_previous) }}</td>
                    <td class="px-3 py-2 text-right">{{ valueOrDash(row.database_present) }}</td>
                    <td class="px-3 py-2 text-right font-medium">{{ valueOrDash(row.file_present) }}</td>
                    <td class="px-3 py-2 text-right">{{ valueOrDash(row.file_usage) }}</td>
                    <td class="min-w-56 px-3 py-2">
                      <span class="inline-block px-2 py-0.5 text-xs font-semibold" :class="statusClass(row.status)">{{ statusLabel(row.status) }}</span>
                      <p class="mt-1 text-xs" :class="mutedClass">{{ row.message }}</p>
                    </td>
                  </tr>
                </tbody>
              </table>
            </div>

            <p class="text-xs" :class="mutedClass">This import updates only previous reading, present reading, and usage for {{ preview.billing_month_label }}. Bill amounts, payment status, and newer readings are not changed.</p>
          </div>

          <p v-if="error" class="mt-4 text-sm text-red-600">{{ error }}</p>
        </div>

        <div class="flex items-center justify-end gap-3 border-t px-6 py-4" :class="isDark ? 'border-gray-800' : 'border-gray-200'">
          <button type="button" class="px-4 py-2" :disabled="isReviewing || isConfirming" @click="closeModal">Cancel</button>
          <button v-if="!preview" type="button" class="bg-indigo-600 px-4 py-2 font-medium text-white hover:bg-indigo-700 disabled:opacity-50" :disabled="!selectedFile || isReviewing" @click="reviewFile">
            {{ isReviewing ? 'Reviewing...' : 'Review File' }}
          </button>
          <button v-else type="button" class="bg-green-600 px-4 py-2 font-medium text-white hover:bg-green-700 disabled:opacity-50" :disabled="!canConfirm || isConfirming" @click="confirmImport">
            {{ isConfirming ? 'Applying...' : (importableCount === 0 ? 'No Changes to Apply' : `Apply ${importableCount} Changes`) }}
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { computed, ref, watch } from 'vue'

const props = defineProps({
  isOpen: { type: Boolean, default: false },
  isDark: { type: Boolean, default: false },
})

const emit = defineEmits(['close', 'imported'])
const fileInput = ref(null)
const selectedFile = ref(null)
const preview = ref(null)
const error = ref('')
const isReviewing = ref(false)
const isConfirming = ref(false)
const isDragging = ref(false)

const importableCount = computed(() => (preview.value?.summary?.changes || 0) + (preview.value?.summary?.warnings || 0))
const canConfirm = computed(() => Boolean(preview.value?.token) && preview.value.summary.errors === 0 && importableCount.value > 0)
const panelClass = computed(() => props.isDark ? 'border-gray-700 bg-gray-800/60' : 'border-gray-200 bg-gray-50')
const mutedClass = computed(() => props.isDark ? 'text-gray-400' : 'text-gray-500')
const dropZoneClass = computed(() => {
  if (isDragging.value) return props.isDark ? 'border-indigo-400 bg-indigo-950/40' : 'border-indigo-500 bg-indigo-50'
  return props.isDark ? 'border-gray-700 bg-gray-800/60' : 'border-gray-300 bg-gray-50'
})

watch(() => props.isOpen, (open) => {
  if (!open) resetAll()
})

function selectFile(event) {
  acceptFile(event.target.files?.[0] || null)
}

function dropFile(event) {
  isDragging.value = false
  acceptFile(event.dataTransfer?.files?.[0] || null)
}

function acceptFile(file) {
  preview.value = null
  error.value = ''
  selectedFile.value = null

  if (!file) return
  if (!file.name.toLowerCase().endsWith('.xlsx')) {
    error.value = 'Please select an XLSX workbook.'
    return
  }
  if (file.size > 5 * 1024 * 1024) {
    error.value = 'The workbook must be 5 MB or smaller.'
    return
  }

  selectedFile.value = file
}

async function reviewFile() {
  if (!selectedFile.value) return
  isReviewing.value = true
  error.value = ''
  try {
    const formData = new FormData()
    formData.append('file', selectedFile.value)
    const response = await window.axios.post('/customers/readings-import/preview', formData, {
      headers: { 'Content-Type': 'multipart/form-data' },
    })
    preview.value = response.data
  } catch (requestError) {
    error.value = firstError(requestError, 'The workbook could not be reviewed.')
  } finally {
    isReviewing.value = false
  }
}

async function confirmImport() {
  if (!canConfirm.value) return
  isConfirming.value = true
  error.value = ''
  try {
    const response = await window.axios.post('/customers/readings-import/confirm', { token: preview.value.token })
    emit('imported', { ...response.data, billing_month_label: preview.value.billing_month_label })
    emit('close')
  } catch (requestError) {
    error.value = firstError(requestError, 'The readings could not be applied.')
  } finally {
    isConfirming.value = false
  }
}

function firstError(requestError, fallback) {
  const errors = requestError.response?.data?.errors || {}
  const first = Object.values(errors).flat()[0]
  return first || requestError.response?.data?.message || fallback
}

function resetPreview() {
  preview.value = null
  selectedFile.value = null
  error.value = ''
  if (fileInput.value) fileInput.value.value = ''
}

function resetAll() {
  resetPreview()
  isReviewing.value = false
  isConfirming.value = false
  isDragging.value = false
}

function closeModal() {
  if (!isReviewing.value && !isConfirming.value) emit('close')
}

function valueOrDash(value) {
  return value === null || value === undefined ? '-' : value
}

function statusLabel(status) {
  return ({ change: 'Change', warning: 'Check name', unchanged: 'Unchanged', skipped: 'Skipped', error: 'Error' })[status] || status
}

function statusClass(status) {
  const light = {
    change: 'bg-blue-100 text-blue-800', warning: 'bg-amber-100 text-amber-800',
    unchanged: 'bg-gray-200 text-gray-700', skipped: 'bg-gray-200 text-gray-700', error: 'bg-red-100 text-red-800',
  }
  const dark = {
    change: 'bg-blue-900 text-blue-200', warning: 'bg-amber-900 text-amber-200',
    unchanged: 'bg-gray-700 text-gray-200', skipped: 'bg-gray-700 text-gray-200', error: 'bg-red-900 text-red-200',
  }
  return (props.isDark ? dark : light)[status] || ''
}
</script>
