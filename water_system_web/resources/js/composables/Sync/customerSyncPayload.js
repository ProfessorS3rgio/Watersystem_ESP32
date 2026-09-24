export function sanitizeCustomerForDevice(customer = {}) {
  const latestCurrentReading = Number(customer.current_reading ?? customer.previous_reading ?? 0)

  return {
    account_no: customer.account_no ?? '',
    customer_name: customer.customer_name ?? '',
    address: customer.address ?? '',
    previous_reading: latestCurrentReading,
    status: customer.status ?? 'active',
    type_id: Number(customer.type_id ?? 1),
    deduction_id: customer.deduction_id == null ? null : Number(customer.deduction_id),
    brgy_id: Number(customer.brgy_id ?? 1),
  }
}
