<?php

namespace App\Services;

use App\Models\Bill;
use App\Models\CustomerType;
use App\Models\Deduction;
use Carbon\Carbon;

class BillingStatusService
{
    /** Apply the website's due-date and customer-type penalty rules once. */
    public function refreshDueBills(?int $customerId = null): int
    {
        $today = Carbon::today('Asia/Manila');
        $query = Bill::query()
            ->with('customer')
            ->whereNotIn('status', ['Paid', 'Void'])
            ->whereNotNull('due_date')
            ->whereDate('due_date', '<', $today);

        if ($customerId) {
            $query->where('customer_id', $customerId);
        }

        $updated = 0;
        foreach ($query->cursor() as $bill) {
            $customer = $bill->customer;
            if (!$customer) continue;

            $penalty = $this->penaltyFor($customer->type_id, $customer->deduction_id);
            $total = (float) $bill->charges + $penalty;
            $balance = max($total - (float) ($bill->amount_paid ?? 0), 0);

            if (strcasecmp((string) $bill->status, 'Due') !== 0
                || (float) $bill->penalty !== $penalty
                || (float) $bill->total_due !== $total
                || (float) $bill->balance_due !== $balance) {
                $bill->penalty = $penalty;
                $bill->total_due = $total;
                $bill->balance_due = $balance;
                $bill->status = $balance <= 0.01 ? 'Paid' : 'Due';
                $bill->save();
                $updated++;
            }
        }

        return $updated;
    }

    private function penaltyFor(?int $typeId, ?int $deductionId): float
    {
        $deduction = $deductionId ? Deduction::find($deductionId) : null;
        if ($deduction && str_contains(strtolower((string) $deduction->name), 'senior')) {
            return 20.00;
        }

        return max(0, (float) (CustomerType::find($typeId)?->penalty ?? 0));
    }
}
