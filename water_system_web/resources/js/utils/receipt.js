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
      <div class="a4-document"><p class="a4-copy-label">${label}</p><h2>PAYMENT RECEIPT</h2>
        <p class="compact-reference"><strong>RECEIPT NO. ${escapeHtml(receiptNumber)}</strong></p>
        <p>${escapeHtml(formatDate(date))} · ${escapeHtml(time)}</p></div>
    </header>
    <section class="a4-customer">
      <div><h3>${escapeHtml(customerName || receipt.customer_name || 'Customer')}</h3>
        <p>Account: ${escapeHtml(receipt.account_no || '—')}${receipt.barangay ? ` &nbsp; | &nbsp; Barangay: ${escapeHtml(receipt.barangay)}` : ''}${receipt.customer_type ? ` &nbsp; | &nbsp; ${escapeHtml(receipt.customer_type)}` : ''}</p>
        ${receipt.address ? `<p class="a4-address">${escapeHtml(receipt.address)}</p>` : ''}</div>
      <div class="a4-reference"><p>Billing period: ${escapeHtml(receipt.billing_period || formatDate(receipt.bill_date))}</p>
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
  const block = `<section class="receipt-block" aria-label="Customer and Office receipt set">${copy('Customer Copy')}<div class="a4-cut-line"><span>CUT COPIES APART</span></div>${copy('Office Copy')}</section>`
  return `<div class="bond-sheet">${block}<div class="lengthwise-cut" aria-label="Cut the long bond sheet lengthwise"><span>CUT LENGTHWISE</span></div>${block}</div>`
}

// Each half of a long bond sheet contains a complete 3.9 × 5.83 inch receipt set.
export const receiptStyles = `
  .bond-sheet, .bond-sheet * { box-sizing: border-box; }
  .bond-sheet { width: 8.5in; height: 13in; padding: .25in .175in; display: grid; grid-template-columns: 3.9in 3.9in; column-gap: .35in; align-content: start; background: #fff; color: #111; font: 6.5pt/1.15 Arial, sans-serif; position: relative; text-align: left; }
  .bond-sheet p, .bond-sheet h1, .bond-sheet h2, .bond-sheet h3, .bond-sheet h4 { margin: 0; color: inherit; }
  .receipt-block { width: 3.9in; height: 5.83in; padding: .1in; position: relative; break-inside: avoid; page-break-inside: avoid; }
  .a4-copy { height: 2.815in; padding-bottom: 1.5mm; }
  .a4-copy:last-child { padding-top: 1.5mm; padding-bottom: 0; }
  .a4-header { display: flex; justify-content: space-between; gap: 1.5mm; padding-bottom: 1mm; border-bottom: .6pt solid #111; }
  .a4-brand { display: flex; align-items: flex-start; gap: 1.5mm; min-width: 0; }
  .a4-brand img { width: 9mm; height: 9mm; object-fit: contain; filter: grayscale(1); flex-shrink: 0; }
  .a4-brand h1 { font-size: 10pt; line-height: 1.1; font-weight: 800; }
  .a4-brand .a4-association { font-size: 6.5pt; margin: .3mm 0; color: #111; }
  .a4-brand p { font-size: 5.5pt; color: #555; }
  .a4-brand .a4-tin { font-weight: 700; color: #111; margin-top: .3mm; }
  .a4-document { text-align: right; flex-shrink: 0; }
  .a4-copy-label { font-size: 6.5pt; font-weight: 700; text-transform: uppercase; margin-bottom: 1mm !important; }
  .a4-document h2 { font-size: 7pt; font-weight: 700; }
  .a4-document > p:not(.a4-copy-label) { font-size: 6pt; margin-top: .4mm; }
  .a4-document .compact-reference { margin-top: 1mm !important; }
  .a4-customer { margin: 1mm 0; font-size: 6pt; }
  .a4-customer > div { min-width: 0; overflow-wrap: anywhere; }
  .a4-caption { font-size: 6pt; font-weight: 700; color: #555 !important; }
  .a4-customer h3 { font-size: 8pt; font-weight: 700; margin: .3mm 0; }
  .a4-reference { display: flex; flex-wrap: wrap; justify-content: space-between; gap: .3mm 1mm; margin-top: .4mm; }
  .a4-address { margin-top: .3mm !important; color: #555 !important; }
  .a4-meter { display: grid; grid-template-columns: repeat(4, minmax(0, 1fr)); gap: 1mm; background: #eef0f2; padding: 1mm; border-top: .4pt solid #ddd; border-bottom: .4pt solid #ddd; }
  .a4-field { min-width: 0; overflow-wrap: anywhere; }
  .a4-field span { display: block; color: #555; font-size: 5.5pt; font-weight: 700; }
  .a4-field strong { display: block; margin-top: .3mm; font-size: 7pt; font-weight: 700; }
  .a4-payment { display: grid; grid-template-columns: 1fr 1fr; gap: 3mm; margin: 1mm 0; }
  .a4-charges h4 { font-size: 6pt; color: #555; font-weight: 700; margin: 0 0 .5mm; }
  .a4-row { display: flex; justify-content: space-between; gap: 1mm; margin-top: .5mm; }
  .a4-row strong { text-align: right; font-weight: 400; font-variant-numeric: tabular-nums; white-space: nowrap; }
  .a4-bill-total { border-top: .4pt solid #bbb; padding-top: .6mm; margin-top: .6mm; font-weight: 700; }
  .a4-bill-total strong { font-weight: 700; font-size: 7pt; }
  .a4-settlement { min-width: 0; border: .6pt solid #111; padding: 1mm 1.5mm; }
  .a4-settlement .a4-paid { font-size: 12pt; font-weight: 700; text-align: right; line-height: 1.1; margin: .5mm 0; overflow-wrap: anywhere; }
  .a4-remaining { border-top: .4pt solid #bbb; padding-top: .5mm; font-size: 6pt; }
  .a4-remaining strong { font-weight: 700; font-size: 6.5pt; }
  .a4-balance-note { font-size: 5.5pt; font-weight: 700; margin-top: .5mm !important; }
  .a4-asof { font-size: 5.5pt; color: #555 !important; margin-top: .3mm !important; }
  .a4-payment-meta { display: flex; flex-wrap: wrap; justify-content: space-between; gap: .5mm 1mm; font-size: 6pt; }
  .a4-payment-meta strong { font-weight: 400; }
  .a4-bill-reference { display: flex; justify-content: space-between; gap: 2mm; font-size: 6pt; margin-top: .7mm; }
  .a4-bill-reference p { min-width: 0; overflow-wrap: anywhere; }
  .a4-bill-reference p:last-child { text-align: right; }
  .a4-footer { border-top: .4pt solid #bbb; padding-top: .7mm; margin-top: .7mm; font-size: 5.5pt; }
  .a4-cut-line { position: absolute; top: 50%; left: 1mm; right: 1mm; border-top: .4pt dashed #777; text-align: center; height: 0; }
  .a4-cut-line span { display: inline-block; padding: 0 1mm; position: relative; top: -1.3mm; background: #fff; color: #555; font-size: 5pt; }
  .lengthwise-cut { position: absolute; left: 50%; top: .15in; bottom: .15in; border-left: .5pt dashed #777; width: 0; }
  .lengthwise-cut span { position: absolute; top: 50%; left: 0; transform: translate(-50%, -50%) rotate(-90deg); background: #fff; padding: 0 2mm; white-space: nowrap; font-size: 6pt; color: #555; }
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
      @page { size: 8.5in 13in; margin: 0; }
      body { margin: 0; background: #edf3f2; }
      .print-tools { max-width: 750px; margin: 20px auto; padding: 0 20px; font: 13px/1.6 Arial, sans-serif; color: #244b46; }
      .print-tools button { padding: 10px 18px; border: 0; border-radius: 8px; background: #0f766e; color: #fff; cursor: pointer; font-weight: 700; }
      .receipt-paper { width: 8.5in; margin: 0 auto 24px; background: #fff; box-shadow: 0 8px 30px #173c3920; }
      @media print { html, body { width: 8.5in; margin: 0; background: #fff; } .print-tools { display: none; } .receipt-paper { margin: 0; box-shadow: none; } }
    </style></head><body>
    <aside class="print-tools"><button id="print-receipt" type="button">Print receipt</button>
    <p>Select 8.5 × 13-inch long bond paper · portrait · 100% scale · no margins. Turn off browser headers and footers.<br>Cut the sheet lengthwise. Each half contains the same transaction in a 3.9 × 5.83-inch block, including Customer and Office copies. This prints two identical receipt sets.</p><p id="print-status" role="status">Loading logo…</p></aside>
    <main class="receipt-paper">${receiptMarkup(receipt, customerName)}</main></body></html>`)
  popup.document.close()
  const print = () => { popup.focus(); popup.print() }
  const button = popup.document.getElementById('print-receipt')
  button.disabled = true
  try {
    await Promise.all([...popup.document.images].map(img => img.decode()))
    await popup.document.fonts.ready
    if (popup.closed) return
    popup.document.getElementById('print-status').textContent = 'Long bond receipt ready. Both cut halves retain Customer and Office copies.'
    button.disabled = false
    button.addEventListener('click', print)
    print()
  } catch (error) {
    if (!popup.closed) popup.document.getElementById('print-status').textContent = 'The receipt logo could not load. Close this window and try again.'
    throw error
  }
}
