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
        $brgyId = $request->query('brgy_id');
        $updatedAfter = $request->query('updated_after');

        $query = DB::table('customer');

        if ($accountNo) {
            $query->where('account_no', $accountNo);
        }

        if ($name) {
            $query->where('customer_name', 'like', '%' . $name . '%');
        }

        if ($brgyId) {
            $query->where('brgy_id', $brgyId);
        }

        if ($updatedAfter) {
            $query->where('customer.updated_at', '>', Carbon::createFromTimestamp($updatedAfter));
        }

        $latestReadingAt = DB::table('reading')
            ->select('customer_id', DB::raw('MAX(reading_at) as latest_reading_at'))
            ->groupBy('customer_id');

        $latestBillDate = DB::table('bill')
            ->select('customer_id', DB::raw('MAX(bill_date) as latest_bill_date'))
            ->groupBy('customer_id');

        $customers = $query
            ->leftJoinSub($latestReadingAt, 'lr', function ($join) {
                $join->on('lr.customer_id', '=', 'customer.customer_id');
            })
            ->leftJoin('reading as r', function ($join) {
                $join->on('r.customer_id', '=', 'customer.customer_id')
                    ->on('r.reading_at', '=', 'lr.latest_reading_at');
            })
            ->leftJoinSub($latestBillDate, 'lb', function ($join) {
                $join->on('lb.customer_id', '=', 'customer.customer_id');
            })
            ->leftJoin('bill as b', function ($join) {
                $join->on('b.customer_id', '=', 'customer.customer_id')
                    ->on('b.bill_date', '=', 'lb.latest_bill_date');
            })
            ->orderBy('customer.customer_name')
            ->get([
                // alias primary key to `id` for frontend compatibility
                'customer.customer_id as id',
                'customer.account_no',
                'customer.customer_name',
                'customer.type_id',
                'customer.deduction_id',
                'customer.brgy_id',
                'customer.address',
                'customer.previous_reading',
                'customer.status',
                'customer.created_at',
                'customer.updated_at',
                DB::raw('r.current_reading as current_reading'),
                DB::raw('r.usage_m3 as last_usage_m3'),
                DB::raw('r.reading_at as last_reading_at'),
                DB::raw('b.bill_id as latest_bill_id'),
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
            'deduction_id' => ['nullable', 'integer', 'exists:deduction,deduction_id'],
        ]);

        $customer = DB::transaction(function () use ($validated) {
            // Get barangay sequence and generate account number
            $barangay = BarangaySequence::findOrFail($validated['brgy_id']);
            $accountNo = $barangay->generateAccountNumber();

            $customer = Customer::create([
                'account_no' => $accountNo,
                'customer_name' => $validated['customer_name'],
                'type_id' => $validated['type_id'],
                'deduction_id' => $validated['deduction_id'] ?? null,
                'brgy_id' => $validated['brgy_id'],
                'address' => $validated['address'],
                'previous_reading' => array_key_exists('previous_reading', $validated) && $validated['previous_reading'] !== null
                    ? $validated['previous_reading']
                    : 0,
                'status' => $validated['status'],
            ]);

            // Increment the sequence
            $barangay->incrementSequence();

            // Attach deduction to customer_deduction table if provided
            if ($validated['deduction_id']) {
                $customer->deductions()->attach($validated['deduction_id']);
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
            'customers.*.type_id' => ['nullable', 'integer', 'exists:customer_type,type_id'],
            'customers.*.deduction_id' => ['nullable', 'integer', 'exists:deduction,deduction_id'],
            'customers.*.brgy_id' => ['nullable', 'integer', 'exists:barangay_sequence,brgy_id'],
        ]);

        $customers = $validated['customers'];
        $processed = 0;

        foreach ($customers as $row) {
            $customer = Customer::updateOrCreate(
                ['account_no' => $row['account_no']],
                [
                    'customer_name' => $row['customer_name'],
                    'address' => $row['address'],
                    'previous_reading' => array_key_exists('previous_reading', $row) && $row['previous_reading'] !== null ? (int) $row['previous_reading'] : 0,
                    'status' => array_key_exists('is_active', $row) ? ($row['is_active'] ? 'active' : 'disconnected') : 'active',
                    'type_id' => $row['type_id'] ?? null,
                    'deduction_id' => $row['deduction_id'] ?? null,
                    'brgy_id' => $row['brgy_id'] ?? null,
                ]
            );

            // Sync deductions in customer_deduction table
            if ($row['deduction_id']) {
                $customer->deductions()->sync([$row['deduction_id']]);
            } else {
                $customer->deductions()->detach();
            }

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
            'deduction_id' => ['nullable', 'integer', 'exists:deduction,deduction_id'],
        ]);

        DB::transaction(function () use ($customer, $validated) {
            $customer->update([
                'account_no' => $validated['account_no'],
                'customer_name' => $validated['customer_name'],
                'type_id' => $validated['type_id'],
                'deduction_id' => $validated['deduction_id'] ?? null,
                'brgy_id' => $validated['brgy_id'],
                'address' => $validated['address'],
                'previous_reading' => array_key_exists('previous_reading', $validated) && $validated['previous_reading'] !== null
                    ? $validated['previous_reading']
                    : $customer->previous_reading,
                'status' => $validated['status'],
            ]);

            // Sync deductions in customer_deduction table
            if ($validated['deduction_id']) {
                $customer->deductions()->sync([$validated['deduction_id']]);
            } else {
                $customer->deductions()->detach();
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
