<?php

namespace App\Services;

use App\Models\{Customer, CustomerType, Deduction};
use Illuminate\Support\Str;
use Illuminate\Validation\ValidationException;

class ImportBillingCalculator
{
    public function calculate(Customer $customer, int $usage): array
    {
        $type = CustomerType::find($customer->type_id);
        if (! $type) {
            throw ValidationException::withMessages(['file' => "Customer {$customer->account_no} has no valid customer type for bill calculation."]);
        }
        $rate = (float) $type->rate_per_m3;
        $isOfficer = Str::lower(trim((string) $type->type_name)) === 'officers';
        $gross = $usage <= (int) $type->min_m3 ? (float) $type->min_charge
            : ($isOfficer ? (float) $type->min_charge + $usage * $rate : $usage * $rate);
        $deduction = $customer->deduction_id ? Deduction::find($customer->deduction_id) : null;
        $discount = $deduction ? (Str::lower((string) $deduction->type) === 'percentage'
            ? $gross * ((float) $deduction->value / 100) : (float) $deduction->value) : 0;
        return ['rate' => $rate, 'charges' => round($gross, 2), 'total_due' => round(max(0, $gross - $discount), 2)];
    }
}
