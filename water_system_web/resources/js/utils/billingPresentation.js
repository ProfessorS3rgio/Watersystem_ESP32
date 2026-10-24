// Keep checkout amounts consistent between the cashier and bill register.
export function billBalance(bill) {
  const amount = Number(bill?.balance_due ?? bill?.total_due ?? 0)
  return Number.isFinite(amount) ? Math.max(0, amount) : 0
}

export function formatMoney(amount) {
  return Number(amount || 0).toLocaleString('en-PH', {
    minimumFractionDigits: 2,
    maximumFractionDigits: 2,
  })
}
