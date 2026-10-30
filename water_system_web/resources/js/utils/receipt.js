import { formatMoney } from './billingPresentation.js'

const escapeHtml = value => String(value ?? '')
  .replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;')
  .replace(/"/g, '&quot;').replace(/'/g, '&#039;')
const hasValue = value => value !== null && value !== undefined && value !== ''
const money = value => !hasValue(value) || !Number.isFinite(Number(value)) ? '—' : `PHP ${formatMoney(value)}`
const formatDate = value => {
  const date = value ? new Date(/^\d{4}-\d{2}-\d{2}$/.test(value) ? `${value}T00:00:00+08:00` : value) : null
  return date && !Number.isNaN(date.getTime())
    ? date.toLocaleDateString('en-PH', { timeZone: 'Asia/Manila', year: 'numeric', month: 'short', day: '2-digit' }) : '—'
}

export function receiptMarkup(receipt, customerName = '') {
  const date = receipt.transaction_date || receipt.created_at
  const parsed = date ? new Date(date) : null
  const time = parsed && !Number.isNaN(parsed.getTime())
    ? parsed.toLocaleTimeString('en-PH', { timeZone: 'Asia/Manila', hour: '2-digit', minute: '2-digit' }) : '—'
  const field = (label, value) => `<div class="a4-field"><span>${label}</span><strong>${escapeHtml(value)}</strong></div>`
  const row = (label, value, className = '') => `<div class="a4-row ${className}"><span>${label}</span><strong>${escapeHtml(value)}</strong></div>`
  const deduction = [receipt.charges, receipt.penalty, receipt.total_due].every(value => hasValue(value) && Number.isFinite(Number(value)))
    ? Math.max(0, Math.round((Number(receipt.charges) + Number(receipt.penalty) - Number(receipt.total_due)) * 100) / 100) : null
  // Transaction IDs distinguish separate partial payments on the same bill.
  const receiptNumber = receipt.id ? String(receipt.id).padStart(6, '0') : '—'
  const copy = label => `<article class="a4-copy" aria-label="${label}">
    <header class="a4-header">
      <div class="a4-brand">
        <img src="/images/dmbc-seal.png" alt="DMBC Water System Association seal" width="58" height="58">
        <div><h1>DMBC WATER</h1><p class="a4-association">Water System Association</p>
          <p>Doña Josefa · Makilas · Buluan · Caparan</p><p>Ipil, Zamboanga Sibugay</p>
          <p class="a4-tin">TIN: 464-252-005</p></div>
      </div>
      <div class="a4-document"><p class="a4-copy-label">${label}</p><h2>PAYMENT RECEIPT</h2></div>
    </header>
    <section class="a4-customer">
      <div><p class="a4-caption">RECEIVED FROM</p><h3>${escapeHtml(customerName || receipt.customer_name || 'Customer')}</h3>
        <p>Account: ${escapeHtml(receipt.account_no || '—')}${receipt.barangay ? ` &nbsp; | &nbsp; Barangay: ${escapeHtml(receipt.barangay)}` : ''}${receipt.customer_type ? ` &nbsp; | &nbsp; ${escapeHtml(receipt.customer_type)}` : ''}</p>
        ${receipt.address ? `<p class="a4-address">${escapeHtml(receipt.address)}</p>` : ''}</div>
      <div class="a4-reference"><p><strong>RECEIPT NO. ${escapeHtml(receiptNumber)}</strong></p>
        <p>${escapeHtml(formatDate(date))} &nbsp; · &nbsp; ${escapeHtml(time)}</p>
        <p>Billing period: ${escapeHtml(receipt.billing_period || formatDate(receipt.bill_date))}</p>
        ${receipt.due_date ? `<p>Due date: ${escapeHtml(formatDate(receipt.due_date))}</p>` : ''}</div>
    </section>
    <section class="a4-meter" aria-label="Meter readings">
      ${field('METER NUMBER', receipt.meter_number || '—')}${field('PREVIOUS', receipt.previous_reading ?? '—')}
      ${field('PRESENT', receipt.current_reading ?? '—')}${field('CONSUMPTION', hasValue(receipt.usage_m3) ? `${receipt.usage_m3} m³` : '—')}
    </section>
    <section class="a4-payment">
      <div class="a4-charges"><h4>BILL DETAILS</h4>
        ${row('Water charges', money(receipt.charges))}${row('Penalty / late fee', money(receipt.penalty))}
        ${hasValue(deduction) ? row('Deduction', money(deduction)) : ''}
        ${row('BILL TOTAL', money(receipt.total_due), 'a4-bill-total')}
      </div>
      <div class="a4-settlement"><p class="a4-caption">AMOUNT PAID</p>
        <p class="a4-paid">${escapeHtml(money(receipt.amount_paid ?? receipt.amount))}</p>
        ${row('Remaining for this bill', money(receipt.bill_balance), 'a4-remaining')}
        <p class="a4-balance-note">${hasValue(receipt.bill_balance) && Number.isFinite(Number(receipt.bill_balance))
          ? (Number(receipt.bill_balance) <= 0 ? 'BILL PAID IN FULL' : 'PARTIAL PAYMENT / BALANCE REMAINS') : 'BILL BALANCE UNAVAILABLE'}</p>
        ${receipt.balance_as_of ? `<p class="a4-asof">Balance as of ${escapeHtml(formatDate(receipt.balance_as_of))}.</p>` : ''}
      </div>
    </section>
    <section class="a4-payment-meta">
      <p>Payment method: <strong>${escapeHtml(receipt.payment_method || '—')}</strong></p>
      <p>Cash received: <strong>${escapeHtml(money(receipt.cash_received))}</strong></p>
      <p>Change: <strong>${escapeHtml(money(receipt.change_amount ?? receipt.change))}</strong></p>
    </section>
    <div class="a4-bill-reference"><p>Bill reference: ${escapeHtml(receipt.reference_number || '—')}</p>
      <p>Cashier: ${escapeHtml(receipt.collector || receipt.payment_source || '—')}</p></div>
    <footer class="a4-footer"><p>Thank you for your payment. Please keep this receipt for your records.</p></footer>
  </article>`
  return `<div class="a4-receipt">${copy('Customer Copy')}<div class="a4-cut-line"><span>CUT ALONG LINE</span></div>${copy('Office Copy')}</div>`
}

// The preview and print window use exactly the same A4 layout.
export const receiptStyles = `
  .a4-receipt, .a4-receipt * { box-sizing: border-box; }
  .a4-receipt { width: 210mm; height: 297mm; padding: 10mm; background: #fff; color: #111; font: 9pt/1.2 Arial, sans-serif; position: relative; text-align: left; }
  .a4-receipt p, .a4-receipt h1, .a4-receipt h2, .a4-receipt h3, .a4-receipt h4 { margin: 0; color: inherit; }
  .a4-copy { height: 138.5mm; padding-bottom: 8mm; break-inside: avoid; page-break-inside: avoid; }
  .a4-copy:last-child { padding-top: 6mm; padding-bottom: 0; }
  .a4-header { display: flex; justify-content: space-between; gap: 6mm; padding-bottom: 3mm; border-bottom: 1pt solid #111; }
  .a4-brand { display: flex; align-items: flex-start; gap: 4mm; min-width: 0; }
  .a4-brand img { width: 17mm; height: 17mm; object-fit: contain; filter: grayscale(1); flex-shrink: 0; }
  .a4-brand h1 { font-size: 18pt; line-height: 1.1; font-weight: 800; }
  .a4-brand .a4-association { font-size: 10pt; margin: 1mm 0; color: #111; }
  .a4-brand p { font-size: 7.5pt; color: #555; }
  .a4-brand .a4-tin { font-weight: 700; color: #111; margin-top: 1mm; }
  .a4-document { text-align: right; flex-shrink: 0; }
  .a4-copy-label { font-size: 9pt; font-weight: 700; text-transform: uppercase; margin-bottom: 5mm !important; }
  .a4-document h2 { font-size: 12pt; font-weight: 700; }
  .a4-customer { display: grid; grid-template-columns: 1.1fr 1fr; gap: 5mm; margin: 3mm 0; font-size: 8pt; }
  .a4-customer > div { min-width: 0; overflow-wrap: anywhere; }
  .a4-caption { font-size: 7.5pt; font-weight: 700; color: #555 !important; }
  .a4-customer h3 { font-size: 13pt; font-weight: 700; margin: 1mm 0 1mm; }
  .a4-reference { text-align: right; }
  .a4-reference p + p { margin-top: 1mm; }
  .a4-address { margin-top: 1mm !important; color: #555 !important; }
  .a4-meter { display: grid; grid-template-columns: repeat(4, minmax(0, 1fr)); gap: 3mm; background: #eef0f2; padding: 3mm 4mm; border-top: .5pt solid #ddd; border-bottom: .5pt solid #ddd; }
  .a4-field { min-width: 0; overflow-wrap: anywhere; }
  .a4-field span { display: block; color: #555; font-size: 7pt; font-weight: 700; }
  .a4-field strong { display: block; margin-top: 1mm; font-size: 11pt; font-weight: 700; }
  .a4-payment { display: grid; grid-template-columns: 1fr 1fr; gap: 10mm; margin: 3mm 0; }
  .a4-charges h4 { font-size: 8pt; color: #555; font-weight: 700; margin: 0 0 2mm; }
  .a4-row { display: flex; justify-content: space-between; gap: 3mm; margin-top: 1.5mm; }
  .a4-row strong { text-align: right; font-weight: 400; font-variant-numeric: tabular-nums; }
  .a4-bill-total { border-top: .5pt solid #bbb; padding-top: 2mm; margin-top: 2mm; font-weight: 700; }
  .a4-bill-total strong { font-weight: 700; font-size: 11pt; }
  .a4-settlement { min-width: 0; border: 1pt solid #111; padding: 2.5mm 4mm; }
  .a4-settlement .a4-paid { font-size: 22pt; font-weight: 700; text-align: right; line-height: 1.2; margin: 1mm 0 2mm; overflow-wrap: anywhere; }
  .a4-remaining { border-top: .5pt solid #bbb; padding-top: 2mm; font-size: 8pt; }
  .a4-remaining strong { font-weight: 700; font-size: 10pt; }
  .a4-balance-note { font-size: 7pt; font-weight: 700; margin-top: 2mm !important; }
  .a4-asof { font-size: 7pt; color: #555 !important; margin-top: .5mm !important; }
  .a4-payment-meta { display: flex; justify-content: space-between; gap: 4mm; font-size: 8pt; }
  .a4-payment-meta strong { font-weight: 400; }
  .a4-bill-reference { display: flex; justify-content: space-between; gap: 6mm; font-size: 8pt; margin-top: 2mm; }
  .a4-bill-reference p { min-width: 0; overflow-wrap: anywhere; }
  .a4-bill-reference p:last-child { text-align: right; }
  .a4-footer { border-top: .5pt solid #bbb; padding-top: 2mm; margin-top: 2mm; font-size: 8pt; }
  .a4-cut-line { position: absolute; top: 148.5mm; left: 6mm; right: 6mm; border-top: .5pt dashed #777; text-align: center; height: 0; }
  .a4-cut-line span { display: inline-block; padding: 0 3mm; position: relative; top: -2mm; background: #fff; color: #555; font-size: 7pt; }
`

export function openReceiptWindow() {
  const popup = window.open('', '_blank', 'width=960,height=900')
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
    <title>Payment receipt ${escapeHtml(receipt.id || receipt.reference_number)}</title>
    <style>${receiptStyles}
      @page { size: A4 portrait; margin: 0; }
      body { margin: 0; background: #edf3f2; }
      .print-tools { max-width: 750px; margin: 20px auto; padding: 0 20px; font: 13px/1.6 Arial, sans-serif; color: #244b46; }
      .print-tools button { padding: 10px 18px; border: 0; border-radius: 8px; background: #0f766e; color: #fff; cursor: pointer; font-weight: 700; }
      .receipt-paper { width: 210mm; margin: 0 auto 24px; background: #fff; box-shadow: 0 8px 30px #173c3920; }
      @media print { html, body { width: 210mm; margin: 0; background: #fff; } .print-tools { display: none; } .receipt-paper { margin: 0; box-shadow: none; } }
    </style></head><body>
    <aside class="print-tools"><button id="print-receipt" type="button">Print receipt</button>
    <p>Select your regular printer · A4 · portrait · 100% scale · no margins.<br>Turn off browser headers and footers. Cut across the center line for the Customer and Office copies.</p><p id="print-status" role="status">Loading logo…</p></aside>
    <main class="receipt-paper">${receiptMarkup(receipt, customerName)}</main></body></html>`)
  popup.document.close()
  const print = () => { popup.focus(); popup.print() }
  const button = popup.document.getElementById('print-receipt')
  button.disabled = true
  try {
    await Promise.all([...popup.document.images].map(img => img.decode()))
    await popup.document.fonts.ready
    if (popup.closed) return
    popup.document.getElementById('print-status').textContent = 'A4 receipt ready. Customer and Office copies print on one sheet.'
    button.disabled = false
    button.addEventListener('click', print)
    print()
  } catch (error) {
    if (!popup.closed) popup.document.getElementById('print-status').textContent = 'The receipt logo could not load. Close this window and try again.'
    throw error
  }
}
