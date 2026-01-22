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
                @click="handleSyncData"
                :disabled="isSyncing"
                class="bg-blue-600 hover:bg-blue-700 text-white px-4 py-3 rounded-lg font-medium shadow transition-all duration-200 disabled:opacity-50 flex items-center justify-center space-x-2"
              >
                <svg class="h-5 w-5" :class="{ 'animate-spin': isSyncing }" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                  <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15" />
                </svg>
                <span>{{ isSyncing ? 'Syncing...' : 'Sync Data' }}</span>
              </button>

              <button
                @click="showDropConfirm = true"
                class="bg-red-600 hover:bg-red-700 text-white px-4 py-3 rounded-lg font-medium shadow transition-all duration-200 flex items-center justify-center space-x-2"
              >
                <svg class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                  <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M19 7l-.867 12.142A2 2 0 0116.138 21H7.862a2 2 0 01-1.995-1.858L5 7m5 4v6m4-6v6m1-10V4a1 1 0 00-1-1h-4a1 1 0 00-1 1v3M4 7h16" />
                </svg>
                <span>Drop Database</span>
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

            <!-- Custom Command Input -->
            <div class="mt-4">
              <label class="block text-sm font-medium text-gray-700 mb-2">Custom Command</label>
              <div class="flex space-x-2">
                <input
                  v-model="customCommand"
                  @keyup.enter="sendCustomCommand(addLog)"
                  type="text"
                  placeholder="Enter command to send to device..."
                  class="flex-1 px-3 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                />
                <button
                  @click="sendCustomCommand(addLog)"
                  class="bg-green-600 hover:bg-green-700 text-white px-4 py-2 rounded-lg font-medium shadow transition-all duration-200 flex items-center space-x-2"
                >
                  <svg class="h-4 w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 19l9 2-9-18-9 18 9-2zm0 0v-8" />
                  </svg>
                  <span>Send</span>
                </button>
              </div>
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
            <span class="text-sm text-gray-600">Customer Count</span>
            <span class="text-sm font-medium text-gray-900">{{ customerCount }}</span>
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
      <div class="flex items-center justify-between mb-4">
        <h2 class="text-xl font-semibold text-gray-900">Sync Log</h2>
        <button
          @click="clearSyncLogs"
          class="text-gray-600 hover:text-gray-800 text-sm font-medium px-3 py-1 rounded border border-gray-300 hover:border-gray-400 transition-colors duration-200"
        >
          Clear Logs
        </button>
      </div>
      <div class="bg-gray-900 text-green-400 font-mono text-sm p-4 rounded-lg h-96 overflow-y-auto">
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
      @confirm="formatSdCard(addLog, sendLineDevice)"
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
      @confirm="restartDevice(addLog, sendLineDevice, disconnectDevice)"
      @cancel="showRestartConfirm = false"
    />

    <!-- Drop Database Confirmation -->
    <ConfirmDialog
      :is-open="showDropConfirm"
      title="Drop Database"
      :message="'This will delete all data in the ESP32 database including customers, readings, and settings. The database will be reinitialized.<br><br>Are you sure you want to drop the database?'"
      confirm-text="Drop Database"
      cancel-text="Cancel"
      :is-loading="false"
      @confirm="dropDatabase(addLog, sendLineDevice)"
      @cancel="showDropConfirm = false"
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
import { useSyncDevice } from '../composables/Sync/useSyncDevice'
import { useSyncCustomers } from '../composables/Sync/useSyncCustomers'
import { useSyncCustomerTypes } from '../composables/Sync/useSyncCustomerTypes'
import { useSyncDeductions } from '../composables/Sync/useSyncDeductions'
import { useSyncReadings } from '../composables/Sync/useSyncReadings'
import { useSyncData } from '../composables/Sync/useSyncData'
import { useSyncController } from '../composables/Sync/useSyncController'

export default {
  name: 'SyncDevice',
  components: {
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
      syncReadingsFromDevice,
      handleDeviceLine: handleDeviceLineReadings
    } = useSyncReadings()

    const {
      isSyncing,
      syncLogs,
      syncData,
      addLog
    } = useSyncData()

    const {
      customCommand,
      isFormatting,
      isRestarting,
      showFormatConfirm,
      showRestartConfirm,
      showDropConfirm,
      showCommandSuccess,
      commandSuccessMessage,
      sendCommand,
      sendCustomCommand,
      formatSdCard,
      restartDevice,
      dropDatabase
    } = useSyncController()

    return {
      deviceId,
      brgyId,
      firmwareVersion,
      lastSync,
      pendingRecords,
      printCount,
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
      syncReadingsFromDevice,
      handleDeviceLineReadings,
      isSyncing,
      syncLogs,
      syncData,
      addLog,
      customCommand,
      isFormatting,
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
      showDropConfirm
    }
  },
  data() {
    return {
      isConnected: false,
      connectedPort: '',
      batteryLevel: 85,

      showSuccessDialog: false,

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

    async handleSyncData() {
      await this.syncData({
        isConnected: this.isConnected,
        exportDeviceInfoFromDevice: this.exportDeviceInfoFromDevice,
        pushCustomerTypesToDevice: this.pushCustomerTypesToDevice,
        pushDeductionsToDevice: this.pushDeductionsToDevice,
        syncReadingsFromDevice: this.syncReadingsFromDevice,
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
      }

      // Handle device info and readings exports
      this.handleDeviceLineDevice(line)
      this.handleDeviceLineReadings(line)

      if (line.startsWith('ERR|')) {
        // Error handling is done in the individual handlers
      }
    },

    clearSyncLogs() {
      this.syncLogs.length = 0
    },

    // Disconnect handling is handled by serialService now.


  }
}
</script>
