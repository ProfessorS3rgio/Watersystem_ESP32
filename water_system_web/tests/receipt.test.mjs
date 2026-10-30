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

test('A4 receipt repeats payment data and TIN on the Customer and Office copies', () => {
  const html = receiptMarkup({
    id: 27, meter_number: '00123', customer_type: 'Residential', address: 'Purok 2 <West>', barangay: 'Makilas',
    billing_period: 'Oct 1, 2026 – Oct 24, 2026', due_date: '2026-10-29',
    previous_reading: 0, current_reading: 12, usage_m3: 12,
    rate_per_m3: 20, charges: 240, penalty: 20, total_due: 260,
    amount_paid: 100, cash_received: 100, change: 0, bill_balance: 160,
    balance_as_of: '2026-10-24T06:00:00Z', payment_method: 'cash', collector: 'Ana & Co.',
    contact: { contact_is_example: true, phone: '09XX XXX XXXX', email: 'office@example.com',
      office_hours: 'Mon–Fri', office_address: '<Example office>' },
  })
  for (const detail of ['Residential', 'Purok 2 &lt;West&gt;', 'Makilas',
    'CONSUMPTION', '>0</strong>', 'PHP 260.00', 'PHP 100.00', 'PHP 160.00',
    'Oct 29, 2026', 'Ana &amp; Co.', 'RECEIPT NO. 000027', '00123']) assert.ok(html.includes(detail), detail)
  assert.equal((html.match(/TIN: 464-252-005/g) || []).length, 2)
  assert.equal((html.match(/PARTIAL PAYMENT \/ BALANCE REMAINS/g) || []).length, 2)
  assert.ok(html.includes('Customer Copy'))
  assert.ok(html.includes('Office Copy'))
  assert.ok(html.includes('CUT ALONG LINE'))
  assert.ok(html.includes('Remaining for this bill'))
  assert.ok(html.includes('Balance as of Oct 24, 2026.'))
  assert.ok(!html.includes('office@example.com'))
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
      open() {}, write(html) { events.push(html) }, close() {},
      images: [{ decode: async () => { events.push('logo') } }],
      fonts: { ready: Promise.resolve() },
      getElementById: id => id === 'print-receipt' ? button : status,
      querySelector: () => ({ getBoundingClientRect: () => ({ height: 600 }) }),
      createElement: () => ({}),
      head: { append: style => events.push(style.textContent) },
    },
  }
  await printReceiptDocument({ amount_paid: 100 }, 'Customer', popup)
  assert.match(events[0], /@page \{ size: A4 portrait; margin: 0; \}/)
  assert.ok(!events[0].includes('58mm'))
  assert.equal(events[1], 'logo')
  assert.equal(events.at(-1), 'print')
  assert.equal(button.disabled, false)
  button.click()
  assert.equal(events.filter(event => event === 'print').length, 2)
})

test('settled and unknown balances have distinct receipt labels', () => {
  assert.ok(receiptMarkup({ amount_paid: 150, bill_balance: 0 }).includes('BILL PAID IN FULL'))
  const missing = receiptMarkup({ amount_paid: 150 })
  assert.ok(missing.includes('BILL BALANCE UNAVAILABLE'))
  assert.ok(!missing.includes('BILL PAID IN FULL'))
})

test('deduction is taken from recorded bill amounts and each payment retains its transaction number', () => {
  const bill = { reference_number: 'BILL-1', charges: 150, penalty: 0, total_due: 135, amount_paid: 100, bill_balance: 35 }
  const first = receiptMarkup({ ...bill, id: 27 })
  const second = receiptMarkup({ ...bill, id: 28 })
  assert.equal((first.match(/<span>Deduction<\/span><strong>PHP 15\.00<\/strong>/g) || []).length, 2)
  assert.equal((first.match(/RECEIPT NO\. 000027/g) || []).length, 2)
  assert.equal((second.match(/RECEIPT NO\. 000028/g) || []).length, 2)
})
