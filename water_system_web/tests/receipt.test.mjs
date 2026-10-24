import assert from 'node:assert/strict'
import test from 'node:test'
import { receiptMarkup, openReceiptWindow, printReceiptDocument } from '../resources/js/utils/receipt.js'

test('receipt renders transaction amounts, preserves zero change and escapes customer data', () => {
  const html = receiptMarkup({ amount: 1234.5, cash_received: 1234.5, change_amount: 0,
    change: 100, reference_number: '<script>alert(1)</script>', account_no: 'A&1',
    transaction_date: '2026-10-24T16:30:00Z' }, '<img src=x onerror=alert(1)>')
  assert.ok(html.includes('PHP 1,234.50'))
  assert.ok(html.includes('PHP 0.00'))
  assert.ok(!html.includes('PHP 100.00'))
  assert.ok(html.includes('&lt;img src=x onerror=alert(1)&gt;'))
  assert.ok(html.includes('&lt;script&gt;'))
  assert.ok(html.includes('A&amp;1'))
  assert.ok(html.includes('Oct 25, 2026')) // Asia/Manila, regardless of browser timezone
  assert.ok(html.includes('/images/dmbc-seal.png'))
})

test('missing cash details and invalid dates are not fabricated as zero payments', () => {
  const html = receiptMarkup({ amount: 50, created_at: 'invalid' })
  assert.ok(!html.includes('NaN'))
  assert.ok(!html.includes('Invalid Date'))
  assert.ok(!html.includes('PHP 0.00'))
  assert.ok(html.includes('PHP 50.00'))
})

test('blocked popups give a useful error', () => {
  globalThis.window = { open: () => null }
  assert.throws(openReceiptWindow, /allow pop-ups/)
  delete globalThis.window
})

test('detailed receipt separates the bill from this payment and prints safe sample contacts', () => {
  const html = receiptMarkup({
    customer_type: 'Residential', address: 'Purok 2 <West>', barangay: 'Makilas',
    billing_period: 'Oct 1, 2026 – Oct 24, 2026', due_date: '2026-10-29',
    previous_reading: 0, current_reading: 12, usage_m3: 12,
    rate_per_m3: 20, charges: 240, penalty: 20, total_due: 260,
    amount_paid: 100, cash_received: 100, change: 0, bill_balance: 160,
    balance_as_of: '2026-10-24T06:00:00Z', payment_method: 'cash', collector: 'Ana & Co.',
    contact: { contact_is_example: true, phone: '09XX XXX XXXX', email: 'office@example.com',
      office_hours: 'Mon–Fri', office_address: '<Example office>' },
  })
  for (const detail of ['Residential', 'Purok 2 &lt;West&gt;', 'Makilas',
    'Used (m³)', '>0</strong>', 'PHP 260.00', 'PHP 100.00', 'PHP 160.00',
    'Oct 29, 2026', 'Ana &amp; Co.', 'SAMPLE CONTACT DETAILS',
    'office@example.com', '&lt;Example office&gt;']) assert.ok(html.includes(detail), detail)
  assert.ok(html.includes('Bill balance now'))
  assert.ok(html.includes('Balance for this bill as of Oct 24, 2026.'))
})

test('printing waits for the logo and fonts and keeps a manual reprint button', async () => {
  const events = []
  const button = { addEventListener: (event, callback) => { button.click = callback } }
  const status = {}
  const popup = {
    closed: false,
    focus: () => events.push('focus'),
    print: () => events.push('print'),
    document: {
      open() {}, write() {}, close() {},
      images: [{ decode: async () => { events.push('logo') } }],
      fonts: { ready: Promise.resolve() },
      getElementById: id => id === 'print-receipt' ? button : status,
      querySelector: () => ({ getBoundingClientRect: () => ({ height: 600 }) }),
      createElement: () => ({}),
      head: { append: style => events.push(style.textContent) },
    },
  }
  await printReceiptDocument({ amount_paid: 100 }, 'Customer', popup)
  assert.equal(events[0], 'logo')
  assert.match(events[1], /size: 58mm 161mm; margin: 0/)
  assert.equal(events.at(-1), 'print')
  assert.equal(button.disabled, false)
  button.click()
  assert.equal(events.filter(event => event === 'print').length, 2)
})
