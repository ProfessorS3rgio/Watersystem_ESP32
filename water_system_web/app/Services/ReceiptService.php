<?php

namespace App\Services;

use App\Models\Bill;
use App\Models\BillTransaction;
use App\Models\BarangaySequence;
use App\Models\CustomerType;

class ReceiptService
{
    public function build(Bill $bill, BillTransaction $payment): array
    {
        $bill->loadMissing(['customer', 'reading']);
        $payment->loadMissing('processedBy');
        $customer = $bill->customer;
        $reading = $bill->reading;
        $billDate = $bill->bill_date;

        return [
            'id' => $payment->bill_transaction_id,
            'reference_number' => $bill->reference_number,
            'customer_name' => $customer?->customer_name,
            'account_no' => $customer?->account_no ?? $bill->customer_account_number,
            'customer_type' => $customer?->type_id ? CustomerType::find($customer->type_id)?->type_name : null,
            'address' => $customer?->address,
            'barangay' => $customer?->brgy_id ? BarangaySequence::find($customer->brgy_id)?->barangay : null,
            // Match the field printer's month-start through bill-date convention.
            'billing_period' => $billDate
                ? $billDate->copy()->startOfMonth()->format('M j, Y') . ' – ' . $billDate->format('M j, Y')
                : null,
            'bill_date' => $billDate?->toDateString(),
            'due_date' => $bill->due_date?->toDateString(),
            'previous_reading' => $reading?->previous_reading,
            'current_reading' => $reading?->current_reading,
            'usage_m3' => $reading?->usage_m3,
            // Print recorded bill values, never recalculate using today's rates/discounts.
            'rate_per_m3' => $bill->rate_per_m3,
            'charges' => $bill->charges,
            'penalty' => $bill->penalty,
            'total_due' => $bill->total_due,
            'bill_balance' => $bill->balance_due,
            'balance_as_of' => now()->toISOString(),
            'amount_paid' => $payment->amount,
            'cash_received' => $payment->cash_received,
            'change' => $payment->change,
            'transaction_date' => $payment->transaction_date?->toISOString(),
            'payment_method' => $payment->payment_method,
            'collector' => $payment->processedBy?->name,
            'payment_source' => $payment->source,
            'contact' => config('receipt'),
        ];
    }
}
