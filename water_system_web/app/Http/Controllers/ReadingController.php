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
                'reading.source',
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
            'readings.*.account_no' => ['required', 'string', 'max:64'],
            'readings.*.previous_reading' => ['required', 'integer', 'min:0'],
            'readings.*.current_reading' => ['required', 'integer', 'min:0'],
            'readings.*.usage_m3' => ['required', 'integer', 'min:0'],
            'readings.*.reading_at' => ['nullable', 'integer', 'min:0'], // epoch seconds
        ]);

        $rows = $validated['readings'];
        $accountNos = collect($rows)->pluck('account_no')->unique()->values();

        $customersByAccount = Customer::query()
            ->whereIn('account_no', $accountNos)
            ->get(['id', 'account_no'])
            ->keyBy('account_no');

        $settings = Setting::query()->orderBy('id')->first();
        $ratePerM3 = $settings ? (float) $settings->rate_per_m3 : (float) env('WATER_RATE_PER_M3', 15.00);
        $dueDays = $settings ? (int) $settings->bill_due_days : (int) env('BILL_DUE_DAYS', 15);

        $inserted = 0;
        $updated = 0;
        $skipped = 0;

        DB::transaction(function () use ($rows, $customersByAccount, $ratePerM3, $dueDays, &$inserted, &$updated, &$skipped) {
            foreach ($rows as $row) {
                $customer = $customersByAccount->get($row['account_no']);
                if (!$customer) {
                    $skipped++;
                    continue;
                }

                $readingAt = array_key_exists('reading_at', $row) && (int) $row['reading_at'] > 0
                    ? Carbon::createFromTimestamp((int) $row['reading_at'])
                    : now();

                $reading = Reading::updateOrCreate(
                    [
                        'customer_id' => $customer->id,
                        'reading_at' => $readingAt,
                    ],
                    [
                        'previous_reading' => (int) $row['previous_reading'],
                        'current_reading' => (int) $row['current_reading'],
                        'usage_m3' => (int) $row['usage_m3'],
                        'read_by_user_id' => null,
                        'source' => 'device',
                    ]
                );

                if ($reading->wasRecentlyCreated) {
                    $inserted++;
                } else {
                    $updated++;
                }

                // Auto-create/update bill for this reading.
                // One bill per reading event.
                $charges = (float) ((int) $reading->usage_m3) * $ratePerM3;
                $penalty = 0.0;
                $totalDue = $charges + $penalty;

                $bill = Bill::query()->where('reading_id', $reading->id)->first();
                if (!$bill) {
                    $billDate = Carbon::parse($reading->reading_at)->toDateString();
                    $billNo = 'BILL-' . Carbon::parse($reading->reading_at)->format('Ymd') . '-' . $reading->id;

                    Bill::create([
                        'customer_id' => $customer->id,
                        'reading_id' => $reading->id,
                        'bill_no' => $billNo,
                        'bill_date' => $billDate,
                        'rate_per_m3' => $ratePerM3,
                        'charges' => $charges,
                        'penalty' => $penalty,
                        'total_due' => $totalDue,
                        'due_date' => Carbon::parse($billDate)->addDays($dueDays)->toDateString(),
                        'status' => 'pending',
                    ]);
                } else {
                    // Keep bill_no stable; don't override paid bills.
                    if (strtolower((string) $bill->status) !== 'paid') {
                        $bill->rate_per_m3 = $ratePerM3;
                        $bill->charges = $charges;
                        $bill->penalty = $penalty;
                        $bill->total_due = $totalDue;
                        if (!$bill->status) {
                            $bill->status = 'pending';
                        }
                        $bill->save();
                    }
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
