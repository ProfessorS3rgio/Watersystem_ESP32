<template>
  <AppSidebar v-slot="{ isDark }">
    <div class="p-8">
      <div class="mb-8">
        <h1 class="text-3xl font-bold mb-2 transition-colors duration-200" :class="isDark ? 'text-white' : 'text-gray-900'">Sync Device</h1>
        <p class="transition-colors duration-200" :class="isDark ? 'text-gray-400' : 'text-gray-600'">Connect and synchronize data with your ESP32 water system device</p>
    </div>

    <!-- Connection Status -->
    <div class="grid grid-cols-1 lg:grid-cols-2 gap-6 mb-8">
      <div class="bg-white rounded-lg shadow-lg p-6">
        <h2 class="text-xl font-semibold text-gray-900 mb-4">Web Serial Connection</h2>
        <div class="space-y-4">
          <div v-if="!isConnected" class="text-center py-8">
            <svg class="h-16 w-16 text-gray-400 mx-auto mb-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M8 9l3 3-3 3m5 0h3M5 20h14a2 2 0 002-2V6a2 2 0 00-2-2H5a2 2 0 00-2 2v12a2 2 0 002 2z" />
            </svg>
            <p class="text-gray-600 mb-4">No device connected</p>
            <button
              @click="connectDevice"
              class="bg-indigo-600 hover:bg-indigo-700 text-white px-6 py-3 rounded-lg font-medium shadow-lg transition-all duration-200 inline-flex items-center space-x-2"
            >
              <svg class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M13 10V3L4 14h7v7l9-11h-7z" />
              </svg>
              <span>Connect Device</span>
            </button>
          </div>

          <div v-else class="space-y-4">
            <div class="bg-green-50 border border-green-200 rounded-lg p-4">
              <div class="flex items-center">
                <svg class="h-6 w-6 text-green-600 mr-3" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                  <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 12l2 2 4-4m6 2a9 9 0 11-18 0 9 9 0 0118 0z" />
                </svg>
                <div class="flex-1">
                  <p class="text-sm font-medium text-green-800">Connected</p>
                  <p class="text-xs text-green-600">{{ connectedPort }}</p>
                </div>
                <button
                  @click="disconnectDevice"
                  class="text-red-600 hover:text-red-800 text-sm font-medium"
                >
                  Disconnect
                </button>
              </div>
            </div>

            <div class="grid grid-cols-2 gap-4">
              <button
                @click="syncData"
                :disabled="isSyncing"
                class="bg-blue-600 hover:bg-blue-700 text-white px-4 py-3 rounded-lg font-medium shadow transition-all duration-200 disabled:opacity-50 flex items-center justify-center space-x-2"
              >
                <svg class="h-5 w-5" :class="{ 'animate-spin': isSyncing }" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                  <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15" />
                </svg>
                <span>{{ isSyncing ? 'Syncing...' : 'Sync Data' }}</span>
              </button>

              <button
                @click="sendCommand"
                class="bg-purple-600 hover:bg-purple-700 text-white px-4 py-3 rounded-lg font-medium shadow transition-all duration-200 flex items-center justify-center space-x-2"
              >
                <svg class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                  <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M8 12h.01M12 12h.01M16 12h.01M21 12c0 4.418-4.03 8-9 8a9.863 9.863 0 01-4.255-.949L3 20l1.395-3.72C3.512 15.042 3 13.574 3 12c0-4.418 4.03-8 9-8s9 3.582 9 8z" />
                </svg>
                <span>Send Command</span>
              </button>

              <button
                @click="showFormatConfirm = true"
                class="bg-red-600 hover:bg-red-700 text-white px-4 py-3 rounded-lg font-medium shadow transition-all duration-200 flex items-center justify-center space-x-2"
              >
                <svg class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                  <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M19 7l-.867 12.142A2 2 0 0116.138 21H7.862a2 2 0 01-1.995-1.858L5 7m5 4v6m4-6v6m1-10V4a1 1 0 00-1-1h-4a1 1 0 00-1 1v3M4 7h16" />
                </svg>
                <span>Format SD Card</span>
              </button>

              <button
                @click="showRestartConfirm = true"
                class="bg-orange-600 hover:bg-orange-700 text-white px-4 py-3 rounded-lg font-medium shadow transition-all duration-200 flex items-center justify-center space-x-2"
              >
                <svg class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                  <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15M4.75 4.75l.75.75m0 0l-.75.75M4.75 19.25l.75-.75m0 0l-.75-.75" />
                </svg>
                <span>Restart Device</span>
              </button>
            </div>
          </div>
        </div>
      </div>

      <!-- Device Info -->
      <div class="bg-white rounded-lg shadow-lg p-6">
        <h2 class="text-xl font-semibold text-gray-900 mb-4">Device Information</h2>
        <div class="space-y-3">
          <div class="flex items-center justify-between py-2 border-b border-gray-200">
            <span class="text-sm text-gray-600">Device Type</span>
            <span class="text-sm font-medium text-gray-900">ESP32 Water System</span>
          </div>
          <div class="flex items-center justify-between py-2 border-b border-gray-200">
            <span class="text-sm text-gray-600">Device ID</span>
            <span class="text-sm font-medium text-gray-900">{{ deviceId || '-' }}</span>
          </div>
          <div class="flex items-center justify-between py-2 border-b border-gray-200">
            <span class="text-sm text-gray-600">Barangay ID</span>
            <span class="text-sm font-medium text-gray-900">{{ brgyId || '-' }}</span>
          </div>
          <div class="flex items-center justify-between py-2 border-b border-gray-200">
            <span class="text-sm text-gray-600">Firmware Version</span>
            <span class="text-sm font-medium text-gray-900">{{ firmwareVersion }}</span>
          </div>
          <div class="flex items-center justify-between py-2 border-b border-gray-200">
            <span class="text-sm text-gray-600">Last Sync</span>
            <span class="text-sm font-medium text-gray-900">{{ lastSync }}</span>
          </div>
          <div class="flex items-center justify-between py-2 border-b border-gray-200">
            <span class="text-sm text-gray-600">Print counts</span>
            <span class="text-sm font-medium text-gray-900">{{ printCount }}</span>
          </div>
          <div class="flex items-center justify-between py-2 border-b border-gray-200">
            <span class="text-sm text-gray-600">Records Pending</span>
            <span class="text-sm font-medium text-orange-600">{{ pendingRecords }}</span>
          </div>
          <div class="flex items-center justify-between py-2 border-b border-gray-200">
            <span class="text-sm text-gray-600">SD Used</span>
            <span class="text-sm font-medium text-gray-900">{{ formatBytes(sdUsedBytes) }}</span>
          </div>
          <div class="flex items-center justify-between py-2 border-b border-gray-200">
            <span class="text-sm text-gray-600">SD Free</span>
            <span class="text-sm font-medium text-gray-900">{{ formatBytes(sdFreeBytes) }}</span>
          </div>
          <div class="flex items-center justify-between py-2 border-b border-gray-200">
            <span class="text-sm text-gray-600">SD Total</span>
            <span class="text-sm font-medium text-gray-900">{{ formatBytes(sdTotalBytes) }}</span>
          </div>
          <div class="flex items-center justify-between py-2 border-b border-gray-200">
            <span class="text-sm text-gray-600">CPU</span>
            <span class="text-sm font-medium text-gray-900">{{ cpuFreqMhz > 0 ? (cpuFreqMhz + ' MHz') : '-' }}</span>
          </div>
          <div class="flex items-center justify-between py-2 border-b border-gray-200">
            <span class="text-sm text-gray-600">Heap Free</span>
            <span class="text-sm font-medium text-gray-900">{{ formatBytes(heapFreeBytes) }}</span>
          </div>
          <div class="flex items-center justify-between py-2">
            <span class="text-sm text-gray-600">Battery Level</span>
            <div class="flex items-center space-x-2">
              <div class="w-20 h-2 bg-gray-200 rounded-full overflow-hidden">
                <div class="h-full bg-green-500" :style="{ width: batteryLevel + '%' }"></div>
              </div>
              <span class="text-sm font-medium text-gray-900">{{ batteryLevel }}%</span>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- Sync Log -->
    <div class="bg-white rounded-lg shadow-lg p-6">
      <h2 class="text-xl font-semibold text-gray-900 mb-4">Sync Log</h2>
      <div class="bg-gray-900 text-green-400 font-mono text-sm p-4 rounded-lg h-64 overflow-y-auto">
        <div v-for="(log, index) in syncLogs" :key="index" class="mb-1">
          <span class="text-gray-500">[{{ log.time }}]</span> {{ log.message }}
        </div>
        <div v-if="syncLogs.length === 0" class="text-gray-600">
          Waiting for connection...
        </div>
      </div>
    </div>

    <!-- Success Dialog -->
    <div v-if="showSuccessDialog" class="fixed inset-0 flex items-center justify-center z-50">
      <div class="bg-white rounded-lg p-6 max-w-md w-full mx-4 shadow-lg">
        <div class="flex items-center mb-4">
          <svg class="h-6 w-6 text-green-600 mr-3" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 12l2 2 4-4m6 2a9 9 0 11-18 0 9 9 0 0118 0z" />
          </svg>
          <h3 class="text-lg font-semibold text-gray-900">Sync Completed Successfully</h3>
        </div>
        <p class="text-gray-600 mb-6">All data has been synchronized between the web app and your ESP32 device.</p>
        <button
          @click="showSuccessDialog = false"
          class="w-full bg-blue-600 hover:bg-blue-700 text-white px-4 py-2 rounded-lg font-medium"
        >
          OK
        </button>
      </div>
    </div>

    <!-- Format SD Card Confirmation -->
    <ConfirmDialog
      :is-open="showFormatConfirm"
      title="Format SD Card"
      :message="'This will permanently delete all data on the SD card including customers, readings, and settings. This action cannot be undone.<br><br>Are you sure you want to format the SD card?'"
      confirm-text="Format SD Card"
      cancel-text="Cancel"
      :is-loading="isFormatting"
      @confirm="formatSdCard"
      @cancel="showFormatConfirm = false"
    />

    <!-- Restart Device Confirmation -->
    <ConfirmDialog
      :is-open="showRestartConfirm"
      title="Restart Device"
      :message="'This will restart the ESP32 device. Any unsaved data may be lost.<br><br>Are you sure you want to restart the device?'"
      confirm-text="Restart Device"
      cancel-text="Cancel"
      :is-loading="isRestarting"
      @confirm="restartDevice"
      @cancel="showRestartConfirm = false"
    />

    <!-- Command Success Modal -->
    <SuccessModal
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
import AppSidebar from '../components/AppSidebar.vue'
import ConfirmDialog from '../components/ConfirmDialog.vue'
import SuccessModal from '../components/SuccessModal.vue'
import { serialService } from '../services/serialService'
import { databaseService } from '../services/databaseService'

export default {
  name: 'SyncDevice',
  components: {
    AppSidebar,
    ConfirmDialog,
    SuccessModal
  },
  data() {
    return {
      isConnected: false,
      isSyncing: false,
      connectedPort: '',
      deviceId: null,
      brgyId: null,
      firmwareVersion: '-',
      lastSync: 'Never',
      pendingRecords: 0,
      printCount: 0,
      sdTotalBytes: 0,
      sdUsedBytes: 0,
      sdFreeBytes: 0,
      cpuFreqMhz: 0,
      heapFreeBytes: 0,
      batteryLevel: 85,
      syncLogs: [],

      showSuccessDialog: false,

      // Command dialogs
      showFormatConfirm: false,
      showRestartConfirm: false,
      showCommandSuccess: false,
      commandSuccessMessage: '',
      isFormatting: false,
      isRestarting: false,

      // Serial connection lives in serialService (persists across tabs/pages)
      unlistenLine: null,
      unlistenStatus: null,

      exportInProgress: false,
      exportLines: [],
      exportResolve: null,
      exportReject: null,
      exportTimeoutId: null,
      exportBeginMarker: null,
      exportEndMarker: null,
      exportLinePrefix: null,
      lastAck: null
    }
  },
  mounted() {
    serialService.installSerialDisconnectListener()

    this.unlistenLine = serialService.onLine((line) => {
      // Route device lines through the same handler used by the exporter.
      this.handleDeviceLine(line)
      if (this.shouldDisplayDeviceLine(line)) {
        this.addLog('< ' + line)
      }
    })

    this.unlistenStatus = serialService.onStatus((st) => {
      this.isConnected = !!st.isConnected
      this.connectedPort = st.connectedPortLabel || ''
    })

    // Try to reuse a previously-granted port without prompting again.
    void serialService.autoReconnect({ baudRate: 115200 }).catch(() => {})
  },
  beforeUnmount() {
    // Important: DO NOT disconnect on unmount; this keeps USB serial persistent across pages.
    if (this.unlistenLine) this.unlistenLine()
    if (this.unlistenStatus) this.unlistenStatus()
  },
  methods: {
    shouldDisplayDeviceLine(line) {
      const t = String(line || '')
      if (!t) return false

      // Hide protocol markers and payload lines to keep the log readable.
      if (
        t === 'BEGIN_CUSTOMERS' ||
        t === 'END_CUSTOMERS' ||
        t === 'BEGIN_READINGS' ||
        t === 'END_READINGS' ||
        t === 'BEGIN_DEVICE_INFO' ||
        t === 'END_DEVICE_INFO' ||
        t === 'BEGIN_CUSTOMER_TYPES' ||
        t === 'END_CUSTOMER_TYPES'
      ) {
        return false
      }

      if (t.startsWith('CUST|') || t.startsWith('READ|') || t.startsWith('INFO|') || t.startsWith('TYPE|')) {
        return false
      }

      // Everything else (Exporting..., Done., Totals, ACK/ERR) is useful.
      return true
    },
    async connectDevice() {
      try {
        await serialService.connect({ baudRate: 115200 })
        this.addLog('Device connected successfully')
        this.addLog('Listening for ESP32 serial output...')

        await this.refreshDeviceInfo()
      } catch (error) {
        this.addLog('Connection failed: ' + error.message)
        console.error('Serial connection error:', error)
      }
    },

    async disconnectDevice() {
      await serialService.disconnect()
      this.addLog('Device disconnected')
    },

    async syncData() {
      var st = serialService.getState()
      if (!this.isConnected || !st.writer) {
        this.addLog('Not connected. Connect a device first.')
        return
      }

      this.isSyncing = true
      try {
        this.addLog('Starting customer sync...')

        // Get device info first to know which barangay this device belongs to
        this.addLog('Getting device information...')
        var deviceInfo = await this.exportDeviceInfoFromDevice()
        var deviceBrgyId = deviceInfo?.brgy_id || 1
        var deviceId = deviceInfo?.device_id || 1
        this.addLog(`Device ID: ${deviceId}, Barangay ID: ${deviceBrgyId}`)

        // 1) Export customers from device for comparison
        this.addLog('Fetching customers from ESP32 (SD card)...')
        var deviceCustomers = await this.exportCustomersFromDevice()
        this.addLog(`Device has ${deviceCustomers.length} customers`)

        // 2) Get customers from web database
        this.addLog('Fetching customers from database...')
        var dbCustomers = await databaseService.fetchCustomersFromDatabase()
        this.addLog(`Database has ${dbCustomers.length} customers`)

        // 3) Filter database customers by device's barangay
        var filteredDbCustomers = dbCustomers.filter(c => c.brgy_id == deviceBrgyId)
        this.addLog(`Filtered to ${filteredDbCustomers.length} customers for barangay ${deviceBrgyId}`)

        // 4) Find customers to remove (on device but not in filtered web DB)
        var filteredDbAccountNos = new Set(filteredDbCustomers.map(c => c.account_no))
        var customersToRemove = deviceCustomers.filter(c => !filteredDbAccountNos.has(c.account_no))
        
        if (customersToRemove.length > 0) {
          this.addLog(`Removing ${customersToRemove.length} customers from device that no longer exist in database...`)
          for (var customer of customersToRemove) {
            await this.sendLine('REMOVE_CUSTOMER|' + this.escapeDeviceField(customer.account_no))
            await new Promise(r => setTimeout(r, 50))
          }
        }

        // 5) Sync customer types from DB to device first (needed for customer references)
        this.addLog('Fetching customer types from database...')
        var dbCustomerTypes = await databaseService.fetchCustomerTypesFromDatabase()
        this.addLog(`Database has ${dbCustomerTypes.length} customer types`)
        this.addLog(`Pushing ${dbCustomerTypes.length} customer types to ESP32...`)
        await this.pushCustomerTypesToDevice(dbCustomerTypes)
        this.addLog('✓ Customer types sync completed')

        // 5.5) Sync deductions from DB to device
        this.addLog('Fetching deductions from database...')
        var dbDeductions = await databaseService.fetchDeductionsFromDatabase()
        this.addLog(`Database has ${dbDeductions.length} deductions`)
        this.addLog(`Pushing ${dbDeductions.length} deductions to ESP32...`)
        await this.pushDeductionsToDevice(dbDeductions)
        this.addLog('✓ Deductions sync completed')

        // 5.6) Push filtered DB customers to device (adds new, updates existing)
        this.addLog(`Pushing ${filteredDbCustomers.length} customers to ESP32...`)
        await this.pushCustomersToDevice(filteredDbCustomers)

        this.addLog('✓ Customer sync completed')

        // 6) Sync readings (device -> DB)
        await this.syncReadingsFromDevice()

        // Always update last sync on the device at the end of a successful run.
        // (If there were no new readings, READINGS_SYNCED isn't sent, so device last_sync_epoch would stay 0.)
        var epochNow = Math.floor(Date.now() / 1000)
        await this.sendLine('SET_LAST_SYNC|' + String(epochNow))

        await this.refreshDeviceInfo()

        // Reload SD card to apply new settings
        this.addLog('Reloading SD card on ESP32...')
        await this.sendLine('RELOAD_SD')
        // Wait a moment for reload
        await new Promise(r => setTimeout(r, 1000))
        await this.refreshDeviceInfo()

        // Show success dialog
        this.showSuccessDialog = true
      } catch (error) {
        this.addLog('Sync failed: ' + (error?.message || String(error)))
      } finally {
        this.isSyncing = false
      }
    },

    async refreshDeviceInfo() {
      try {
        var info = await this.exportDeviceInfoFromDevice()
        if (typeof info?.device_id === 'number') this.deviceId = info.device_id
        if (typeof info?.brgy_id === 'number') this.brgyId = info.brgy_id
        if (info?.firmware_version) this.firmwareVersion = info.firmware_version
        if (typeof info?.pending_readings === 'number') this.pendingRecords = info.pending_readings

        var last = Number(info?.last_sync_epoch || 0)
        this.lastSync = last > 0 ? new Date(last * 1000).toLocaleString() : 'Never'

        if (typeof info?.print_count === 'number') this.printCount = info.print_count

        if (typeof info?.sd_total_bytes === 'number') this.sdTotalBytes = info.sd_total_bytes
        if (typeof info?.sd_used_bytes === 'number') this.sdUsedBytes = info.sd_used_bytes
        if (typeof info?.sd_free_bytes === 'number') this.sdFreeBytes = info.sd_free_bytes

        if (typeof info?.cpu_freq_mhz === 'number') this.cpuFreqMhz = info.cpu_freq_mhz
        if (typeof info?.heap_free_bytes === 'number') this.heapFreeBytes = info.heap_free_bytes
      } catch (e) {
        this.addLog('Device info unavailable: ' + (e?.message || String(e)))
      }
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

    async syncReadingsFromDevice() {
      // Ensure device has a usable clock (epoch seconds)
      const epochNow = Math.floor(Date.now() / 1000)
      this.addLog('Setting device time...')
      await this.sendLine('SET_TIME|' + String(epochNow))

      this.addLog('Fetching unsynced readings from ESP32...')
      const deviceReadings = await this.exportReadingsFromDevice()
      this.addLog(`Device exported ${deviceReadings.length} readings`)

      if (deviceReadings.length === 0) {
        this.addLog('No new readings to sync')
        return
      }

      this.addLog('Saving readings into database...')
      const processed = await databaseService.upsertReadingsToDatabase(deviceReadings)
      this.addLog(`Database processed ${processed} readings`)

      // Mark device readings as synced (so next export is incremental)
      await this.sendLine('READINGS_SYNCED')
      this.addLog('✓ Readings sync completed')
    },

    async sendCommand() {
      var st = serialService.getState()
      if (!this.isConnected || !st.writer) {
        this.addLog('Not connected. Connect a device first.')
        return
      }

      // Minimal test write. Your ESP32 firmware must decide what to do with this.
      var command = 'EXPORT_CUSTOMERS'
      try {
        await this.sendLine(command)
      } catch (error) {
        this.addLog('Write failed: ' + error.message)
      }
    },

    async sendLine(line) {
      const text = String(line).replace(/\r|\n/g, '')
      this.addLog('> ' + text)
      await serialService.sendLine(text)
    },

    escapeDeviceField(value) {
      return String(value ?? '')
        .replace(/\r|\n/g, ' ')
        .replace(/\|/g, ' ')
        .trim()
    },

    async exportCustomersFromDevice() {
      if (this.exportInProgress) {
        throw new Error('Export already in progress')
      }

      this.exportInProgress = true
      this.exportLines = []
      this.lastAck = null
      this.exportBeginMarker = 'BEGIN_CUSTOMERS'
      this.exportEndMarker = 'END_CUSTOMERS'
      this.exportLinePrefix = 'CUST|'

      const promise = new Promise((resolve, reject) => {
        this.exportResolve = resolve
        this.exportReject = reject
        this.exportTimeoutId = setTimeout(() => {
          this.finishExport(new Error('Timed out waiting for device export'))
        }, 12000)
      })

      await this.sendLine('EXPORT_CUSTOMERS')

      const lines = await promise
      return this.parseExportLines(lines)
    },

    async exportDeviceInfoFromDevice() {
      if (this.exportInProgress) {
        throw new Error('Export already in progress')
      }

      this.exportInProgress = true
      this.exportLines = []
      this.lastAck = null
      this.exportBeginMarker = 'BEGIN_DEVICE_INFO'
      this.exportEndMarker = 'END_DEVICE_INFO'
      this.exportLinePrefix = 'INFO|'

      const promise = new Promise((resolve, reject) => {
        this.exportResolve = resolve
        this.exportReject = reject
        this.exportTimeoutId = setTimeout(() => {
          this.finishExport(new Error('Timed out waiting for device export'))
        }, 8000)
      })

      await this.sendLine('EXPORT_DEVICE_INFO')

      const lines = await promise
      return this.parseDeviceInfoLines(lines)
    },

    async exportReadingsFromDevice() {
      if (this.exportInProgress) {
        throw new Error('Export already in progress')
      }

      this.exportInProgress = true
      this.exportLines = []
      this.lastAck = null
      this.exportBeginMarker = 'BEGIN_READINGS'
      this.exportEndMarker = 'END_READINGS'
      this.exportLinePrefix = 'READ|'

      const promise = new Promise((resolve, reject) => {
        this.exportResolve = resolve
        this.exportReject = reject
        this.exportTimeoutId = setTimeout(() => {
          this.finishExport(new Error('Timed out waiting for device export'))
        }, 12000)
      })

      await this.sendLine('EXPORT_READINGS')

      const lines = await promise
      return this.parseReadingsExportLines(lines)
    },

    finishExport(error, lines = null) {
      if (this.exportTimeoutId) {
        clearTimeout(this.exportTimeoutId)
        this.exportTimeoutId = null
      }

      const resolve = this.exportResolve
      const reject = this.exportReject
      this.exportResolve = null
      this.exportReject = null
      this.exportInProgress = false

      if (error) {
        if (reject) reject(error)
        return
      }
      if (resolve) resolve(lines || [])
    },

    parseExportLines(lines) {
      const customers = []
      for (const line of lines) {
        if (!line.startsWith('CUST|')) continue
        const parts = line.split('|')
        // CUST|account_no|customer_name|address|previous_reading|status|type_id|deduction_id|brgy_id
        if (parts.length < 9) continue
        customers.push({
          account_no: parts[1],
          customer_name: parts[2],
          address: parts[3],
          previous_reading: Number(parts[4] || 0),
          status: parts[5] || 'active',
          type_id: Number(parts[6] || 1),
          deduction_id: Number(parts[7] || null),
          brgy_id: Number(parts[8] || 1),
        })
      }
      return customers
    },

    parseDeviceInfoLines(lines) {
      const out = {}
      for (const line of lines) {
        if (!line.startsWith('INFO|')) continue
        const parts = line.split('|')
        // INFO|key|value
        if (parts.length < 3) continue
        const key = parts[1]
        const value = parts.slice(2).join('|')
        if (
          key === 'pending_readings' ||
          key === 'print_count' ||
          key === 'last_sync_epoch' ||
          key === 'device_id' ||
          key === 'brgy_id' ||
          key === 'sd_present' ||
          key === 'sd_total_bytes' ||
          key === 'sd_used_bytes' ||
          key === 'sd_free_bytes' ||
          key === 'cpu_freq_mhz' ||
          key === 'heap_free_bytes' ||
          key === 'heap_min_free_bytes' ||
          key === 'heap_max_alloc_bytes'
        ) {
          out[key] = Number(value || 0)
        } else {
          out[key] = value
        }
      }
      return out
    },

    parseReadingsExportLines(lines) {
      const readings = []
      for (const line of lines) {
        if (!line.startsWith('READ|')) continue
        const parts = line.split('|')
        // READ|customer_id|device_id|previous_reading|current_reading|usage_m3|reading_at_epoch
        if (parts.length < 7) continue
        readings.push({
          customer_id: Number(parts[1] || 0),
          device_id: Number(parts[2] || 1),
          previous_reading: Number(parts[3] || 0),
          current_reading: Number(parts[4] || 0),
          usage_m3: Number(parts[5] || 0),
          reading_at: Number(parts[6] || 0),
        })
      }
      return readings
    },

    async pushCustomersToDevice(dbCustomers) {
      // Send each upsert line-by-line (adds new customers, updates existing ones)
      for (var i = 0; i < dbCustomers.length; i++) {
        var c = dbCustomers[i]
        var line = [
          'UPSERT_CUSTOMER',
          this.escapeDeviceField(c.account_no),
          this.escapeDeviceField(c.customer_name),
          this.escapeDeviceField(c.address),
          String(Number(c.previous_reading || 0)),
          this.escapeDeviceField(c.status || 'active'),
          String(Number(c.type_id || 1)),
          String(Number(c.deduction_id || 0)),
          String(Number(c.brgy_id || 1))
        ].join('|')
        await this.sendLine(line)
        // tiny pacing helps avoid overwhelming the serial buffer
        await new Promise(r => setTimeout(r, 10))
      }
    },
    async pushCustomerTypesToDevice(dbCustomerTypes) {
      // Send each upsert line-by-line (adds new customer types, updates existing ones)
      for (var i = 0; i < dbCustomerTypes.length; i++) {
        var ct = dbCustomerTypes[i]
        var createdAt = ct.created_at ? Math.floor(new Date(ct.created_at).getTime() / 1000) : 0
        var updatedAt = ct.updated_at ? Math.floor(new Date(ct.updated_at).getTime() / 1000) : 0
        var line = [
          'UPSERT_CUSTOMER_TYPE',
          String(Number(ct.id || ct.type_id || 1)),
          this.escapeDeviceField(ct.type_name || ct.name || ''),
          String(Number(ct.rate_per_m3 || 0)),
          String(Number(ct.min_m3 || 0)),
          String(Number(ct.min_charge || 0)),
          String(Number(ct.penalty || 0)),
          String(createdAt),
          String(updatedAt)
        ].join('|')
        await this.sendLine(line)
        // tiny pacing helps avoid overwhelming the serial buffer
        await new Promise(r => setTimeout(r, 10))
      }
    },

    async pushDeductionsToDevice(dbDeductions) {
      // Send each upsert line-by-line (adds new deductions, updates existing ones)
      for (var i = 0; i < dbDeductions.length; i++) {
        var d = dbDeductions[i]
        var createdAt = d.created_at ? Math.floor(new Date(d.created_at).getTime() / 1000) : 0
        var updatedAt = d.updated_at ? Math.floor(new Date(d.updated_at).getTime() / 1000) : 0
        var line = [
          'UPSERT_DEDUCTION',
          String(Number(d.id || d.deduction_id || 1)),
          this.escapeDeviceField(d.name || ''),
          this.escapeDeviceField(d.type || ''),
          String(Number(d.value || 0)),
          String(createdAt),
          String(updatedAt)
        ].join('|')
        await this.sendLine(line)
        // tiny pacing helps avoid overwhelming the serial buffer
        await new Promise(r => setTimeout(r, 10))
      }
    },

    // Reading/parsing is handled by serialService now.

    handleDeviceLine(line) {
      if (!line) return

      // Track acks (useful for debugging)
      if (line.startsWith('ACK|')) {
        this.lastAck = line
      }

      if (!this.exportInProgress) return

      if (this.exportBeginMarker && line === this.exportBeginMarker) {
        this.exportLines = []
        return
      }

      if (this.exportEndMarker && line === this.exportEndMarker) {
        const lines = this.exportLines.slice()
        this.exportLines = []
        this.finishExport(null, lines)
        return
      }

      if (this.exportLinePrefix && line.startsWith(this.exportLinePrefix)) {
        this.exportLines.push(line)
      }

      if (line.startsWith('ERR|')) {
        this.finishExport(new Error('Device error: ' + line))
      }
    },

    // Disconnect handling is handled by serialService now.

    async formatSdCard() {
      var st = serialService.getState()
      if (!this.isConnected || !st.writer) {
        this.addLog('Not connected. Connect a device first.')
        this.showFormatConfirm = false
        return
      }

      this.isFormatting = true
      try {
        this.addLog('Formatting SD card...')
        await this.sendLine('FORMAT_SD')
        
        // Wait for acknowledgment
        await new Promise(resolve => setTimeout(resolve, 2000))
        
        this.commandSuccessMessage = 'SD card has been formatted successfully. The device will reinitialize the file system.'
        this.showCommandSuccess = true
        this.addLog('✓ SD card formatted successfully')
      } catch (error) {
        this.addLog('Format failed: ' + (error?.message || String(error)))
      } finally {
        this.isFormatting = false
        this.showFormatConfirm = false
      }
    },

    async restartDevice() {
      var st = serialService.getState()
      if (!this.isConnected || !st.writer) {
        this.addLog('Not connected. Connect a device first.')
        this.showRestartConfirm = false
        return
      }

      this.isRestarting = true
      try {
        this.addLog('Restarting device...')
        await this.sendLine('RESTART_DEVICE')
        
        // Wait a moment for the command to be sent
        await new Promise(resolve => setTimeout(resolve, 500))
        
        this.commandSuccessMessage = 'Device restart command sent. The device will restart momentarily.'
        this.showCommandSuccess = true
        this.addLog('✓ Restart command sent')
        
        // Disconnect after restart command
        setTimeout(() => {
          this.disconnectDevice()
        }, 1000)
      } catch (error) {
        this.addLog('Restart failed: ' + (error?.message || String(error)))
      } finally {
        this.isRestarting = false
        this.showRestartConfirm = false
      }
    },

    addLog(message) {
      const time = new Date().toLocaleTimeString()
      this.syncLogs.push({ time, message })
      
      // Auto-scroll to bottom
      this.$nextTick(() => {
        const logContainer = this.$el.querySelector('.overflow-y-auto')
        if (logContainer) {
          logContainer.scrollTop = logContainer.scrollHeight
        }
      })
    }
  }
}
</script>
