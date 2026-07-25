<?php

namespace App\Http\Controllers;

use App\Models\Bill;
use App\Models\Customer;
use App\Models\Reading;
use App\Models\Setting;
use Carbon\Carbon;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\DB;

class ReadingController extends Controller
{
   public function indexByCustomer(Request $request, Customer $customer)
{
    $validated = $request->validate([
        'limit' => ['nullable', 'integer', 'min:1', 'max:200'],
    ]);

    $limit = array_key_exists('limit', $validated) && $validated['limit'] !== null
        ? (int) $validated['limit']
        : 50;

    $readings = DB::table('reading')
        ->leftJoin('bill as b', function($join) use ($customer) {
            $join->on('b.reading_id', '=', 'reading.reading_id')
                 ->where('b.customer_id', '=', $customer->customer_id);
        })
        ->where('reading.customer_id', $customer->customer_id)
        ->orderByDesc('reading.reading_at')
        ->limit($limit)
        ->get([
            DB::raw('reading.reading_id as id'),
            'reading.customer_id',
            'reading.previous_reading',
            'reading.current_reading',
            'reading.usage_m3',
            'reading.reading_at',
            'reading.read_by_user_id',
            'reading.created_at',
            'reading.updated_at',
            DB::raw('b.bill_id as bill_id'),
            DB::raw('b.reference_number as reference_number'),
            DB::raw('b.bill_date as bill_date'),
            DB::raw('b.due_date as due_date'),
            DB::raw('b.total_due as total_due'),
            DB::raw('b.status as status'),
        ]);

    return response()->json([
        'data' => $readings,
    ]);
}

  public function sync(Request $request)
{
    $validated = $request->validate([
        'readings' => ['required', 'array', 'max:2000'],
        'readings.*.reading_id' => ['required', 'integer', 'min:1'],
        'readings.*.customer_id' => ['nullable', 'integer'], // Make nullable, we don't trust it
        'readings.*.device_uid' => ['nullable', 'string'],
        'readings.*.previous_reading' => ['required', 'integer', 'min:0'],
        'readings.*.current_reading' => ['required', 'integer', 'min:0'],
        'readings.*.usage_m3' => ['required', 'integer', 'min:0'],
        'readings.*.customer_account_number' => ['required', 'string', 'max:255'],
        'readings.*.reading_at' => ['nullable', 'integer', 'min:0'],
    ]);

    $rows = $validated['readings'];
    
    // FIX: Look up customers by ACCOUNT NUMBER, not customer_id from device!
    $accountNumbers = collect($rows)->pluck('customer_account_number')->unique()->values();
    $customersByAccount = Customer::whereIn('account_no', $accountNumbers)
        ->get()
        ->keyBy('account_no');

    $inserted = 0;
    $updated = 0;
    $skipped = 0;

    foreach ($rows as $row) {
        try {
            // FIX: Find customer by account_number, NOT by customer_id!
            $customer = $customersByAccount->get($row['customer_account_number']);
            
            if (!$customer) {
                \Log::warning('Customer not found for account', [
                    'account_no' => $row['customer_account_number']
                ]);
                $skipped++;
                continue;
            }

            $readingAt = !empty($row['reading_at']) ? Carbon::createFromTimestamp((int)$row['reading_at']) : now();
            
            $existing = Reading::where('customer_account_number', $row['customer_account_number'])
                ->where('reading_at', $readingAt)
                ->first();

            if ($existing) {
                $existing->update([
                    'customer_id' => $customer->customer_id, // Use WEB database customer_id
                    'device_uid' => $row['device_uid'] ?? null,
                    'previous_reading' => (int)$row['previous_reading'],
                    'current_reading' => (int)$row['current_reading'],
                    'usage_m3' => (int)$row['usage_m3'],
                    'customer_account_number' => $row['customer_account_number'],
                ]);
                $updated++;
            } else {
                Reading::create([
                    'customer_id' => $customer->customer_id, // Use WEB database customer_id
                    'device_uid' => $row['device_uid'] ?? null,
                    'previous_reading' => (int)$row['previous_reading'],
                    'current_reading' => (int)$row['current_reading'],
                    'usage_m3' => (int)$row['usage_m3'],
                    'reading_at' => $readingAt,
                    'customer_account_number' => $row['customer_account_number'],
                ]);
                $inserted++;
            }
        } catch (\Exception $e) {
            \Log::error('Failed to sync reading: ' . $e->getMessage());
            $skipped++;
        }
    }

    return response()->json([
        'processed' => $inserted + $updated,
        'inserted' => $inserted,
        'updated' => $updated,
        'skipped' => $skipped,
    ]);
}
    /**
     * Mark readings as synced after successful device sync.
     */
    public function markSynced(Request $request)
    {
        $validated = $request->validate([
            'reading_ids' => ['required', 'array'],
            'reading_ids.*' => ['required', 'integer'],
        ]);

        $updated = Reading::whereIn('reading_id', $validated['reading_ids'])
            ->update([
                'Synced' => true,
                'last_sync' => now(),
            ]);

        return response()->json([
            'updated' => $updated,
            'message' => "{$updated} readings marked as synced",
        ]);
    }
}
