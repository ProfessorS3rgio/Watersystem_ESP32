import { ref } from 'vue'
import { serialService } from '../../services/serialService'

export function useSyncController() {
  const customCommand = ref('')
  const isFormatting = ref(false)
  const isRestarting = ref(false)
  const showFormatConfirm = ref(false)
  const showRestartConfirm = ref(false)
  const showDropConfirm = ref(false)
  const showCommandSuccess = ref(false)
  const commandSuccessMessage = ref('')

  const sendCommand = async (command, addLog) => {
    const st = serialService.getState()
    if (!st.writer) {
      addLog('Not connected. Connect a device first.')
      return
    }

    try {
      await serialService.sendLine(command)
      addLog('Web → ' + command)
    } catch (error) {
      addLog('Write failed: ' + error.message)
    }
  }

  const sendCustomCommand = async (addLog) => {
    if (!customCommand.value.trim()) {
      addLog('Please enter a command')
      return
    }

    await sendCommand(customCommand.value.trim(), addLog)
    customCommand.value = '' // Clear the input after sending
  }

  const formatSdCard = async (addLog, sendLineDevice) => {
    const st = serialService.getState()
    if (!st.writer) {
      addLog('Not connected. Connect a device first.')
      showFormatConfirm.value = false
      return
    }

    isFormatting.value = true
    try {
      addLog('Formatting SD card...')
      await sendLineDevice('FORMAT_SD')

      // Wait for acknowledgment
      await new Promise(resolve => setTimeout(resolve, 2000))

      commandSuccessMessage.value = 'SD card has been formatted successfully. The device will reinitialize the file system.'
      showCommandSuccess.value = true
      addLog('✓ SD card formatted successfully')
    } catch (error) {
      addLog('Format failed: ' + (error?.message || String(error)))
    } finally {
      isFormatting.value = false
      showFormatConfirm.value = false
    }
  }

  const restartDevice = async (addLog, sendLineDevice, disconnectDevice) => {
    const st = serialService.getState()
    if (!st.writer) {
      addLog('Not connected. Connect a device first.')
      showRestartConfirm.value = false
      return
    }

    isRestarting.value = true
    try {
      addLog('Restarting device...')
      await sendLineDevice('RESTART_DEVICE')

      // Wait a moment for the command to be sent
      await new Promise(resolve => setTimeout(resolve, 500))

      commandSuccessMessage.value = 'Device restart command sent. The device will restart momentarily.'
      showCommandSuccess.value = true
      addLog('✓ Restart command sent')

      // Disconnect after restart command
      setTimeout(() => {
        disconnectDevice()
      }, 1000)
    } catch (error) {
      addLog('Restart failed: ' + (error?.message || String(error)))
    } finally {
      isRestarting.value = false
      showRestartConfirm.value = false
    }
  }

  const dropDatabase = async (addLog, sendLineDevice) => {
    const st = serialService.getState()
    if (!st.writer) {
      addLog('Not connected. Connect a device first.')
      return
    }

    try {
      addLog('Dropping database...')
      await sendLineDevice('DROP_DB')

      // Wait for acknowledgment
      await new Promise(resolve => setTimeout(resolve, 2000))

      commandSuccessMessage.value = 'Database has been dropped and reinitialized successfully.'
      showCommandSuccess.value = true
      addLog('✓ Database dropped successfully')
    } catch (error) {
      addLog('Drop database failed: ' + (error?.message || String(error)))
    }
  }

  return {
    // Reactive state
    customCommand,
    isFormatting,
    isRestarting,
    showFormatConfirm,
    showRestartConfirm,
    showDropConfirm,
    showCommandSuccess,
    commandSuccessMessage,

    // Methods
    sendCommand,
    sendCustomCommand,
    formatSdCard,
    restartDevice,
    dropDatabase
  }
}