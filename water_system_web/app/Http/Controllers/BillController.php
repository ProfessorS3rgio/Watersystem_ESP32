<?php

namespace App\Http\Controllers;

use App\Models\Bill;
use App\Models\Setting;
use Carbon\Carbon;
use Illuminate\Http\Request;

class BillController extends Controller
{
    public function markPaid(Request $request, Bill $bill)
    {
        $validated = $request->validate([
            'status' => ['nullable', 'string', 'in:paid'],
        ]);

        // Apply penalty at payment time (if overdue beyond grace days).
        $settings = Setting::query()->orderBy('id')->first();
        $penaltyAfterDays = $settings ? (int) $settings->penalty_after_days : (int) env('PENALTY_AFTER_DAYS', 0);
        $penaltyAmount = $settings ? (float) $settings->penalty_amount : (float) env('PENALTY_AMOUNT', 0.00);

        $penalty = 0.0;
        if ($bill->due_date) {
            $threshold = Carbon::parse($bill->due_date)->addDays($penaltyAfterDays);
            if (now()->greaterThan($threshold) && $penaltyAmount > 0) {
                $penalty = $penaltyAmount;
            }
        }

        $bill->penalty = $penalty;
        $bill->total_due = (float) $bill->charges + (float) $bill->penalty;

        $bill->status = 'paid';
        $bill->save();

        return response()->json([
            'data' => $bill,
        ]);
    }
}
