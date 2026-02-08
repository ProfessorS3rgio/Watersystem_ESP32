<?php

namespace App\Http\Controllers;

use Illuminate\Http\Request;
use Illuminate\Support\Facades\DB;

class SyncController extends Controller
{
    /**
     * Reset sync status for all records in the web database.
     * Sets Synced = 0 and last_sync = null for all tables with sync columns.
     */
    public function resetStatus(Request $request)
    {
        $tables = [
            'customer' => 'customer_id',
            'reading' => 'reading_id',
            'bill' => 'bill_id',
            'customer_type' => 'type_id',
            'deduction' => 'deduction_id',
            'barangay_sequence' => 'brgy_id',
            'settings' => 'id'
        ];

        $totalReset = 0;

        DB::transaction(function () use ($tables, &$totalReset) {
            foreach ($tables as $table => $primaryKey) {
                try {
                    $updated = DB::table($table)
                        ->update([
                            'Synced' => 0,
                            'last_sync' => null,
                            'updated_at' => now()
                        ]);
                    
                    $totalReset += $updated;
                } catch (\Exception $e) {
                    // Log the error but continue with other tables
                    \Log::error("Failed to reset sync status for table {$table}: " . $e->getMessage());
                }
            }
        });

        return response()->json([
            'message' => "Sync status reset successfully for {$totalReset} records",
            'total_reset' => $totalReset
        ]);
    }
}