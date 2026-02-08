<?php

namespace App\Http\Controllers;

use App\Models\BarangaySequence;
use Illuminate\Http\Request;

class BarangaySequenceController extends Controller
{
    public function index()
    {
        return response()->json([
            'data' => BarangaySequence::all()
        ]);
    }

    /**
     * Mark barangay sequences as synced after successful device sync.
     */
    public function markSynced(Request $request)
    {
        $validated = $request->validate([
            'brgy_ids' => ['required', 'array'],
            'brgy_ids.*' => ['required', 'integer'],
        ]);

        $updated = BarangaySequence::whereIn('brgy_id', $validated['brgy_ids'])
            ->update([
                'Synced' => true,
                'last_sync' => now(),
            ]);

        return response()->json([
            'updated' => $updated,
            'message' => "{$updated} barangay sequences marked as synced",
        ]);
    }
}