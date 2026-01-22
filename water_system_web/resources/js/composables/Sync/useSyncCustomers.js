import { ref } from 'vue'
import { serialService } from '../../services/serialService'

export function useSyncCustomers() {
  const chunkAckPromise = ref(null)
  const chunkAckResolve = ref(null)
  const chunkAckReject = ref(null)
  const expectedChunkAck = ref(null)
  const chunkAckTimeoutId = ref(null)

  const sendLine = async (line) => {
    const text = String(line).replace(/\r|\n/g, '')
    await serialService.sendLine(text)
  }

  const sendCustomerChunks = async (type, customers) => {
    // Send customers in chunks sequentially, waiting for ACK each time.
    // Register the ACK waiter BEFORE sending to avoid ACK arriving
    // while the promise hasn't been hooked (race condition).
    // Implement retries in case ACKs are lost or the web serial write fails.
    const chunkSize = 50
    const totalChunks = Math.ceil(customers.length / chunkSize)
    const maxRetries = 3
    for (let i = 0; i < customers.length; i += chunkSize) {
      const chunk = customers.slice(i, i + chunkSize)
      const customersJson = JSON.stringify(chunk).replace(/\|/g, '\\|')
      const chunkIndex = Math.floor(i / chunkSize)
      const line = `UPSERT_${type}_CUSTOMER_JSON_CHUNK|${chunkIndex}|${totalChunks}|${customersJson}`

      let attempt = 0
      while (attempt < maxRetries) {
        attempt++
        try {
          // Create the ACK promise first so the line handler can resolve it
          const ackPromise = waitForChunkAck(chunkIndex, totalChunks)
          await sendLine(line)
          // Wait for ACK (or timeout inside waitForChunkAck)
          await ackPromise
          // Success
          break
        } catch (error) {
          console.error(`Chunk ${chunkIndex} for ${type} attempt ${attempt} failed:`, error)
          if (attempt >= maxRetries) {
            throw new Error(`Failed to send chunk ${chunkIndex} for ${type} after ${maxRetries} attempts`)
          }
          // Backoff before retry
          await new Promise(resolve => setTimeout(resolve, 500 * attempt))
        }
      }

      // Small pause to let device finalize processing before next chunk
      await new Promise(resolve => setTimeout(resolve, 200))
    }
  }

  const pushCustomersToDevice = async (newCustomers, updatedCustomers) => {
    // Send new customers first
    if (newCustomers && newCustomers.length > 0) {
      await sendCustomerChunks('NEW', newCustomers)
    }
    // Then send updated customers
    if (updatedCustomers && updatedCustomers.length > 0) {
      await sendCustomerChunks('UPDATED', updatedCustomers)
    }
  }

  const waitForChunkAck = (chunkIndex, totalChunks) => {
    return new Promise((resolve, reject) => {
      chunkAckResolve.value = resolve
      chunkAckReject.value = reject
      expectedChunkAck.value = chunkIndex
      // Timeout after 30 seconds
      if (chunkAckTimeoutId.value) {
        clearTimeout(chunkAckTimeoutId.value)
        chunkAckTimeoutId.value = null
      }
      chunkAckTimeoutId.value = setTimeout(() => {
        if (chunkAckResolve.value) {
          chunkAckResolve.value = null
          chunkAckReject.value = null
          expectedChunkAck.value = null
          chunkAckTimeoutId.value = null
          reject(new Error('Timeout waiting for chunk ACK'))
        }
      }, 30000)
    })
  }

  const handleChunkAck = (line) => {
    // Handle chunk ACKs: only resolve if ACK matches expected chunk index
    if (line.startsWith('ACK|CHUNK|') || line.startsWith('ACK|UPSERT_NEW_CUSTOMER_JSON|') || line.startsWith('ACK|UPSERT_UPDATED_CUSTOMER_JSON|') || line.startsWith('ACK|UPSERT_CUSTOMERS_JSON|')) {
      try {
        let ackIndex = null
        if (line.startsWith('ACK|CHUNK|')) {
          const parts = line.split('|')
          ackIndex = Number(parts[2])
        } else if (line.startsWith('ACK|UPSERT_NEW_CUSTOMER_JSON|') || line.startsWith('ACK|UPSERT_UPDATED_CUSTOMER_JSON|') || line.startsWith('ACK|UPSERT_CUSTOMERS_JSON|')) {
          // final ACK may include count; treat as last chunk acknowledge
          ackIndex = expectedChunkAck.value // allow final ack to resolve current waiter
        }

        if (chunkAckResolve.value && (expectedChunkAck.value === null || ackIndex === expectedChunkAck.value)) {
          chunkAckResolve.value(line)
          chunkAckResolve.value = null
          chunkAckReject.value = null
          expectedChunkAck.value = null
          if (chunkAckTimeoutId.value) {
            clearTimeout(chunkAckTimeoutId.value)
            chunkAckTimeoutId.value = null
          }
        }
      } catch (e) {
        console.error('Error handling ACK line:', e)
      }
    }
  }

  return {
    pushCustomersToDevice,
    handleChunkAck
  }
}