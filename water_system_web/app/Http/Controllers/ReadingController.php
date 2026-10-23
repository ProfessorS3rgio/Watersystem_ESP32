<?php

namespace App\Http\Controllers;

use App\Models\Bill;
use App\Models\Customer;
use App\Models\Reading;
use App\Models\Setting;
use Carbon\Carbon;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\DB;
use App\Services\AuditLogService;
use App\Services\BillingStatusService;

class ReadingController extends Controller
{
   public function indexByCustomer(Request $request, Customer $customer)
{
    app(BillingStatusService::class)->refreshDueBills($customer->customer_id);
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
            DB::raw('b.charges as charges'),
            DB::raw('b.penalty as penalty'),
            DB::raw('b.total_due as total_due'),
            DB::raw('b.amount_paid as amount_paid'),
            DB::raw('b.balance_due as balance_due'),
            DB::raw('b.status as status'),
        ]);

    return response()->json([
        'data' => $readings,
    ]);
}

  public function sync(Request $request, AuditLogService $audit)
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

            // Device epochs represent the RTC's Philippine calendar time.
            // Persist them in the same timezone so an Aug 30 device reading is
            // not stored/displayed as Aug 29 after a UTC conversion.
            $readingAt = !empty($row['reading_at'])
                ? Carbon::createFromTimestamp((int) $row['reading_at'], 'Asia/Manila')
                : now('Asia/Manila');
            
            // A device reading ID is only meaningful together with its device UID.
            // This makes retries idempotent even when timestamps differ.
            $existing = !empty($row['device_uid'])
                ? Reading::where('device_uid', $row['device_uid'])
                    ->where('device_reading_id', $row['reading_id'])
                    ->first()
                : Reading::where('customer_account_number', $row['customer_account_number'])
                    ->where('reading_at', $readingAt)
                    ->first();

            if ($existing) {
                $existing->update([
                    'customer_id' => $customer->customer_id, // Use WEB database customer_id
                    'device_uid' => $row['device_uid'] ?? null,
                    'device_reading_id' => $row['reading_id'],
                    'previous_reading' => (int)$row['previous_reading'],
                    'current_reading' => (int)$row['current_reading'],
                    'usage_m3' => (int)$row['usage_m3'],
                    // Date normalization resends the same device reading ID.
                    // Preserve that identity while accepting its corrected time.
                    'reading_at' => $readingAt,
                    'customer_account_number' => $row['customer_account_number'],
                    'Synced' => true,
                    'last_sync' => now(),
                ]);
                $updated++;
            } else {
                Reading::create([
                    'customer_id' => $customer->customer_id, // Use WEB database customer_id
                    'device_uid' => $row['device_uid'] ?? null,
                    'device_reading_id' => $row['reading_id'],
                    'previous_reading' => (int)$row['previous_reading'],
                    'current_reading' => (int)$row['current_reading'],
                    'usage_m3' => (int)$row['usage_m3'],
                    'reading_at' => $readingAt,
                    'customer_account_number' => $row['customer_account_number'],
                    'Synced' => true,
                    'last_sync' => now(),
                ]);
                $inserted++;
            }
        } catch (\Exception $e) {
            \Log::error('Failed to sync reading: ' . $e->getMessage());
            $skipped++;
        }
    }

    $audit->syncOperation('readings', count($rows), [
        'processed' => $inserted + $updated,
        'inserted' => $inserted,
        'updated' => $updated,
        'skipped' => $skipped,
    ]);

    return response()->json([
        'processed' => $inserted + $updated,
        'inserted' => $inserted,
        'updated' => $updated,
        'skipped' => $skipped,
    ]);
    }

    /** Compare a read-only device export with records already in the website. */
    public function inspect(Request $request)
    {
        $validated = $request->validate([
            // A device query with no matches is a valid diagnostic result.
            'readings' => ['present', 'array', 'max:2000'],
            'readings.*.reading_id' => ['required', 'integer', 'min:1'],
            'readings.*.device_uid' => ['nullable', 'string', 'max:255'],
            'readings.*.customer_account_number' => ['required', 'string', 'max:255'],
            'readings.*.previous_reading' => ['required', 'integer', 'min:0'],
            'readings.*.current_reading' => ['required', 'integer', 'min:0'],
            'readings.*.usage_m3' => ['required', 'integer', 'min:0'],
            'readings.*.reading_at' => ['nullable', 'integer', 'min:0'],
            'readings.*.synced' => ['nullable'],
        ]);

        $deviceRows = collect($validated['readings']);
        $accounts = $deviceRows->pluck('customer_account_number')->unique()->values();
        $webRows = Reading::whereIn('customer_account_number', $accounts)->get();
        $webByDeviceKey = $webRows->filter(fn ($row) => $row->device_uid && $row->device_reading_id)
            ->keyBy(fn ($row) => $row->device_uid . ':' . $row->device_reading_id);
        // Existing website rows created before device identity support remain
        // comparable by account and timestamp, but are not used for new upserts.
        $webByLegacyKey = $webRows->keyBy(fn ($row) => $row->customer_account_number . ':' . optional($row->reading_at)->timestamp);
        // A timestamp can differ in legacy imports because earlier firmware
        // exported created_at instead of reading_at. An unambiguous meter
        // transition is a safe fallback for those historical rows.
        $webByTransition = $webRows->groupBy(fn ($row) => implode(':', [
            $row->customer_account_number, $row->previous_reading, $row->current_reading, $row->usage_m3,
        ]));

        // Multiple readings within a month can be valid corrections or sequential
        // readings. Flag only a repeated *meter transition*, not a date grouping.
        $deviceTransitionCounts = $deviceRows->groupBy(fn ($row) => implode(':', [
            $row['customer_account_number'], $row['previous_reading'], $row['current_reading'], $row['usage_m3'],
        ]))->map->count();
        $webTransitionCounts = $webRows->groupBy(fn ($row) => implode(':', [
            $row->customer_account_number, $row->previous_reading, $row->current_reading, $row->usage_m3,
        ]))->map->count();

        $results = $deviceRows->map(function ($row) use ($webByDeviceKey, $webByLegacyKey, $webByTransition, $deviceTransitionCounts, $webTransitionCounts) {
            $key = ($row['device_uid'] ?? '') . ':' . $row['reading_id'];
            $legacyKey = $row['customer_account_number'] . ':' . (int) ($row['reading_at'] ?? 0);
            $transitionKey = implode(':', [$row['customer_account_number'], $row['previous_reading'], $row['current_reading'], $row['usage_m3']]);
            $transitionMatches = $webByTransition->get($transitionKey, collect());
            $legacyTransitionMatch = $transitionMatches->count() === 1 ? $transitionMatches->first() : null;
            $server = $webByDeviceKey->get($key)
                ?? $webByLegacyKey->get($legacyKey)
                ?? $legacyTransitionMatch;
            $duplicate = ($deviceTransitionCounts->get($transitionKey, 0) > 1) || ($webTransitionCounts->get($transitionKey, 0) > 1);
            $conflict = $server && ((int) $server->previous_reading !== (int) $row['previous_reading']
                || (int) $server->current_reading !== (int) $row['current_reading']
                || (int) $server->usage_m3 !== (int) $row['usage_m3']);
            $status = $conflict ? 'conflict' : ($duplicate ? 'duplicate' : ($server ? 'match' : 'missing_on_web'));

            return ['status' => $status, 'device' => $row, 'server' => $server];
        })->values();

        return response()->json([
            'results' => $results,
            'summary' => $results->countBy('status'),
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
