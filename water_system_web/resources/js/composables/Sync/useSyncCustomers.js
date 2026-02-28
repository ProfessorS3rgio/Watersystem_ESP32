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
    // Sequentially send chunks; automatically reduce chunk size if line becomes
    // too large for the device.  This keeps us under any hidden String limits
    // while still attempting to maximise throughput.
    let chunkSize = 60  // starting guess
    const maxLineLen = 32000
    const maxRetries = 3

    let chunkIndex = 0
    for (let i = 0; i < customers.length; /* advanced inside */) {
      const end = Math.min(i + chunkSize, customers.length)
      const chunk = customers.slice(i, end)
      const totalChunks = Math.ceil(customers.length / chunkSize)

      const rawJson = JSON.stringify(chunk)
      const customersJson = btoa(rawJson)
      const line = `UPSERT_${type}_CUSTOMER_JSON_CHUNK|${chunkIndex}|${totalChunks}|${customersJson}`

      // If the generated line is too big, halve the chunk size and retry
      if (line.length > maxLineLen) {
        chunkSize = Math.max(1, Math.floor(chunkSize / 2))
        console.warn(`chunk too large (${line.length}), lowering size to ${chunkSize}`)
        continue // try again with smaller chunk
      }

      // debug: log length so we can spot overly large transmissions
      console.debug(`sending chunk ${chunkIndex}/${totalChunks} len=${line.length}`)

      let attempt = 0
      while (attempt < maxRetries) {
        attempt++
        try {
          const ackPromise = waitForChunkAck(chunkIndex, totalChunks)
          await sendLine(line)
          await ackPromise
          break
        } catch (error) {
          console.error(`Chunk ${chunkIndex} for ${type} attempt ${attempt} failed:`, error)
          if (attempt >= maxRetries) {
            throw new Error(`Failed to send chunk ${chunkIndex} for ${type} after ${maxRetries} attempts`)
          }
          await new Promise(resolve => setTimeout(resolve, 500 * attempt))
        }
      }

      // Move forward
      i = end
      chunkIndex++
      await new Promise(resolve => setTimeout(resolve, 50))
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