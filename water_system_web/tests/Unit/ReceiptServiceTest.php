<?php

namespace Tests\Unit;

use App\Models\Bill;
use App\Models\BillTransaction;
use App\Models\Customer;
use App\Models\Reading;
use App\Models\User;
use App\Services\ReceiptService;
use Tests\TestCase;

class ReceiptServiceTest extends TestCase
{
    public function test_receipt_uses_recorded_bill_and_payment_values_without_recalculating_charges(): void
    {
        $bill = new Bill([
            'reference_number' => 'TEST-001', 'bill_date' => '2026-10-24',
            'due_date' => '2026-10-29', 'rate_per_m3' => 20, 'charges' => 240,
            'penalty' => 20, 'total_due' => 260, 'amount_paid' => 100, 'balance_due' => 160,
        ]);
        $bill->setRelation('customer', new Customer(['customer_name' => 'Sample Customer',
            'account_no' => 'M-001', 'address' => 'Purok 2']));
        $bill->setRelation('reading', new Reading(['previous_reading' => 0, 'current_reading' => 12, 'usage_m3' => 12]));
        $payment = new BillTransaction(['amount' => 100, 'cash_received' => 100, 'change' => 0,
            'transaction_date' => '2026-10-24 06:00:00', 'payment_method' => 'cash', 'source' => 'Office']);
        $payment->bill_transaction_id = 27;
        $payment->setRelation('processedBy', new User(['name' => 'Sample Collector']));

        $receipt = app(ReceiptService::class)->build($bill, $payment);

        $this->assertSame(27, $receipt['id']);
        $this->assertSame('100.00', $receipt['amount_paid']);
        $this->assertSame('260.00', $receipt['total_due']);
        $this->assertSame('160.00', $receipt['bill_balance']);
        $this->assertSame('0.00', $receipt['change']);
        $this->assertSame(0, $receipt['previous_reading']);
        $this->assertSame('Oct 1, 2026 – Oct 24, 2026', $receipt['billing_period']);
        $this->assertSame('Sample Collector', $receipt['collector']);
        $this->assertSame(config('receipt'), $receipt['contact']);
    }

    public function test_missing_historical_details_are_not_replaced_with_current_customer_readings(): void
    {
        $bill = new Bill(['reference_number' => 'OLD', 'customer_account_number' => 'OLD-1']);
        $bill->setRelation('customer', null);
        $bill->setRelation('reading', null);
        $payment = new BillTransaction(['amount' => 50]);
        $payment->setRelation('processedBy', null);

        $receipt = app(ReceiptService::class)->build($bill, $payment);

        $this->assertSame('OLD-1', $receipt['account_no']);
        $this->assertNull($receipt['previous_reading']);
        $this->assertNull($receipt['billing_period']);
        $this->assertNull($receipt['cash_received']);
        $this->assertNull($receipt['collector']);
    }
}
