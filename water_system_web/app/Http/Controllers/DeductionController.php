<?php

namespace App\Http\Controllers;

use App\Models\Deduction;
use Illuminate\Http\Request;

class DeductionController extends Controller
{
    public function index()
    {
        return response()->json([
            'data' => Deduction::all()
        ]);
    }

    /**
     * Mark deductions as synced after successful device sync.
     */
    public function markSynced(Request $request)
    {
        $validated = $request->validate([
            'deduction_ids' => ['required', 'array'],
            'deduction_ids.*' => ['required', 'integer'],
        ]);

        $updated = Deduction::whereIn('deduction_id', $validated['deduction_ids'])
            ->update([
                'Synced' => true,
                'last_sync' => now(),
            ]);

        return response()->json([
            'updated' => $updated,
            'message' => "{$updated} deductions marked as synced",
        ]);
    }
}