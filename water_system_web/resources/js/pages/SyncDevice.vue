<template>
  <AppSidebar v-slot="{ isDark }">
    <div class="sync-workspace min-h-full space-y-6 p-5 sm:p-8 lg:p-10" :data-dark="isDark">
      <header class="flex flex-wrap items-start justify-between gap-4"><div><p class="mb-2 text-xs font-bold uppercase tracking-[.2em] text-teal-600">DMBC Water · Field operations</p><h1 class="text-3xl font-extrabold tracking-tight sm:text-4xl">Device sync center</h1><p class="mt-2 text-sm sync-muted">Bring your field records and association data together.</p></div><span class="sync-soft rounded-xl px-4 py-2 text-xs font-semibold">{{ transportMode === 'wifi' ? 'Wi-Fi hotspot · 192.168.4.1' : 'USB serial · 115200 baud' }}</span></header>
      <p v-if="navigationNotice && deviceBusy" role="status" class="rounded-xl bg-amber-50 p-4 text-sm text-amber-800">{{ navigationNotice }}</p>
      <SyncConnectionCard v-model:transport="transportMode" v-model:pairingCode="wifiPairingCode" :connected="isConnected" :connecting="isConnecting" :busy="deviceBusy" :syncing="isSyncing" :supported="serialSupported" :port="connectedPort" :error="connectionError" :stage="syncStage" :status="syncStatus" :sync-error="syncError" @connect="connectDevice" @disconnect="disconnectDevice" @sync="handleSyncData" />
      <nav aria-label="Device workspace" class="sync-panel flex flex-wrap gap-2 p-2">
        <RouterLink v-for="tab in [{ key: 'sync', label: 'Sync overview' }, { key: 'inspect', label: 'Reading inspector' }, { key: 'tools', label: 'Maintenance' }]" :key="tab.key" :to="{ path: '/sync', query: tab.key === 'sync' ? {} : { view: tab.key } }" :aria-current="activeWorkspace === tab.key && $route.path !== '/sync/normalize-dates' ? 'page' : undefined" class="flex-1 min-w-36 rounded-xl px-4 py-3 text-sm font-bold transition" :class="activeWorkspace === tab.key && $route.path !== '/sync/normalize-dates' ? 'bg-teal-700 text-white' : 'sync-muted hover:bg-teal-500/10'">{{ tab.label }}</RouterLink>
        <RouterLink to="/sync/normalize-dates" class="flex-1 min-w-36 rounded-xl px-4 py-3 text-center text-sm font-bold" :class="$route.path === '/sync/normalize-dates' ? 'bg-teal-700 text-white' : 'sync-muted hover:bg-teal-500/10'">Date normalization</RouterLink>
      </nav>
      <template v-if="activeWorkspace === 'sync' && $route.path !== '/sync/normalize-dates'">
        <div class="grid gap-4 md:grid-cols-2"><section class="sync-panel p-6"><p class="text-xs font-bold uppercase tracking-wider text-cyan-600">Device → Website</p><h2 class="mt-3 text-lg font-bold">Readings, bills &amp; payments</h2><p class="mt-2 text-sm leading-6 sync-muted">Import field activity so office accounts reflect the latest device records.</p></section><section class="sync-panel p-6"><p class="text-xs font-bold uppercase tracking-wider text-teal-600">Website → Device</p><h2 class="mt-3 text-lg font-bold">Members, rates &amp; reference data</h2><p class="mt-2 text-sm leading-6 sync-muted">Send account changes and billing settings to the field device. Each sync creates a web restore point before transferring data.</p></section></div>
        <SyncDeviceInfo :connected="isConnected" :details="deviceDetails" />
      </template>
    <div v-if="$route.path === '/sync/normalize-dates'" class="sync-panel mb-6 overflow-hidden" :class="isDark ? 'border-gray-700 bg-gray-800' : 'border-teal-100 bg-white'">
      <div class="bg-gradient-to-r from-teal-600 to-cyan-600 px-6 py-5 text-white">
        <div class="flex items-start gap-4">
          <div class="rounded-xl bg-white/15 p-3">
            <svg class="h-7 w-7" fill="none" viewBox="0 0 24 24" stroke="currentColor"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.8" d="M7 3v18m10-18v18M3 7h18M3 17h18" /></svg>
          </div>
          <div>
            <h2 class="text-xl font-semibold">Normalize Record Dates</h2>
            <p class="mt-1 text-sm text-teal-100">Move records from selected dates to one target date on the connected ESP32.</p>
          </div>
        </div>
      </div>
      <div class="p-6">
        <div class="grid gap-6 lg:grid-cols-[1fr_1fr_auto] lg:items-end">
          <label class="block">
            <span class="mb-2 block text-sm font-semibold" :class="isDark ? 'text-gray-200' : 'text-gray-700'">Target date</span>
            <input v-model="normalizeTargetDate" type="date" class="w-full rounded-xl border px-4 py-3 text-sm shadow-sm focus:border-teal-500 focus:ring-2 focus:ring-teal-500" :class="isDark ? 'border-gray-600 bg-gray-900 text-white' : 'border-gray-300 bg-white text-gray-900'" />
            <span class="mt-1 block text-xs" :class="isDark ? 'text-gray-400' : 'text-gray-500'">All selected records will use this date.</span>
          </label>
          <label class="block">
            <span class="mb-2 block text-sm font-semibold" :class="isDark ? 'text-gray-200' : 'text-gray-700'">Add source date</span>
            <div class="flex gap-2">
              <input v-model="normalizeSourceDateDraft" type="date" @keyup.enter="addNormalizeSourceDate" class="min-w-0 flex-1 rounded-xl border px-4 py-3 text-sm shadow-sm focus:border-teal-500 focus:ring-2 focus:ring-teal-500" :class="isDark ? 'border-gray-600 bg-gray-900 text-white' : 'border-gray-300 bg-white text-gray-900'" />
              <button type="button" @click="addNormalizeSourceDate" class="rounded-xl bg-teal-600 px-4 py-3 text-sm font-semibold text-white shadow-sm hover:bg-teal-700">Add</button>
            </div>
          </label>
          <button type="button" @click="normalizeDates" :disabled="deviceBusy || !isConnected" class="inline-flex items-center justify-center gap-2 rounded-xl bg-emerald-600 px-5 py-3 font-semibold text-white shadow-sm transition hover:bg-emerald-700 disabled:cursor-not-allowed disabled:opacity-50">
            <svg v-if="isNormalizing" class="h-5 w-5 animate-spin" fill="none" viewBox="0 0 24 24"><circle cx="12" cy="12" r="9" stroke="currentColor" stroke-width="3" stroke-dasharray="42 20" /></svg>
            <span>{{ isNormalizing ? 'Applying...' : 'Apply normalization' }}</span>
          </button>
        </div>
        <div class="mt-6 rounded-xl border p-4" :class="isDark ? 'border-gray-700 bg-gray-900/60' : 'border-gray-200 bg-gray-50'">
          <div class="flex items-center justify-between gap-3">
            <div>
              <p class="text-sm font-semibold" :class="isDark ? 'text-gray-200' : 'text-gray-800'">Source dates to change</p>
              <p class="mt-1 text-xs" :class="isDark ? 'text-gray-400' : 'text-gray-500'">Leave this list empty to normalize all records.</p>
            </div>
            <button v-if="normalizeSourceDates.length" type="button" @click="normalizeSourceDates = []" class="text-xs font-semibold text-red-600 hover:text-red-700">Clear all</button>
          </div>
          <div v-if="normalizeSourceDates.length" class="mt-3 flex flex-wrap gap-2">
            <span v-for="date in normalizeSourceDates" :key="date" class="inline-flex items-center gap-2 rounded-full bg-teal-100 px-3 py-1.5 text-sm font-medium text-teal-800">
              {{ formatNormalizeDate(date) }}
              <button type="button" @click="removeNormalizeSourceDate(date)" class="rounded-full text-teal-600 hover:text-teal-900" :aria-label="`Remove ${date}`">×</button>
            </span>
          </div>
          <p v-else class="mt-3 text-sm italic" :class="isDark ? 'text-gray-400' : 'text-gray-500'">Every device record will be considered.</p>
        </div>
        <p v-if="normalizeError" class="mt-4 rounded-lg bg-red-50 px-4 py-3 text-sm text-red-700">{{ normalizeError }}</p>
        <p v-if="normalizeSuccess" class="mt-4 rounded-lg bg-emerald-50 px-4 py-3 text-sm text-emerald-700">{{ normalizeSuccess }}</p>
      </div>
    </div>


    <!-- Read-only device/database comparison. This never changes device sync state. -->
    <div v-if="activeWorkspace === 'inspect' && $route.path !== '/sync/normalize-dates'" class="sync-panel p-6 mb-6">
      <div class="flex flex-wrap items-center justify-between gap-3 mb-4">
        <div>
          <h2 class="text-xl font-semibold text-gray-900">Device Reading Inspector</h2>
          <p class="text-sm text-gray-600">Compare every ESP32 SQLite reading with the website without syncing or changing records.</p>
        </div>
        <div class="flex flex-wrap gap-2">
          <input v-model.trim="deviceAccountQuery" @keyup.enter="runAccountInspection" class="w-44 px-3 py-2 border border-gray-300 rounded-lg" placeholder="Account, e.g. B-001" />
          <button @click="runAccountInspection" :disabled="!isConnected || deviceBusy || !deviceAccountQuery"
            class="bg-teal-600 hover:bg-teal-700 text-white px-4 py-2 rounded-lg font-medium disabled:opacity-50">
            {{ isInspecting ? 'Inspecting…' : 'Inspect Account' }}
          </button>
          <button @click="runReadingInspection" :disabled="!isConnected || deviceBusy"
            class="border border-teal-600 text-teal-700 hover:bg-teal-50 px-4 py-2 rounded-lg font-medium disabled:opacity-50">
            Inspect All
          </button>
        </div>
      </div>
      <p v-if="inspectionError" class="mb-3 text-sm text-red-700">{{ inspectionError }}</p>
      <template v-if="inspectionSummary">
        <div class="grid grid-cols-2 md:grid-cols-4 gap-3 mb-4 text-sm">
          <div class="rounded bg-green-50 p-3 text-green-800">Matches: {{ inspectionSummary.match || 0 }}</div>
          <div class="rounded bg-yellow-50 p-3 text-yellow-800">Missing on web: {{ inspectionSummary.missing_on_web || 0 }}</div>
          <div class="rounded bg-orange-50 p-3 text-orange-800">Duplicates: {{ inspectionSummary.duplicate || 0 }}</div>
          <div class="rounded bg-red-50 p-3 text-red-800">Conflicts: {{ inspectionSummary.conflict || 0 }}</div>
        </div>
        <input v-model.trim="inspectionSearch" class="w-full mb-3 px-3 py-2 border border-gray-300 rounded-lg" placeholder="Filter by account number, device reading ID, or status" />
        <div class="mb-3 flex flex-wrap items-end gap-3 rounded-xl border border-amber-200 bg-amber-50 p-3">
          <div class="text-sm text-amber-900"><p class="font-semibold">Repair selected device dates</p><p class="text-xs">Moves each selected reading and its linked device bill. It does not remove duplicates.</p></div>
          <label class="ml-auto text-xs font-medium text-amber-900">New date<input v-model="inspectionRepairDate" type="date" class="ml-2 rounded border border-amber-300 bg-white px-2 py-1 text-sm" /></label>
          <button @click="repairSelectedInspectionDates" :disabled="deviceBusy || !isConnected || !inspectionRepairDate || !selectedInspectionReadingIds.length" class="rounded-lg bg-amber-600 px-3 py-2 text-sm font-semibold text-white hover:bg-amber-700 disabled:cursor-not-allowed disabled:opacity-50">Set date for {{ selectedInspectionReadingIds.length || 'selected' }} reading(s)</button>
        </div>
        <div class="overflow-x-auto max-h-[32rem] border border-gray-200 rounded-xl">
          <table class="min-w-[820px] w-full text-sm">
            <thead class="sticky top-0 bg-gray-100 text-left text-gray-700"><tr><th class="px-3 py-2">Repair</th><th class="px-3 py-2">Status</th><th class="px-3 py-2">Account</th><th class="px-3 py-2">Device ID</th><th class="px-3 py-2">Device reading</th><th class="px-3 py-2">Reading date &amp; time</th><th class="px-3 py-2">Website reading</th></tr></thead>
            <tbody>
              <tr v-for="row in filteredInspectionResults" :key="row.device.device_uid + ':' + row.device.reading_id" class="border-t border-gray-100 align-top hover:bg-teal-50/40">
                <td class="px-3 py-2"><input v-model="selectedInspectionReadingIds" :value="String(row.device.reading_id)" type="checkbox" :aria-label="`Repair device reading ${row.device.reading_id}`" /></td>
                <td class="px-3 py-2"><span class="inline-flex rounded-full px-2 py-1 text-xs font-semibold capitalize whitespace-nowrap" :class="inspectionStatusBadgeClass(row.status)">{{ row.status.replaceAll('_', ' ') }}</span></td>
                <td class="px-3 py-2 font-semibold text-gray-900 whitespace-nowrap">{{ row.device.customer_account_number }}</td>
                <td class="px-3 py-2 text-gray-700 font-medium">#{{ row.device.reading_id }}</td>
                <td class="px-3 py-2"><div class="grid grid-cols-3 gap-x-3"><div><div class="text-[10px] uppercase text-gray-500">Prev</div><div class="font-semibold text-blue-700 whitespace-nowrap">{{ row.device.previous_reading }}</div></div><div><div class="text-[10px] uppercase text-gray-500">Current</div><div class="font-semibold text-green-700 whitespace-nowrap">{{ row.device.current_reading }}</div></div><div><div class="text-[10px] uppercase text-gray-500">Usage</div><div class="font-semibold text-teal-700 whitespace-nowrap">{{ row.device.usage_m3 }} m³</div></div></div></td>
                <td class="px-3 py-2 text-xs font-medium text-gray-700 whitespace-nowrap">{{ formatReadingDateTime(row.device.reading_at) }}</td>
                <td class="px-3 py-2"><template v-if="row.server"><div class="grid grid-cols-3 gap-x-3"><div><div class="text-[10px] uppercase text-gray-500">Prev</div><div class="font-semibold text-blue-700 whitespace-nowrap">{{ row.server.previous_reading }}</div></div><div><div class="text-[10px] uppercase text-gray-500">Current</div><div class="font-semibold text-green-700 whitespace-nowrap">{{ row.server.current_reading }}</div></div><div><div class="text-[10px] uppercase text-gray-500">Usage</div><div class="font-semibold text-teal-700 whitespace-nowrap">{{ row.server.usage_m3 }} m³</div></div></div></template><span v-else class="text-gray-400">No website record</span></td>
              </tr>
              <tr v-if="filteredInspectionResults.length === 0"><td colspan="7" class="p-4 text-center text-gray-500">No readings match this filter.</td></tr>
            </tbody>
          </table>
        </div>
      </template>
    </div>


      <section v-if="activeWorkspace === 'tools' && $route.path !== '/sync/normalize-dates'" class="sync-panel p-6">
        <h2 class="text-lg font-bold">Device maintenance</h2><p class="mt-1 text-sm sync-muted">Tools for troubleshooting and managing the connected device.</p><p v-if="transportMode === 'wifi'" class="mt-2 text-xs text-teal-600">Use USB for device restart and SD formatting.</p>
        <div class="mt-6 grid gap-4 md:grid-cols-3">
          <article class="sync-soft rounded-2xl p-5"><h3 class="font-bold">Reset sync status</h3><p class="mt-2 text-sm leading-6 sync-muted">Mark website records as unsynced so they can be sent again.</p><button @click="showResetConfirm = true" :disabled="deviceBusy" class="sync-secondary mt-5 disabled:opacity-50">Reset web sync flags</button></article>
          <article class="sync-soft rounded-2xl p-5"><h3 class="font-bold">Restart device</h3><p class="mt-2 text-sm leading-6 sync-muted">Restart the ESP32 and reconnect after it finishes booting.</p><button @click="showRestartConfirm = true" :disabled="!isConnected || deviceBusy || transportMode === 'wifi'" class="sync-secondary mt-5 disabled:opacity-50">Restart ESP32</button></article>
          <article class="rounded-2xl border border-red-500/25 bg-red-500/5 p-5"><h3 class="font-bold text-red-500">Format SD card</h3><p class="mt-2 text-sm leading-6 sync-muted">Permanently erase device records, customers, and settings from the card.</p><button @click="showFormatConfirm = true" :disabled="!isConnected || deviceBusy || transportMode === 'wifi'" class="mt-5 rounded-xl bg-red-600 px-4 py-2.5 text-sm font-semibold text-white hover:bg-red-700 disabled:opacity-50">Format SD card</button></article>
        </div>
        <details class="mt-6 border-t border-inherit pt-5"><summary class="cursor-pointer text-sm font-bold text-teal-600">Advanced serial command</summary><form class="mt-4 flex flex-wrap gap-3" @submit.prevent="!deviceBusy && sendCustomCommand(addLog)"><input v-model="customCommand" :disabled="deviceBusy || !isConnected" aria-label="Serial command" placeholder="Enter a device command…" class="min-w-0 flex-1"/><button :disabled="!isConnected || deviceBusy || !customCommand.trim()" class="sync-primary disabled:opacity-50">Send command</button></form></details>
      </section>
      <SyncActivityLog :logs="filteredSyncLogs" :busy="deviceBusy" @clear="clearSyncLogs" />
      <SuccessModal :is-open="showSuccessDialog" :is-dark="isDark" title="Sync completed" message="Device and website data transfers have completed. Review the activity log for details." @close="showSuccessDialog = false" />
    <!-- Format SD Card Confirmation -->
    <ConfirmDialog
      :is-dark="isDark"
      :is-open="showFormatConfirm"
      title="Format SD Card"
      :message="'This will permanently delete all data on the SD card including customers, readings, and settings. This action cannot be undone.<br><br>Are you sure you want to format the SD card?'"
      confirm-text="Format SD Card"
      cancel-text="Cancel"
      :is-loading="isFormatting"
      @confirm="formatSdCard(addLog, sendLineDevice)"
      @cancel="showFormatConfirm = false"
    />

    <!-- Restart Device Confirmation -->
    <ConfirmDialog
      :is-dark="isDark"
      :is-open="showRestartConfirm"
      title="Restart Device"
      :message="'This will restart the ESP32 device. Any unsaved data may be lost.<br><br>Are you sure you want to restart the device?'"
      confirm-text="Restart Device"
      cancel-text="Cancel"
      :is-loading="isRestarting"
      @confirm="restartDevice(addLog, sendLineDevice, disconnectDevice)"
      @cancel="showRestartConfirm = false"
    />

    <!-- Reset Sync Status Confirmation -->
    <ConfirmDialog
      :is-dark="isDark"
      :is-open="showResetConfirm"
      title="Reset Sync Status"
      :message="'This will reset the sync status for all records in the web database. All records will be marked as unsynced (Synced = 0, last_sync = null), allowing them to be synced again with the device.<br><br>Are you sure you want to reset the sync status?'"
      confirm-text="Reset Sync Status"
      cancel-text="Cancel"
      :is-loading="isResetting"
      @confirm="resetSyncStatus(addLog)"
      @cancel="showResetConfirm = false"
    />

    <!-- Command Success Modal -->
    <SuccessModal
      :is-dark="isDark"
      :is-open="showCommandSuccess"
      title="Command Executed"
      :message="commandSuccessMessage"
      button-text="OK"
      @close="showCommandSuccess = false"
    />

    </div>
  </AppSidebar>
</template>

<script>
import SyncConnectionCard from '../components/sync/SyncConnectionCard.vue'
import SyncDeviceInfo from '../components/sync/SyncDeviceInfo.vue'
import SyncActivityLog from '../components/sync/SyncActivityLog.vue'
import AppSidebar from '../components/AppSidebar.vue'
import ConfirmDialog from '../components/ConfirmDialog.vue'
import SuccessModal from '../components/SuccessModal.vue'
import { serialService } from '../services/serialService'
import { databaseService } from '../services/databaseService'
import { useSyncDevice } from '../composables/Sync/useSyncDevice'
import { useSyncCustomers } from '../composables/Sync/useSyncCustomers'
import { useSyncCustomerTypes } from '../composables/Sync/useSyncCustomerTypes'
import { useSyncDeductions } from '../composables/Sync/useSyncDeductions'
import { useSyncBarangays } from '../composables/Sync/useSyncBarangays'
import { useSyncSettings } from '../composables/Sync/useSyncSettings'
import { useSyncReadings } from '../composables/Sync/useSyncReadings'
import { useSyncBills } from '../composables/Sync/useSyncBills'
import { useSyncBillTransactions } from '../composables/Sync/useSyncBillTransactions'
import { useSyncData } from '../composables/Sync/useSyncData'
import { useSyncController } from '../composables/Sync/useSyncController'

export default {
  name: 'SyncDevice',
  components: {
    SyncConnectionCard,
    SyncDeviceInfo,
    SyncActivityLog,
    AppSidebar,
    ConfirmDialog,
    SuccessModal
  },
  setup() {
    const {
      deviceId,
      brgyId,
      firmwareVersion,
      lastSync,
      pendingRecords,
      printCount,
      customerCount,
      sdTotalBytes,
      sdUsedBytes,
      sdFreeBytes,
      cpuFreqMhz,
      heapFreeBytes,
      exportDeviceInfoFromDevice,
      refreshDeviceInfo,
      handleDeviceLine: handleDeviceLineDevice,
      sendLine: sendLineDevice
    } = useSyncDevice()

    const {
      pushCustomersToDevice,
      handleChunkAck
    } = useSyncCustomers()

    const {
      pushCustomerTypesToDevice
    } = useSyncCustomerTypes()

    const {
      pushDeductionsToDevice
    } = useSyncDeductions()

    const {
      pushBarangaysToDevice
    } = useSyncBarangays()

    const {
      pushSettingsToDevice,
      handleAck: handleAckSettings
    } = useSyncSettings()

    const {
      syncReadingsFromDevice,
      inspectReadingsFromDevice,
      inspectReadingsForAccount,
      handleDeviceLine: handleDeviceLineReadings
    } = useSyncReadings()

    const {
      syncBillsFromDevice,
      handleDeviceLine: handleDeviceLineBills
    } = useSyncBills()

    const {
      exportBillTransactionsFromDevice,
      handleExportLine: handleExportLineBillTransactions,
      handleAck: handleAckBillTransactions
    } = useSyncBillTransactions()

    const {
      isSyncing,
      syncStage,
      syncStatus,
      syncError,
      syncLogs,
      filteredSyncLogs,
      syncData,
      addLog
    } = useSyncData()

    const {
      customCommand,
      isFormatting,
      isResetting,
      isRestarting,
      showFormatConfirm,
      showRestartConfirm,
      showDropConfirm,
      showResetConfirm,
      showCommandSuccess,
      commandSuccessMessage,
      sendCommand,
      sendCustomCommand,
      formatSdCard,
      restartDevice,
      dropDatabase,
      resetSyncStatus
    } = useSyncController()

    return {
      deviceId,
      brgyId,
      firmwareVersion,
      lastSync,
      pendingRecords,
      printCount,
      customerCount,
      sdTotalBytes,
      sdUsedBytes,
      sdFreeBytes,
      cpuFreqMhz,
      heapFreeBytes,
      exportDeviceInfoFromDevice,
      refreshDeviceInfo,
      handleDeviceLineDevice,
      sendLineDevice,
      pushCustomersToDevice,
      handleChunkAck,
      pushCustomerTypesToDevice,
      pushDeductionsToDevice,
      pushBarangaysToDevice,
      pushSettingsToDevice,
      handleAckSettings,
      syncReadingsFromDevice,
      inspectReadingsFromDevice,
      inspectReadingsForAccount,
      handleDeviceLineReadings,
      syncBillsFromDevice,
      handleDeviceLineBills,
      exportBillTransactionsFromDevice,
      handleExportLineBillTransactions,
      handleAckBillTransactions,
      isSyncing,
      syncStage,
      syncStatus,
      syncError,
      syncLogs,
      filteredSyncLogs,
      syncData,
      addLog,
      customCommand,
      isFormatting,
      isResetting,
      isRestarting,
      showFormatConfirm,
      showRestartConfirm,
      showCommandSuccess,
      commandSuccessMessage,
      sendCommand,
      sendCustomCommand,
      formatSdCard,
      restartDevice,
      dropDatabase,
      resetSyncStatus,
      showDropConfirm,
      showResetConfirm
    }
  },
  data() {
    return {
      isConnected: false,
      connectedPort: '',
      isConnecting: false,
      connectionError: '',
      navigationNotice: '',
      serialSupported: 'serial' in navigator || Boolean(globalThis.__TAURI_INTERNALS__),
      transportMode: localStorage.getItem('dmbc-sync-transport') === 'wifi' ? 'wifi' : 'usb',
      wifiPairingCode: '',

      showSuccessDialog: false,
      isInspecting: false,
      inspectionError: '',
      inspectionSummary: null,
      inspectionResults: [],
      inspectionSearch: '',
      deviceAccountQuery: '',
      selectedInspectionReadingIds: [],
      inspectionRepairDate: '',
      normalizeTargetDate: '',
      normalizeSourceDateDraft: '',
      normalizeSourceDates: [],
      isNormalizing: false,
      normalizeError: '',
      normalizeSuccess: '',

      // Serial connection lives in serialService (persists across tabs/pages)
      unlistenLine: null,
      unlistenStatus: null
    }
  },
  mounted() {
    serialService.installSerialDisconnectListener()

    this.unlistenLine = serialService.onLine((line) => {
      // Route device lines through the same handler used by the exporter.
      this.handleDeviceLine(line)
      if (this.shouldDisplayDeviceLine(line)) {
        this.addLog('Device ← ' + line)
      }
    })

    this.unlistenStatus = serialService.onStatus((st) => {
      this.isConnected = !!st.isConnected
      this.connectedPort = st.connectedPortLabel || ''
      if (st.isConnected) this.transportMode = st.transport || 'usb'
    })

    // Try to reuse a previously-granted port without prompting again.
    void serialService.autoReconnect({ baudRate: 115200 }).catch(() => {})
  },
  beforeRouteLeave() {
    if (this.deviceBusy) {
      this.navigationNotice = 'Wait for the current device operation to finish before leaving this page.'
      return false
    }
  },
  beforeUnmount() {
    // Important: DO NOT disconnect on unmount; this keeps USB serial persistent across pages.
    if (this.unlistenLine) this.unlistenLine()
    if (this.unlistenStatus) this.unlistenStatus()
  },
  methods: {
    addNormalizeSourceDate() {
      const date = this.normalizeSourceDateDraft
      if (!date || this.normalizeSourceDates.includes(date)) return
      this.normalizeSourceDates = [...this.normalizeSourceDates, date].sort()
      this.normalizeSourceDateDraft = ''
    },
    removeNormalizeSourceDate(date) {
      this.normalizeSourceDates = this.normalizeSourceDates.filter(item => item !== date)
    },
    formatNormalizeDate(value) {
      const date = new Date(`${value}T00:00:00`)
      return Number.isNaN(date.getTime()) ? value : date.toLocaleDateString('en-PH', { year: 'numeric', month: 'short', day: 'numeric' })
    },
    async normalizeDates() {
      if (this.deviceBusy) return
      if (!this.isConnected) {
        this.normalizeError = 'Connect an ESP32 device first.'
        return
      }
      if (!this.normalizeTargetDate) {
        this.normalizeError = 'Choose a target date.'
        return
      }

      this.isNormalizing = true
      this.normalizeError = ''
      this.normalizeSuccess = ''
      try {
        const sourceDates = this.normalizeSourceDates.join(',')
        const command = `NORMALIZE_DATES ${this.normalizeTargetDate}${sourceDates ? `|${sourceDates}` : ''}`
        this.addLog(`Applying date normalization to ${this.normalizeSourceDates.length ? this.normalizeSourceDates.length + ' selected date(s)' : 'all dates'}...`)
        await this.sendLineDevice(command)
        this.addLog('Date normalization command sent to device')
        this.normalizeSuccess = this.normalizeSourceDates.length
          ? `${this.normalizeSourceDates.length} source date(s) will be changed to ${this.formatNormalizeDate(this.normalizeTargetDate)}. Check the device log for the result.`
          : `All dates will be changed to ${this.formatNormalizeDate(this.normalizeTargetDate)}. Check the device log for the result.`
      } catch (error) {
        this.normalizeError = error?.message || 'Failed to send date normalization command.'
        this.addLog('Date normalization failed: ' + this.normalizeError)
      } finally {
        this.isNormalizing = false
      }
    },
    async runAccountInspection() {
      if (this.deviceBusy) return
      if (!this.isConnected || this.isInspecting) return
      this.isInspecting = true
      this.inspectionError = ''
      try {
        const account = this.deviceAccountQuery.trim().toUpperCase()
        this.addLog('Querying device readings for ' + account + '...')
        const report = await this.inspectReadingsForAccount(account)
        this.inspectionSummary = report.summary || {}
        this.inspectionResults = report.results || []
        this.inspectionSearch = ''
        this.selectedInspectionReadingIds = []
        this.addLog(`Inspection complete: ${this.inspectionResults.length} record(s) found for ${account}`)
      } catch (error) {
        this.inspectionError = error?.response?.data?.message || error?.message || 'Inspection failed'
        this.addLog('Inspection failed: ' + this.inspectionError)
      } finally {
        this.isInspecting = false
      }
    },
    async repairSelectedInspectionDates() {
      if (this.deviceBusy) return
      const ids = [...new Set(this.selectedInspectionReadingIds.map(Number).filter(id => Number.isInteger(id) && id > 0))]
      if (!this.inspectionRepairDate || ids.length === 0 || !this.isConnected) return
      if (!window.confirm(`Set ${ids.length} selected device reading(s), plus their linked device bill(s), to ${this.formatNormalizeDate(this.inspectionRepairDate)}? This changes the ESP32 database.`)) return
      try {
        this.addLog(`Repairing ${ids.length} selected device reading date(s) to ${this.inspectionRepairDate}...`)
        await this.sendLineDevice(`NORMALIZE_READING_IDS ${this.inspectionRepairDate}|${ids.join(',')}`)
        this.addLog('Selected date repair command sent to device. Inspect the account again after the device confirms completion.')
      } catch (error) {
        this.addLog('Selected date repair failed: ' + (error?.message || 'Unknown error'))
      }
    },
    async runReadingInspection() {
      if (this.deviceBusy) return
      if (!this.isConnected || this.isInspecting) return
      this.isInspecting = true
      this.inspectionError = ''
      try {
        this.addLog('Starting read-only device reading inspection...')
        const report = await this.inspectReadingsFromDevice()
        this.inspectionSummary = report.summary || {}
        this.inspectionResults = report.results || []
        this.selectedInspectionReadingIds = []
        this.addLog(`Inspection complete: ${this.inspectionResults.length} device readings checked`)
      } catch (error) {
        this.inspectionError = error?.response?.data?.message || error?.message || 'Inspection failed'
        this.addLog('Inspection failed: ' + this.inspectionError)
      } finally {
        this.isInspecting = false
      }
    },
    inspectionStatusClass(status) {
      return { match: 'text-green-700', missing_on_web: 'text-yellow-700', duplicate: 'text-orange-700', conflict: 'text-red-700' }[status] || 'text-gray-700'
    },
    inspectionStatusBadgeClass(status) {
      return { match: 'bg-green-100 text-green-800', missing_on_web: 'bg-yellow-100 text-yellow-800', duplicate: 'bg-orange-100 text-orange-800', conflict: 'bg-red-100 text-red-800' }[status] || 'bg-gray-100 text-gray-700'
    },
    formatReadingDateTime(value) {
      if (value === null || value === undefined || value === '') return '—'
      const numeric = Number(value)
      const date = Number.isFinite(numeric) && numeric > 0 ? new Date(numeric * 1000) : new Date(value)
      if (Number.isNaN(date.getTime())) return '—'
      return date.toLocaleString('en-PH', { dateStyle: 'medium', timeStyle: 'medium' })
    },
    shouldDisplayDeviceLine(line) {
      const t = String(line || '')
      if (!t) return false

      // Hide protocol markers and payload lines to keep the log readable.
      if (
        t === 'BEGIN_READINGS' ||
        t === 'END_READINGS' ||
        t === 'BEGIN_DEVICE_INFO' ||
        t === 'END_DEVICE_INFO' ||
        t === 'BEGIN_CUSTOMER_TYPES' ||
        t === 'END_CUSTOMER_TYPES'
      ) {
        return false
      }

      if (t.startsWith('ACK|UPSERT_CUSTOMERS_JSON|')) {
        return true; // Show customer sync success
      }

      if (t.startsWith('ACK|CHUNK|')) {
        return true; // Show chunk progress
      }

      if (t.startsWith('READINGS_CHUNK|') || t.startsWith('BILLS_CHUNK|')) {
        return false // Hide verbose JSON chunks
      }

      if (t.startsWith('BILL_TRANSACTIONS_CHUNK|')) {
        return false // Hide verbose JSON chunks
      }

      // Sometimes the serial layer may emit fragments of long JSON lines.
      // Hide those too to keep the log clean.
      if (
        t.includes('"bill_transaction_id"') ||
        t.includes('"bill_reference_number"') ||
        t.includes('"processed_by_device_uid"') ||
        t.includes('"transaction_date"')
      ) {
        return false
      }

      // Everything else (Exporting..., Done., Totals, ACK/ERR) is useful.
      return true
    },
    async connectDevice() {
      if (this.isConnecting || this.deviceBusy) return
      this.isConnecting = true
      this.connectionError = ''
      try {
        if (this.transportMode === 'wifi') await serialService.connectWifi(this.wifiPairingCode.trim())
        else await serialService.connect({ baudRate: 115200 })
        this.addLog('Device connected successfully')
        this.addLog('Receiving ESP32 sync protocol responses over ' + this.transportMode.toUpperCase() + '...')

        await this.refreshDeviceInfo()
      } catch (error) {
        this.addLog('Connection failed: ' + error.message)
        this.connectionError = error.message || 'Unable to connect the device.'
        console.error('Serial connection error:', error)
      } finally {
        this.isConnecting = false
      }
    },

    async disconnectDevice() {
      if (this.deviceBusy) return
      await serialService.disconnect()
      this.addLog('Device disconnected')
    },

    async handleSyncData() {
      if (this.deviceBusy) return
      await this.syncData({
        isConnected: this.isConnected,
        exportDeviceInfoFromDevice: this.exportDeviceInfoFromDevice,
        pushCustomerTypesToDevice: this.pushCustomerTypesToDevice,
        pushDeductionsToDevice: this.pushDeductionsToDevice,
        pushBarangaysToDevice: this.pushBarangaysToDevice,
        pushSettingsToDevice: this.pushSettingsToDevice,
        syncReadingsFromDevice: this.syncReadingsFromDevice,
        syncBillsFromDevice: this.syncBillsFromDevice,
        exportBillTransactionsFromDevice: this.exportBillTransactionsFromDevice,
        pushCustomersToDevice: this.pushCustomersToDevice,
        refreshDeviceInfo: this.refreshDeviceInfo,
        sendLineDevice: this.sendLineDevice,
        onSyncSuccess: () => {
          this.showSuccessDialog = true
        }
      })
    },

    formatBytes(value) {
      const n = Number(value || 0)
      if (!Number.isFinite(n) || n <= 0) return '-'
      const kb = 1024
      const mb = kb * 1024
      const gb = mb * 1024
      if (n >= gb) return (n / gb).toFixed(2) + ' GB'
      if (n >= mb) return (n / mb).toFixed(2) + ' MB'
      if (n >= kb) return (n / kb).toFixed(2) + ' KB'
      return String(Math.floor(n)) + ' B'
    },

    handleDeviceLine(line) {
      if (!line) return

      // Track acks (useful for debugging)
      if (line.startsWith('ACK|')) {
        // Handle chunk ACKs
        this.handleChunkAck(line)
        this.handleAckSettings(line)
      }

      // Handle device info and readings exports
      this.handleDeviceLineDevice(line)
      this.handleDeviceLineReadings(line)
      this.handleDeviceLineBills(line)
      this.handleExportLineBillTransactions(line)
      this.handleAckBillTransactions(line)

      if (line.startsWith('ERR|')) {
        // Error handling is done in the individual handlers
      }
    },

    clearSyncLogs() {
      this.syncLogs.length = 0
    },

    // Disconnect handling is handled by serialService now.


  }
  ,computed: {
    activeWorkspace() {
      const view = this.$route.query.view
      return ['sync', 'inspect', 'tools'].includes(view) ? view : 'sync'
    },
    deviceBusy() {
      return this.isConnecting || this.isSyncing || this.isInspecting || this.isNormalizing || this.isFormatting || this.isRestarting || this.isResetting
    },
    deviceDetails() {
      return [
        { label: 'Device ID / Barangay', value: (this.deviceId ?? '—') + ' / ' + (this.brgyId ?? '—') },
        { label: 'Firmware', value: this.firmwareVersion },
        { label: 'Last device sync', value: this.lastSync },
        { label: 'Pending readings', value: this.pendingRecords },
        { label: 'Member accounts', value: this.customerCount },
        { label: 'Receipts printed', value: this.printCount },
        { label: 'Storage used / total', value: this.formatBytes(this.sdUsedBytes) + ' / ' + this.formatBytes(this.sdTotalBytes) },
        { label: 'Storage free', value: this.formatBytes(this.sdFreeBytes) },
        { label: 'CPU', value: this.cpuFreqMhz > 0 ? this.cpuFreqMhz + ' MHz' : '—' },
        { label: 'Free memory', value: this.formatBytes(this.heapFreeBytes) }
      ]
    },
    filteredInspectionResults() {
      const q = this.inspectionSearch.toLowerCase()
      if (!q) return this.inspectionResults
      return this.inspectionResults.filter(row => [row.status, row.device.customer_account_number, row.device.reading_id].join(' ').toLowerCase().includes(q))
    }
  }
}
</script>
