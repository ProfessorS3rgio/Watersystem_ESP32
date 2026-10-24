<?php

namespace App\Http\Controllers;

use App\Models\Setting;
use Illuminate\Http\Request;
use App\Services\AuditLogService;
use Illuminate\Support\Facades\DB;

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
            'penalty_after_days' => (int) env('PENALTY_AFTER_DAYS', 0),
            'penalty_amount' => (float) env('PENALTY_AMOUNT', 0),
            'penalty_enabled' => true,
        ]);
    }

    public function show(Request $request)
    {
        $row = $this->ensureSettingsRow();

        return response()->json([
            'data' => $row,
        ]);
    }

    public function update(Request $request, AuditLogService $audit)
    {
        $row = $this->ensureSettingsRow();

        $validated = $request->validate([
            'bill_due_days' => ['required', 'integer', 'min:0', 'max:365'],
            'disconnection_days' => ['required', 'integer', 'min:0', 'max:365'],
            'penalty_after_days' => ['required', 'integer', 'min:0', 'max:365'],
            'penalty_amount' => ['required', 'numeric', 'min:0', 'max:1000000'],
            'penalty_enabled' => ['required', 'boolean'],
        ]);

        return DB::transaction(function () use ($row, $validated, $audit) {
            $before = $row->getAttributes();
            $row->fill($validated);
            $row->Synced = false;
            $row->save();
            $audit->record('settings_updated', $row, $before, $row->getAttributes());

            return response()->json([
                'data' => $row,
            ]);
        });
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
