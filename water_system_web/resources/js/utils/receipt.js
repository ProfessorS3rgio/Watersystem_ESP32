import { formatMoney } from './billingPresentation.js'

const escapeHtml = value => String(value ?? '')
  .replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;')
  .replace(/"/g, '&quot;').replace(/'/g, '&#039;')

const money = value => value === null || value === undefined || value === '' || !Number.isFinite(Number(value))
  ? '—' : `PHP ${formatMoney(value)}`

const hasValue = value => value !== null && value !== undefined && value !== ''
const formatDate = value => {
  // Treat date-only bill/due dates as calendar dates, with no timezone shift.
  const date = value ? new Date(/^\d{4}-\d{2}-\d{2}$/.test(value) ? `${value}T00:00:00+08:00` : value) : null
  return date && !Number.isNaN(date.getTime())
    ? date.toLocaleDateString('en-PH', { timeZone: 'Asia/Manila', year: 'numeric', month: 'short', day: '2-digit' }) : '—'
}

export function receiptMarkup(receipt, customerName = '') {
  const date = receipt.transaction_date || receipt.created_at
  const parsedDate = date ? new Date(date) : null
  const validDate = parsedDate && !Number.isNaN(parsedDate.getTime())
  const row = (label, value, className = '') => `<div class="receipt-row ${className}"><span>${label}</span><strong>${escapeHtml(value)}</strong></div>`
  const optionalRow = (label, value) => hasValue(value) ? row(label, value) : ''
  const contact = receipt.contact || {}
  const hasMeter = [receipt.previous_reading, receipt.current_reading, receipt.usage_m3].some(hasValue)
  const hasBilling = [receipt.billing_period, receipt.bill_date, receipt.total_due, receipt.charges].some(hasValue)

  return `<article class="thermal-receipt" aria-label="Payment receipt">
    <header class="receipt-brand">
      <img class="receipt-logo" src="/images/dmbc-seal.png" alt="DMBC Water System Association seal" width="92" height="92">
      <h1>DMBC WATER</h1>
      <p class="receipt-association">Water System Association</p>
      <p>Doña Josefa · Makilas<br>Buluan · Caparan</p>
      <p>Ipil, Zamboanga Sibugay</p>
    </header>
    <h2 class="receipt-title">PAYMENT RECEIPT</h2>
    <section class="receipt-section receipt-identity">
      <p class="receipt-caption">CUSTOMER</p>
      <p class="receipt-customer">${escapeHtml(customerName || receipt.customer_name || 'Customer')}</p>
      ${row('Account', receipt.account_no || '—')}
      ${optionalRow('Class', receipt.customer_type)}
      ${receipt.address ? `<p class="receipt-address">${escapeHtml(receipt.address)}</p>` : ''}
      ${optionalRow('Barangay', receipt.barangay)}
      ${row('Bill no.', receipt.reference_number || '—')}
      ${receipt.id ? row('Transaction', receipt.id) : ''}
      ${row('Date', validDate ? parsedDate.toLocaleDateString('en-PH', { timeZone: 'Asia/Manila', year: 'numeric', month: 'short', day: '2-digit' }) : '—')}
      ${row('Time', validDate ? parsedDate.toLocaleTimeString('en-PH', { timeZone: 'Asia/Manila', hour: '2-digit', minute: '2-digit' }) : '—')}
    </section>
    ${hasBilling ? `<section class="receipt-section">
      <p class="receipt-caption">BILLING DETAILS</p>
      ${receipt.billing_period ? `<p class="receipt-period">${escapeHtml(receipt.billing_period)}</p>` : optionalRow('Bill date', receipt.bill_date ? formatDate(receipt.bill_date) : null)}
      ${optionalRow('Due date', receipt.due_date ? formatDate(receipt.due_date) : null)}
      ${hasMeter ? `<div class="receipt-meter">
        <div><span>Previous</span><strong>${escapeHtml(receipt.previous_reading ?? '—')}</strong></div>
        <div><span>Present</span><strong>${escapeHtml(receipt.current_reading ?? '—')}</strong></div>
        <div><span>Used (m³)</span><strong>${escapeHtml(receipt.usage_m3 ?? '—')}</strong></div>
      </div>` : ''}
      ${optionalRow('Rate / m³', hasValue(receipt.rate_per_m3) ? money(receipt.rate_per_m3) : null)}
      ${optionalRow('Water charge', hasValue(receipt.charges) ? money(receipt.charges) : null)}
      ${optionalRow('Penalty / late fee', hasValue(receipt.penalty) ? money(receipt.penalty) : null)}
      ${optionalRow('Bill total', hasValue(receipt.total_due) ? money(receipt.total_due) : null)}
    </section>` : ''}
    <section class="receipt-section">
      <p class="receipt-caption">WATER BILL PAYMENT</p>
      ${row('Amount paid', money(receipt.amount_paid ?? receipt.amount), 'receipt-total')}
      ${row('Cash received', money(receipt.cash_received))}
      ${row('Change', money(receipt.change_amount ?? receipt.change))}
      ${optionalRow('Method', receipt.payment_method)}
      ${optionalRow('Collector', receipt.collector || receipt.payment_source)}
      ${hasValue(receipt.bill_balance) ? `${row('Bill balance now', money(receipt.bill_balance))}
        <p class="receipt-note">Balance for this bill as of ${escapeHtml(formatDate(receipt.balance_as_of))}.</p>` : ''}
    </section>
    <section class="receipt-section receipt-contact">
      <p class="receipt-caption">CUSTOMER ASSISTANCE</p>
      ${contact.contact_is_example ? '<p class="receipt-note receipt-example">SAMPLE CONTACT DETAILS</p>' : ''}
      <p>${escapeHtml(contact.office_address || 'Makilas, Ipil, Zamboanga Sibugay')}</p>
      ${contact.phone ? `<p>Tel: ${escapeHtml(contact.phone)}</p>` : ''}
      ${contact.email ? `<p>${escapeHtml(contact.email)}</p>` : ''}
      ${contact.office_hours ? `<p>${escapeHtml(contact.office_hours)}</p>` : ''}
      <p class="receipt-note">For billing concerns or leak reports,<br>please quote your account number.</p>
    </section>
    <footer class="receipt-footer">
      <p class="receipt-thanks">Thank you for your payment!</p>
      <p>Please keep this receipt<br>for your records.</p>
      <p class="receipt-tagline">Every drop counts. Save water.</p>
    </footer>
  </article>`
}

// Shared by the in-app preview and the standalone print document.
export const receiptStyles = `
  .thermal-receipt, .thermal-receipt * { box-sizing: border-box; }
  .thermal-receipt { width: 48mm; margin: 0 auto; padding: 3mm 0 5mm; color: #000; background: #fff; font: 11px/1.45 Arial, sans-serif; text-align: left; overflow-wrap: anywhere; }
  .thermal-receipt p, .thermal-receipt h1, .thermal-receipt h2 { margin: 0; color: #000; }
  .thermal-receipt .receipt-brand { text-align: center; }
  .thermal-receipt .receipt-logo { display: block; width: 24mm; height: 24mm; object-fit: contain; margin: 0 auto 2mm; filter: grayscale(1) contrast(1.3); }
  .thermal-receipt h1 { font-size: 17px; font-weight: 800; letter-spacing: .7px; line-height: 1.2; }
  .thermal-receipt .receipt-association { font-size: 10px; font-weight: 700; margin: 1mm 0; }
  .thermal-receipt .receipt-brand p { font-size: 10px; line-height: 1.4; }
  .thermal-receipt .receipt-title { margin-top: 3mm; padding: 2mm 0; border-top: 1.5px solid #000; border-bottom: 1.5px solid #000; text-align: center; font-size: 11px; font-weight: 800; letter-spacing: 1px; }
  .thermal-receipt .receipt-section { padding: 2.5mm 0; border-bottom: 1px dashed #000; }
  .thermal-receipt .receipt-caption { font-size: 9px; font-weight: 700; letter-spacing: .7px; margin-bottom: 1mm; }
  .thermal-receipt .receipt-customer { font-size: 13px; font-weight: 700; line-height: 1.3; margin-bottom: 2mm; }
  .thermal-receipt .receipt-address { margin-top: 1mm; font-size: 10px; }
  .thermal-receipt .receipt-period { margin-bottom: 1.5mm; font-size: 10px; font-weight: 700; }
  .thermal-receipt .receipt-meter { display: grid; grid-template-columns: repeat(3, minmax(0, 1fr)); gap: 1mm; padding: 2mm 0; margin: 1mm 0; border-top: 1px dotted #000; border-bottom: 1px dotted #000; text-align: center; }
  .thermal-receipt .receipt-meter span { display: block; font-size: 9px; }
  .thermal-receipt .receipt-meter strong { display: block; font-size: 12px; font-weight: 700; }
  .thermal-receipt .receipt-note { font-size: 9px; line-height: 1.4; margin-top: 1.5mm; }
  .thermal-receipt .receipt-contact { text-align: center; font-size: 10px; }
  .thermal-receipt .receipt-example { font-weight: 700; margin: 0 0 1mm; }
  .thermal-receipt .receipt-row { display: flex; align-items: baseline; justify-content: space-between; gap: 2mm; margin-top: 1mm; }
  .thermal-receipt .receipt-row > span { flex-shrink: 0; }
  .thermal-receipt .receipt-row > strong { min-width: 0; font-weight: 600; text-align: right; font-variant-numeric: tabular-nums; }
  .thermal-receipt .receipt-total { display: block; margin: 1mm 0 2mm; padding-bottom: 2mm; border-bottom: 1px solid #000; }
  .thermal-receipt .receipt-total > span { display: block; font-size: 11px; }
  .thermal-receipt .receipt-total > strong { display: block; font-size: 21px; line-height: 1.3; font-weight: 800; text-align: left; }
  .thermal-receipt .receipt-footer { padding-top: 3mm; text-align: center; font-size: 10px; }
  .thermal-receipt .receipt-thanks { font-weight: 700; margin-bottom: 1mm; }
  .thermal-receipt .receipt-tagline { margin-top: 2.5mm; font-size: 9px; }
`

export function openReceiptWindow() {
  const popup = window.open('', '_blank', 'width=460,height=780')
  if (!popup) throw new Error('Please allow pop-ups for this website to print receipts.')
  popup.document.write('<!doctype html><html><head><title>Preparing receipt</title></head><body><p>Preparing your receipt…</p></body></html>')
  popup.document.close()
  return popup
}

export async function printReceiptDocument(receipt, customerName, popup = openReceiptWindow()) {
  if (popup.closed) throw new Error('The receipt window was closed. Please try printing again.')
  popup.document.open()
  popup.document.write(`<!doctype html><html lang="en"><head><meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Payment receipt ${escapeHtml(receipt.reference_number)}</title>
    <style>${receiptStyles}
      body { margin: 0; background: #edf3f2; }
      .print-tools { max-width: 360px; margin: 24px auto; padding: 0 20px; font: 13px/1.6 Arial, sans-serif; color: #244b46; }
      .print-tools button { padding: 10px 18px; border: 0; border-radius: 8px; background: #0f766e; color: #fff; cursor: pointer; font-weight: 700; }
      .receipt-paper { box-sizing: border-box; width: 58mm; margin: 0 auto 24px; padding: 0 5mm; background: #fff; box-shadow: 0 8px 30px #173c3920; }
      @media print { body { width: 58mm; background: #fff; } .print-tools { display: none; } .receipt-paper { margin: 0; box-shadow: none; } }
    </style></head><body>
    <aside class="print-tools"><button id="print-receipt" type="button">Print receipt</button>
    <p>Select your Xprinter · 58 mm roll · 100% scale · no margins.<br>Turn off browser headers and footers.</p><p id="print-status" role="status">Loading logo…</p></aside>
    <main class="receipt-paper">${receiptMarkup(receipt, customerName)}</main></body></html>`)
  popup.document.close()
  const print = () => { popup.focus(); popup.print() }
  const button = popup.document.getElementById('print-receipt')
  button.disabled = true
  try {
    await Promise.all([...popup.document.images].map(img => img.decode()))
    await popup.document.fonts.ready
    if (popup.closed) return
    // Use an explicit roll length: CSS does not support "58mm auto" page sizes.
    const heightMm = Math.ceil(popup.document.querySelector('.receipt-paper').getBoundingClientRect().height * 25.4 / 96) + 2
    const pageStyle = popup.document.createElement('style')
    pageStyle.textContent = `@page { size: 58mm ${heightMm}mm; margin: 0; }`
    popup.document.head.append(pageStyle)
    popup.document.getElementById('print-status').textContent = 'Receipt ready. You can print again from this window.'
    button.disabled = false
    button.addEventListener('click', print)
    print()
  } catch (error) {
    if (!popup.closed) popup.document.getElementById('print-status').textContent = 'The receipt logo could not load. Close this window and try again.'
    throw error
  }
}
