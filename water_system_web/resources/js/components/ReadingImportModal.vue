<template>
  <div v-if="isOpen" :data-dark="isDark" class="dmbc-modal fixed inset-0 z-50">
    <div class="absolute inset-0 bg-black/50" @click="closeModal"></div>
    <div class="absolute inset-0 flex items-center justify-center p-2 lg:p-4">
      <div class="dmbc-dialog flex max-h-[96vh] w-[98vw] max-w-[1800px] flex-col rounded-lg shadow-xl" :class="isDark ? 'bg-gray-900 text-white' : 'bg-white text-gray-900'">
        <div class="dmbc-dialog-header flex items-center justify-between border-b px-6 py-4" :class="isDark ? 'border-gray-800' : 'border-gray-200'">
          <div>
            <h3 class="text-lg font-semibold">Import Meter Readings</h3>
            <p class="mt-1 text-sm" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Upload a meter-reading workbook with one or more barangay sheets and review every row before applying it.</p>
          </div>
          <button type="button" class="text-2xl leading-none" :class="isDark ? 'text-gray-400 hover:text-white' : 'text-gray-500 hover:text-gray-900'" aria-label="Close" @click="closeModal">&times;</button>
        </div>

        <div class="min-h-0 flex-1 overflow-y-auto px-4 py-4 lg:px-6 lg:py-5">
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
              <label for="reading-import-file" class="inline-block cursor-pointer bg-teal-600 px-4 py-2 font-medium text-white hover:bg-teal-700">
                Browse XLSX File
              </label>
              <p v-if="selectedFile" class="mt-4 font-medium text-green-600">Selected: {{ selectedFile.name }}</p>
              <p class="mt-3 text-xs" :class="mutedClass">Supports reading workbooks, numbered legacy lists, and monthly collection workbooks with meter numbers. Maximum file size: 5 MB.</p>
            </div>

            <div v-if="sheetOptions.length" class="border p-4" :class="panelClass">
              <label for="reading-import-sheet" class="block text-sm font-semibold">Worksheet</label>
              <select id="reading-import-sheet" v-model="selectedSheet" class="mt-2 w-full border px-3 py-2" :class="isDark ? 'border-gray-700 bg-gray-900 text-white' : 'border-gray-300 bg-white text-gray-900'">
                <option v-for="sheet in sheetOptions" :key="sheet.name" :value="sheet.name">
                  {{ sheet.name }} - {{ sheet.barangay }} ({{ sheet.rows }} rows, {{ sheet.billing_month_label }})
                </option>
              </select>
              <p class="mt-2 text-xs" :class="mutedClass">Only the selected worksheet will be reviewed and applied.</p>
            </div>

            <div class="border-l-4 border-amber-500 px-4 py-3 text-sm" :class="isDark ? 'bg-amber-950/40 text-amber-100' : 'bg-amber-50 text-amber-900'">
              Reviewing a file does not change the database. An automatic restore point is saved before confirmed changes are applied.
            </div>
          </div>

          <div v-else class="space-y-5">
            <div class="grid grid-cols-2 gap-3 md:grid-cols-6">
              <div class="border p-3" :class="panelClass">
                <p class="text-xs uppercase" :class="mutedClass">Barangay</p>
                <p class="mt-1 font-semibold">{{ preview.barangay }}</p>
              </div>
              <div class="border p-3" :class="panelClass">
                <p class="text-xs uppercase" :class="mutedClass">Workbook Month</p>
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
              <button type="button" class="px-3 py-2 font-medium" :class="isDark ? 'text-teal-300 hover:text-teal-200' : 'text-teal-700 hover:text-teal-900'" @click="resetPreview">Choose another file</button>
            </div>

            <div v-if="selectedErrorCount" class="border-l-4 border-red-500 px-4 py-3 text-sm" :class="isDark ? 'bg-red-950/40 text-red-100' : 'bg-red-50 text-red-900'">
              Unmark {{ selectedErrorCount }} error {{ selectedErrorCount === 1 ? 'row' : 'rows' }} in the Apply column to continue. Error rows are never sent to the database.
            </div>
            <div v-else-if="preview.summary.errors" class="border-l-4 border-amber-500 px-4 py-3 text-sm" :class="isDark ? 'bg-amber-950/40 text-amber-100' : 'bg-amber-50 text-amber-900'">
              {{ preview.summary.errors }} error {{ preview.summary.errors === 1 ? 'row is' : 'rows are' }} excluded. You can apply the {{ selectedImportableCount }} marked valid {{ selectedImportableCount === 1 ? 'row' : 'rows' }}.
            </div>
            <div v-else-if="importableCount === 0" class="border-l-4 border-green-500 px-4 py-3 text-sm" :class="isDark ? 'bg-green-950/40 text-green-100' : 'bg-green-50 text-green-900'">
              All workbook rows already match the database. There is nothing to update.
            </div>

            <div class="max-h-[55vh] overflow-y-auto overflow-x-hidden border" :class="isDark ? 'border-gray-700' : 'border-gray-200'">
              <table class="w-full table-fixed text-[10px] xl:text-xs 2xl:text-sm">
                <colgroup>
                  <col style="width: 4%" />
                  <col style="width: 3%" />
                  <col style="width: 6%" />
                  <col style="width: 16%" />
                  <col style="width: 7%" />
                  <col style="width: 7%" />
                  <col style="width: 7%" />
                  <col style="width: 7%" />
                  <col style="width: 5%" />
                  <col style="width: 8%" />
                  <col style="width: 8%" />
                  <col style="width: 8%" />
                  <col style="width: 8%" />
                  <col style="width: 8%" />
                  <col style="width: 14%" />
                </colgroup>
                <thead class="sticky top-0 z-10" :class="isDark ? 'bg-gray-800' : 'bg-gray-100'">
                  <tr>
                    <th class="px-1 py-2 text-center">
                      <input type="checkbox" :checked="allMarkableSelected" aria-label="Mark or unmark all review rows" @change="toggleAllRows" />
                      <span class="mt-1 block font-medium">Apply</span>
                    </th>
                    <th class="px-1 py-2 text-left">Row</th>
                    <th class="px-1 py-2 text-left">Account</th>
                    <th class="px-1 py-2 text-left">Customer</th>
                    <th class="px-1 py-2 text-right">DB Prev</th>
                    <th class="px-1 py-2 text-right">File Prev</th>
                    <th class="px-1 py-2 text-right">DB Present</th>
                    <th class="px-1 py-2 text-right">File Present</th>
                    <th class="px-1 py-2 text-right">Usage</th>
                    <th class="px-1 py-2 text-left">Bill Date</th>
                    <th class="px-1 py-2 text-left">DB Payment</th>
                    <th class="px-1 py-2 text-left">File Payment</th>
                    <th class="px-1 py-2 text-left">Bill action</th>
                    <th class="px-1 py-2 text-right">Proposed bill</th>
                    <th class="px-1 py-2 text-left">Review</th>
                  </tr>
                </thead>
                <tbody :class="isDark ? 'divide-gray-800' : 'divide-gray-200'" class="divide-y">
                  <tr v-for="row in preview.rows" :key="`${row.row}-${row.account_no}`" :class="reviewRowClass(row)">
                    <td class="px-1 py-2 text-center">
                      <input
                        type="checkbox"
                        :checked="isRowMarked(row)"
                        :disabled="!isRowMarkable(row)"
                        :aria-label="`${isRowMarked(row) ? 'Unmark' : 'Mark'} ${row.account_no || `row ${row.row}`}`"
                        @change="toggleRow(row)"
                      />
                    </td>
                    <td class="px-1 py-2" :class="mutedClass">{{ row.row }}</td>
                    <td class="whitespace-nowrap px-1 py-2 font-semibold">{{ row.account_no }}</td>
                    <td class="break-words px-1 py-2 leading-tight">{{ row.customer_name }}<p v-if="row.file_meter_number" class="mt-1" :class="mutedClass">Meter: {{ row.database_meter_number || '—' }} → {{ row.file_meter_number }}<span v-if="row.meter_update_skipped"> (update skipped)</span></p></td>
                    <td class="px-1 py-2 text-right">{{ valueOrDash(row.database_previous) }}</td>
                    <td class="px-1 py-2 text-right font-medium">{{ valueOrDash(row.file_previous) }}</td>
                    <td class="px-1 py-2 text-right">{{ valueOrDash(row.database_present) }}</td>
                    <td class="px-1 py-2 text-right font-medium">{{ valueOrDash(row.file_present) }}</td>
                    <td class="px-1 py-2 text-right">{{ valueOrDash(row.file_usage) }}</td>
                    <td class="px-1 py-2">{{ formatBillDate(row.bill_date) }}</td>
                    <td class="px-1 py-2">
                      <span v-if="row.database_payment_status" class="inline-block px-1 py-0.5 font-semibold" :class="paymentStatusClass(row.database_payment_status)">{{ row.database_payment_status }}</span>
                      <span v-else :class="mutedClass">-</span>
                    </td>
                    <td class="px-1 py-2">
                      <span v-if="row.file_payment_status" class="inline-block px-1 py-0.5 font-semibold" :class="paymentStatusClass(row.file_payment_status)">{{ row.file_payment_status }}</span>
                      <span v-else :class="mutedClass">-</span>
                      <p v-if="row.file_payment_amount !== undefined" class="mt-1">{{ formatCurrency(row.file_payment_amount) }}</p>
                    </td>
                    <td class="px-1 py-2">{{ row.bill_action || '-' }}</td>
                    <td class="px-1 py-2 text-right">{{ formatCurrency(row.proposed_bill_amount) }}</td>
                    <td class="break-words px-1 py-2 leading-tight">
                      <span class="inline-block px-1 py-0.5 font-semibold" :class="statusClass(row.status)">{{ statusLabel(row.status) }}</span>
                      <p class="mt-1" :class="mutedClass">{{ row.message }}</p>
                    </td>
                  </tr>
                </tbody>
              </table>
            </div>

            <p v-if="preview.format === 'monthly_collection'" class="text-xs" :class="mutedClass">Monthly collections combine both collection columns and apply to the existing bill for the workbook month. Zero or blank amounts leave payments unchanged. Meter numbers are matched by barangay, account sequence, and name. Penalty columns are ignored. Import readings first if the month's bill is missing.</p>
            <p v-else class="text-xs" :class="mutedClass">Valid reading rows create one Pending bill for the workbook month, or update that month's existing unpaid bill. The proposed bill includes the customer type rate, minimum-charge rule, and deduction. Blank workbook payment status means Paid; Not Paid and Not Pad mean Pending.</p>
          </div>

          <p v-if="error" class="mt-4 text-sm text-red-600">{{ error }}</p>
        </div>

        <div class="flex items-center justify-end gap-3 border-t px-6 py-4" :class="isDark ? 'border-gray-800' : 'border-gray-200'">
          <button type="button" class="px-4 py-2" :disabled="isReviewing || isConfirming" @click="closeModal">Cancel</button>
          <button v-if="!preview" type="button" class="bg-teal-600 px-4 py-2 font-medium text-white hover:bg-teal-700 disabled:opacity-50" :disabled="!selectedFile || isReviewing" @click="reviewFile">
            {{ isReviewing ? 'Reviewing...' : (sheetOptions.length ? 'Review Selected Worksheet' : 'Review File') }}
          </button>
          <button v-else type="button" class="bg-green-600 px-4 py-2 font-medium text-white hover:bg-green-700 disabled:opacity-50" :disabled="!canConfirm || isConfirming" @click="confirmImport">
            {{ isConfirming ? 'Applying...' : (selectedImportableCount === 0 ? 'No Marked Changes' : `Apply ${selectedImportableCount} Marked Changes`) }}
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
const selectedRowKeys = ref([])
const sheetOptions = ref([])
const selectedSheet = ref('')

const importableCount = computed(() => (preview.value?.summary?.changes || 0) + (preview.value?.summary?.warnings || 0))
const markableRows = computed(() => preview.value?.rows?.filter(isRowMarkable) || [])
const selectedErrorCount = computed(() => markableRows.value.filter(row => row.status === 'error' && isRowMarked(row)).length)
const selectedImportableRows = computed(() => markableRows.value.filter(row => ['change', 'warning'].includes(row.status) && isRowMarked(row)))
const selectedImportableCount = computed(() => selectedImportableRows.value.length)
const allMarkableSelected = computed(() => markableRows.value.length > 0 && markableRows.value.every(isRowMarked))
const canConfirm = computed(() => Boolean(preview.value?.token) && selectedErrorCount.value === 0 && selectedImportableCount.value > 0)
const panelClass = computed(() => props.isDark ? 'border-gray-700 bg-gray-800/60' : 'border-gray-200 bg-gray-50')
const mutedClass = computed(() => props.isDark ? 'text-gray-400' : 'text-gray-500')
const dropZoneClass = computed(() => {
  if (isDragging.value) return props.isDark ? 'border-teal-400 bg-teal-950/40' : 'border-teal-500 bg-teal-50'
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
  sheetOptions.value = []
  selectedSheet.value = ''

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
    if (selectedSheet.value) formData.append('sheet_name', selectedSheet.value)
    const response = await window.axios.post('/customers/readings-import/preview', formData, {
      headers: { 'Content-Type': 'multipart/form-data' },
    })
    if (response.data.requires_sheet_selection) {
      sheetOptions.value = response.data.sheets || []
      selectedSheet.value = sheetOptions.value[0]?.name || ''
      return
    }
    preview.value = response.data
    selectedRowKeys.value = markableRows.value.map(rowSelectionKey)
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
    const response = await window.axios.post('/customers/readings-import/confirm', {
      token: preview.value.token,
      selected_accounts: selectedImportableRows.value.map(row => row.account_no),
    })
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
  selectedRowKeys.value = []
  selectedFile.value = null
  error.value = ''
  sheetOptions.value = []
  selectedSheet.value = ''
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

function formatBillDate(value) {
  if (!value) return '-'
  return new Intl.DateTimeFormat('en-PH', { month: 'short', day: 'numeric', year: 'numeric', timeZone: 'UTC' }).format(new Date(`${value}T00:00:00Z`))
}

function formatCurrency(value) {
  if (value === null || value === undefined) return '-'
  return new Intl.NumberFormat('en-PH', { style: 'currency', currency: 'PHP' }).format(value)
}

function paymentStatusClass(status) {
  const paid = String(status).toLowerCase() === 'paid'
  if (props.isDark) return paid ? 'bg-green-900 text-green-200' : 'bg-amber-900 text-amber-200'
  return paid ? 'bg-green-100 text-green-800' : 'bg-amber-100 text-amber-800'
}

function rowSelectionKey(row) {
  return `${row.account_no || 'row'}-${row.row}`
}

function isRowMarkable(row) {
  return ['change', 'warning', 'error'].includes(row.status)
}

function isRowMarked(row) {
  return selectedRowKeys.value.includes(rowSelectionKey(row))
}

function toggleRow(row) {
  if (!isRowMarkable(row)) return
  const key = rowSelectionKey(row)
  selectedRowKeys.value = isRowMarked(row)
    ? selectedRowKeys.value.filter(value => value !== key)
    : [...selectedRowKeys.value, key]
}

function toggleAllRows(event) {
  selectedRowKeys.value = event.target.checked ? markableRows.value.map(rowSelectionKey) : []
}

function reviewRowClass(row) {
  if (!isRowMarked(row) && isRowMarkable(row)) return props.isDark ? 'opacity-50 bg-gray-900' : 'opacity-50 bg-gray-50'
  if (row.status === 'error') return props.isDark ? 'bg-red-950/20' : 'bg-red-50/60'
  return ''
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
