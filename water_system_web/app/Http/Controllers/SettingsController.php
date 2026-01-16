<?php

namespace App\Http\Controllers;

use App\Models\Setting;
use Illuminate\Http\Request;

class SettingsController extends Controller
{
    private function ensureSettingsRow(): Setting
    {
        $row = Setting::query()->orderBy('id')->first();
        if ($row) {
            return $row;
        }

        return Setting::create([
            'rate_per_m3' => (float) env('WATER_RATE_PER_M3', 15.00),
            'bill_due_days' => (int) env('BILL_DUE_DAYS', 15),
            'penalty_after_days' => (int) env('PENALTY_AFTER_DAYS', 0),
            'penalty_amount' => (float) env('PENALTY_AMOUNT', 0.00),
        ]);
    }

    public function show(Request $request)
    {
        $row = $this->ensureSettingsRow();

        return response()->json([
            'data' => $row,
        ]);
    }

    public function update(Request $request)
    {
        $row = $this->ensureSettingsRow();

        $validated = $request->validate([
            'rate_per_m3' => ['required', 'numeric', 'min:0', 'max:100000'],
            'bill_due_days' => ['required', 'integer', 'min:0', 'max:365'],
            'penalty_after_days' => ['required', 'integer', 'min:0', 'max:365'],
            'penalty_amount' => ['required', 'numeric', 'min:0', 'max:100000'],
        ]);

        $row->fill($validated);
        $row->save();

        return response()->json([
            'data' => $row,
        ]);
    }
}
