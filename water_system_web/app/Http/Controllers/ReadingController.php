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
            ->leftJoin('bill as b', 'b.reading_id', '=', 'reading.id')
            ->where('reading.customer_id', $customer->id)
            ->orderByDesc('reading.reading_at')
            ->limit($limit)
            ->get([
                'reading.id',
                'reading.customer_id',
                'reading.previous_reading',
                'reading.current_reading',
                'reading.usage_m3',
                'reading.reading_at',
                'reading.read_by_user_id',
                'reading.created_at',
                'reading.updated_at',
                DB::raw('b.id as bill_id'),
                DB::raw('b.bill_no as bill_no'),
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
            'readings.*.customer_id' => ['required', 'integer', 'min:1'],
            'readings.*.device_uid' => ['nullable', 'string'],
            'readings.*.previous_reading' => ['required', 'integer', 'min:0'],
            'readings.*.current_reading' => ['required', 'integer', 'min:0'],
            'readings.*.usage_m3' => ['required', 'integer', 'min:0'],
            'readings.*.reading_at' => ['nullable', 'integer', 'min:0'], // epoch seconds
        ]);

        $rows = $validated['readings'];
        $customerIds = collect($rows)->pluck('customer_id')->unique()->values();

        $customersById = Customer::query()
            ->whereIn('customer_id', $customerIds)
            ->get(['customer_id', 'account_no'])
            ->keyBy('customer_id');

        $settings = Setting::query()->orderBy('id')->first();
        $ratePerM3 = $settings ? (float) $settings->rate_per_m3 : (float) env('WATER_RATE_PER_M3', 15.00);
        $dueDays = $settings ? (int) $settings->bill_due_days : (int) env('BILL_DUE_DAYS', 15);

        $inserted = 0;
        $updated = 0;
        $skipped = 0;

        DB::transaction(function () use ($rows, $customersById, $ratePerM3, $dueDays, &$inserted, &$updated, &$skipped) {
            foreach ($rows as $row) {
                $customer = $customersById->get($row['customer_id']);
                if (!$customer) {
                    $skipped++;
                    continue;
                }

                $readingAt = array_key_exists('reading_at', $row) && (int) $row['reading_at'] > 0
                    ? Carbon::createFromTimestamp((int) $row['reading_at'])
                    : now();

                // Check if reading exists
                $existing = Reading::where('reading_id', (int) $row['reading_id'])->first();
                if ($existing) {
                    $existing->update([
                        'customer_id' => $customer->customer_id,
                        'device_uid' => $row['device_uid'] ?? null,
                        'previous_reading' => (int) $row['previous_reading'],
                        'current_reading' => (int) $row['current_reading'],
                        'usage_m3' => (int) $row['usage_m3'],
                        'reading_at' => $readingAt,
                        'read_by_user_id' => null,
                    ]);
                    $updated++;
                } else {
                    DB::insert("INSERT INTO reading (reading_id, customer_id, device_uid, previous_reading, current_reading, usage_m3, reading_at, read_by_user_id, created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", [
                        (int) $row['reading_id'],
                        $customer->customer_id,
                        $row['device_uid'] ?? null,
                        (int) $row['previous_reading'],
                        (int) $row['current_reading'],
                        (int) $row['usage_m3'],
                        $readingAt,
                        null,
                        now(),
                        now(),
                    ]);
                    $inserted++;
                }
            }
        });

        return response()->json([
            'processed' => $inserted + $updated,
            'inserted' => $inserted,
            'updated' => $updated,
            'skipped' => $skipped,
        ]);
    }
}
