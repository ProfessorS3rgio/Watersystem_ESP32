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
            'bill_due_days' => (int) env('BILL_DUE_DAYS', 5),
            'disconnection_days' => (int) env('DISCONNECTION_DAYS', 8),
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
            'bill_due_days' => ['required', 'integer', 'min:0', 'max:365'],
            'disconnection_days' => ['required', 'integer', 'min:0', 'max:365'],
        ]);

        $row->fill($validated);
        $row->save();

        return response()->json([
            'data' => $row,
        ]);
    }

    /**
     * Mark settings as synced after successful device sync.
     */
    public function markSynced(Request $request)
    {
        $validated = $request->validate([
            'setting_ids' => ['required', 'array'],
            'setting_ids.*' => ['required', 'integer'],
        ]);

        $updated = Setting::whereIn('id', $validated['setting_ids'])
            ->update([
                'Synced' => true,
                'last_sync' => now(),
            ]);

        return response()->json([
            'updated' => $updated,
            'message' => "{$updated} settings marked as synced",
        ]);
    }
}
