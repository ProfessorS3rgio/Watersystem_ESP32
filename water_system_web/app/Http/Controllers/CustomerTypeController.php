<?php

namespace App\Http\Controllers;

use App\Models\CustomerType;
use Illuminate\Http\Request;

class CustomerTypeController extends Controller
{
    public function index()
    {
        return response()->json([
            'data' => CustomerType::all()
        ]);
    }

    public function update(Request $request, CustomerType $customerType)
    {
        $validated = $request->validate([
            'rate_per_m3' => ['required', 'numeric', 'min:0', 'max:1000000'],
            'min_m3' => ['required', 'integer', 'min:0', 'max:1000000'],
            'min_charge' => ['required', 'numeric', 'min:0', 'max:1000000'],
            'penalty' => ['required', 'numeric', 'min:0', 'max:1000000'],
        ]);

        $customerType->fill($validated);
        $customerType->Synced = false;
        $customerType->save();

        return response()->json(['data' => $customerType]);
    }

    /**
     * Mark customer types as synced after successful device sync.
     */
    public function markSynced(Request $request)
    {
        $validated = $request->validate([
            'customer_type_ids' => ['required', 'array'],
            'customer_type_ids.*' => ['required', 'integer'],
        ]);

        $updated = CustomerType::whereIn('type_id', $validated['customer_type_ids'])
            ->update([
                'Synced' => true,
                'last_sync' => now(),
            ]);

        return response()->json([
            'updated' => $updated,
            'message' => "{$updated} customer types marked as synced",
        ]);
    }
}
