import assert from 'node:assert/strict'
import test from 'node:test'
import { billBalance, formatMoney } from '../resources/js/utils/billingPresentation.js'

test('checkout collects only the remaining balance of a partially paid bill', () => {
  assert.equal(billBalance({ total_due: '500.00', amount_paid: '200.00', balance_due: '300.00' }), 300)
})

test('a settled balance never falls back to the original bill total', () => {
  assert.equal(billBalance({ total_due: 500, balance_due: 0 }), 0)
})

test('legacy bills without balance fields retain their original amount', () => {
  assert.equal(billBalance({ total_due: '125.50' }), 125.5)
})

test('invalid amounts cannot produce a positive checkout balance', () => {
  for (const bill of [null, {}, { balance_due: -10 }, { balance_due: 'invalid' }]) {
    assert.equal(billBalance(bill), 0)
  }
})

test('displayed amounts include grouping and centavos', () => {
  assert.equal(formatMoney(1234.5), '1,234.50')
})
