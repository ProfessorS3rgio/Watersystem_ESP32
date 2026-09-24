import assert from 'node:assert/strict'
import { sanitizeCustomerForDevice } from '../resources/js/composables/Sync/customerSyncPayload.js'

const customer = {
  account_no: 'M-001',
  customer_name: 'Gladys Jean G. Elentorio',
  address: 'Makilas',
  previous_reading: 4256,
  current_reading: 4260,
  status: 'active',
  type_id: 2,
  deduction_id: null,
  brgy_id: 1,
}

const payload = sanitizeCustomerForDevice(customer)
assert.equal(payload.previous_reading, 4260, 'should sync the latest current reading when available')
console.log('sync customer reading test passed')
