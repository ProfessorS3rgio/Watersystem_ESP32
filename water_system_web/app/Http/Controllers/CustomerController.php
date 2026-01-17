<?php

namespace App\Http\Controllers;

use App\Models\Customer;
use App\Models\BarangaySequence;
use Carbon\Carbon;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\DB;

class CustomerController extends Controller
{
    /**
     * Display a listing of the resource.
     */
    public function index(Request $request)
    {
        $accountNo = $request->query('account_no');
        $name = $request->query('name');

        $query = DB::table('customer');

        if ($accountNo) {
            $query->where('account_no', $accountNo);
        }

        if ($name) {
            $query->where('customer_name', 'like', '%' . $name . '%');
        }

        $latestReadingAt = DB::table('reading')
            ->select('customer_id', DB::raw('MAX(reading_at) as latest_reading_at'))
            ->groupBy('customer_id');

        $latestBillDate = DB::table('bill')
            ->select('customer_id', DB::raw('MAX(bill_date) as latest_bill_date'))
            ->groupBy('customer_id');

        $customers = $query
            ->leftJoinSub($latestReadingAt, 'lr', function ($join) {
                $join->on('lr.customer_id', '=', 'customer.id');
            })
            ->leftJoin('reading as r', function ($join) {
                $join->on('r.customer_id', '=', 'customer.id')
                    ->on('r.reading_at', '=', 'lr.latest_reading_at');
            })
            ->leftJoinSub($latestBillDate, 'lb', function ($join) {
                $join->on('lb.customer_id', '=', 'customer.id');
            })
            ->leftJoin('bill as b', function ($join) {
                $join->on('b.customer_id', '=', 'customer.id')
                    ->on('b.bill_date', '=', 'lb.latest_bill_date');
            })
            ->orderBy('customer.customer_name')
            ->get([
                'customer.id',
                'customer.account_no',
                'customer.customer_name',
                'customer.type_id',
                'customer.brgy_id',
                'customer.address',
                'customer.previous_reading',
                'customer.status',
                'customer.created_at',
                'customer.updated_at',
                DB::raw('r.current_reading as current_reading'),
                DB::raw('r.usage_m3 as last_usage_m3'),
                DB::raw('r.reading_at as last_reading_at'),
                DB::raw('b.id as latest_bill_id'),
                DB::raw('b.bill_no as latest_bill_no'),
                DB::raw('b.bill_date as latest_bill_date'),
                DB::raw('b.due_date as latest_bill_due_date'),
                DB::raw('b.total_due as latest_bill_total_due'),
                DB::raw('b.status as latest_bill_status'),
            ]);

        $today = Carbon::today();
        $customers = $customers->map(function ($c) use ($today) {
            $status = strtolower((string) ($c->latest_bill_status ?? ''));

            $state = 'none';
            if ($c->latest_bill_id) {
                if ($status === 'paid') {
                    $state = 'paid';
                } else {
                    $dueDate = $c->latest_bill_due_date ? Carbon::parse($c->latest_bill_due_date) : null;
                    $state = ($dueDate && $dueDate->lessThan($today)) ? 'due' : 'pending';
                }
            }

            $c->latest_bill_state = $state;
            return $c;
        });

        return response()->json([
            'data' => $customers,
        ]);
    }

    /**
     * Show the form for creating a new resource.
     */
    public function create()
    {
        //
    }

    /**
     * Store a newly created resource in storage.
     */
    public function store(Request $request)
    {
        $validated = $request->validate([
            'customer_name' => ['required', 'string', 'max:255'],
            'type_id' => ['required', 'integer', 'exists:customer_type,type_id'],
            'brgy_id' => ['required', 'integer', 'exists:barangay_sequence,brgy_id'],
            'address' => ['required', 'string', 'max:255'],
            'previous_reading' => ['nullable', 'integer', 'min:0'],
            'status' => ['required', 'in:active,disconnected'],
            'benefits' => ['nullable', 'array'],
            'benefits.*' => ['integer', 'exists:deduction,deduction_id'],
        ]);

        $customer = DB::transaction(function () use ($validated) {
            // Get barangay sequence and generate account number
            $barangay = BarangaySequence::findOrFail($validated['brgy_id']);
            $accountNo = $barangay->generateAccountNumber();

            $customer = Customer::create([
                'account_no' => $accountNo,
                'customer_name' => $validated['customer_name'],
                'type_id' => $validated['type_id'],
                'brgy_id' => $validated['brgy_id'],
                'address' => $validated['address'],
                'previous_reading' => array_key_exists('previous_reading', $validated) && $validated['previous_reading'] !== null
                    ? $validated['previous_reading']
                    : 0,
                'status' => $validated['status'],
            ]);

            // Increment the sequence
            $barangay->incrementSequence();

            // Handle benefits
            if (isset($validated['benefits']) && is_array($validated['benefits'])) {
                foreach ($validated['benefits'] as $deductionId) {
                    DB::table('customer_deduction')->insert([
                        'customer_id' => $customer->id,
                        'deduction_id' => $deductionId,
                        'created_at' => now(),
                        'updated_at' => now(),
                    ]);
                }
            }

            return $customer;
        });

        return response()->json([
            'data' => $customer,
        ], 201);
    }

    /**
     * Bulk upsert customers coming from the device.
     * Expects: { customers: [ {account_no, customer_name, address, previous_reading, is_active} ] }
     */
    public function sync(Request $request)
    {
        $validated = $request->validate([
            'customers' => ['required', 'array', 'max:200'],
            'customers.*.account_no' => ['required', 'string', 'max:255'],
            'customers.*.customer_name' => ['required', 'string', 'max:255'],
            'customers.*.address' => ['required', 'string', 'max:255'],
            'customers.*.previous_reading' => ['nullable', 'integer', 'min:0'],
            'customers.*.is_active' => ['nullable', 'boolean'],
        ]);

        $customers = $validated['customers'];
        $processed = 0;

        foreach ($customers as $row) {
            Customer::updateOrCreate(
                ['account_no' => $row['account_no']],
                [
                    'customer_name' => $row['customer_name'],
                    'address' => $row['address'],
                    'previous_reading' => array_key_exists('previous_reading', $row) && $row['previous_reading'] !== null ? (int) $row['previous_reading'] : 0,
                    'is_active' => array_key_exists('is_active', $row) ? (bool) $row['is_active'] : true,
                ]
            );
            $processed++;
        }

        return response()->json([
            'processed' => $processed,
        ]);
    }

    /**
     * Display the specified resource.
     */
    public function show(string $id)
    {
        //
    }

    /**
     * Show the form for editing the specified resource.
     */
    public function edit(string $id)
    {
        //
    }

    /**
     * Update the specified resource in storage.
     */
    public function update(Request $request, string $id)
    {
        $customer = Customer::findOrFail($id);

        $validated = $request->validate([
            'account_no' => ['required', 'string', 'max:255', 'unique:customer,account_no,' . $customer->id],
            'customer_name' => ['required', 'string', 'max:255'],
            'type_id' => ['required', 'integer', 'exists:customer_type,type_id'],
            'brgy_id' => ['required', 'integer', 'exists:barangay_sequence,brgy_id'],
            'address' => ['required', 'string', 'max:255'],
            'previous_reading' => ['nullable', 'integer', 'min:0'],
            'status' => ['required', 'in:active,disconnected'],
            'benefits' => ['nullable', 'array'],
            'benefits.*' => ['integer', 'exists:deduction,deduction_id'],
        ]);

        DB::transaction(function () use ($customer, $validated) {
            $customer->update([
                'account_no' => $validated['account_no'],
                'customer_name' => $validated['customer_name'],
                'type_id' => $validated['type_id'],
                'brgy_id' => $validated['brgy_id'],
                'address' => $validated['address'],
                'previous_reading' => array_key_exists('previous_reading', $validated) && $validated['previous_reading'] !== null
                    ? $validated['previous_reading']
                    : $customer->previous_reading,
                'status' => $validated['status'],
            ]);

            // Handle benefits - delete existing and add new
            DB::table('customer_deduction')->where('customer_id', $customer->id)->delete();
            if (isset($validated['benefits']) && is_array($validated['benefits'])) {
                foreach ($validated['benefits'] as $deductionId) {
                    DB::table('customer_deduction')->insert([
                        'customer_id' => $customer->id,
                        'deduction_id' => $deductionId,
                        'created_at' => now(),
                        'updated_at' => now(),
                    ]);
                }
            }
        });

        return response()->json([
            'data' => $customer,
        ]);
    }

    /**
     * Remove the specified resource from storage.
     */
    public function destroy(string $id)
    {
        $customer = Customer::findOrFail($id);
        $customer->delete();

        return response()->json([
            'message' => 'Customer deleted successfully',
        ]);
    }
}
