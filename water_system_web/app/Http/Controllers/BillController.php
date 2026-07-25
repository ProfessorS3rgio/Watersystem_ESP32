<?php

namespace App\Http\Controllers;
use App\Models\Reading;
use App\Models\Bill;
use App\Models\Setting;
use App\Models\Customer;
use App\Models\BillTransaction;
use Carbon\Carbon;
use Illuminate\Http\Request;

class BillController extends Controller
{
    public function index(Request $request)
    {
        $status = $request->query('status');
        $customerId = $request->query('customer_id');
        $customerSearch = $request->query('customer_search');

        $query = Bill::with('customer');

        if ($status) {
            $query->where('status', $status);
        }

        if ($customerId) {
            $query->where('customer_id', $customerId);
        }

        if ($customerSearch) {
            $query->whereHas('customer', function ($q) use ($customerSearch) {
                $q->where('customer_name', 'like', '%' . $customerSearch . '%')
                  ->orWhere('account_no', 'like', '%' . $customerSearch . '%');
            });
        }

        $bills = $query->orderBy('created_at', 'desc')->paginate(20);

        return response()->json([
            'data' => $bills,
        ]);
    }

  public function monthlyReport(Request $request)
{
    $validated = $request->validate([
        'month' => ['required', 'date_format:Y-m'],
        'brgy_id' => ['nullable', 'integer', 'exists:barangay_sequence,brgy_id'],
    ]);

    $month = Carbon::createFromFormat('Y-m', $validated['month'])->startOfMonth();
    $barangayId = $validated['brgy_id'] ?? null;
    
    // Get bills with their readings (existing query)
    $billsQuery = Bill::with(['customer', 'reading'])
        ->whereYear('bill_date', $month->year)
        ->whereMonth('bill_date', $month->month)
        ->orderBy('customer_account_number');

    if ($barangayId) {
        $billsQuery->whereHas('customer', fn ($customer) => $customer->where('brgy_id', $barangayId));
    }

    $bills = $billsQuery->get();
    
    // Get all customers in the barangay (including disconnected)
    $customersQuery = Customer::orderBy('account_no');
    
    if ($barangayId) {
        $customersQuery->where('brgy_id', $barangayId);
    }
    
    $allCustomers = $customersQuery->get()->keyBy('account_no');
    
    // Merge bills with customers
    $reportData = [];
    $processedAccounts = [];
    
    // First, add all bills
    foreach ($bills as $bill) {
        $accountNo = $bill->customer_account_number;
        $processedAccounts[] = $accountNo;
        
        $reportData[] = [
            'account_no' => $accountNo,
            'customer_name' => $bill->customer->customer_name ?? 'Unknown',
            'previous_reading' => $bill->reading->previous_reading ?? '',
            'current_reading' => $bill->reading->current_reading ?? '',
            'usage_m3' => $bill->reading->usage_m3 ?? '',
            'status' => $bill->status,
            'total_due' => (float) $bill->total_due,
            'has_bill' => true,
        ];
    }
    
    // Add customers without bills (disconnected or no readings)
    foreach ($allCustomers as $accountNo => $customer) {
        if (!in_array($accountNo, $processedAccounts)) {
            $reportData[] = [
                'account_no' => $customer->account_no,
                'customer_name' => $customer->customer_name,
                'previous_reading' => '',
                'current_reading' => '',
                'usage_m3' => '',
                'status' => $customer->status == 'disconnected' ? 'Disconnected' : 'No Bill',
                'total_due' => 0,
                'has_bill' => false,
            ];
        }
    }
    
    // Sort by account number
    usort($reportData, function($a, $b) {
        return strcmp($a['account_no'], $b['account_no']);
    });

    $barangay = $barangayId
        ? \App\Models\BarangaySequence::find($barangayId)?->barangay
        : 'All Barangays';
    $filenameBarangay = preg_replace('/[^A-Za-z0-9_-]+/', '-', $barangay);
    $filename = "monthly-billing-report-{$filenameBarangay}-{$month->format('Y-m')}.xlsx";
    $temporaryBase = tempnam(sys_get_temp_dir(), 'monthly-billing-report-');
    if ($temporaryBase === false) {
        abort(500, 'Unable to create the monthly report file.');
    }
    @unlink($temporaryBase);
    $temporaryFile = $temporaryBase . '.xlsx';
    $writer = \Box\Spout\Writer\Common\Creator\WriterEntityFactory::createXLSXWriter();
    $writer->openToFile($temporaryFile);

    $titleStyle = (new \Box\Spout\Writer\Common\Creator\Style\StyleBuilder())
        ->setFontBold()->setFontSize(16)->setFontColor('FFFFFF')->setBackgroundColor('0F3D5E')
        ->setCellAlignment(\Box\Spout\Common\Entity\Style\CellAlignment::CENTER)->build();
    $subtitleStyle = (new \Box\Spout\Writer\Common\Creator\Style\StyleBuilder())
        ->setFontItalic()->setFontColor('475569')->setCellAlignment(\Box\Spout\Common\Entity\Style\CellAlignment::CENTER)->build();
    $headerStyle = (new \Box\Spout\Writer\Common\Creator\Style\StyleBuilder())
        ->setFontBold()->setFontColor('FFFFFF')->setBackgroundColor('0E7490')
        ->setCellAlignment(\Box\Spout\Common\Entity\Style\CellAlignment::CENTER)->build();
    $currencyStyle = (new \Box\Spout\Writer\Common\Creator\Style\StyleBuilder())
        ->setFormat('₱#,##0.00')->setCellAlignment(\Box\Spout\Common\Entity\Style\CellAlignment::RIGHT)->build();
    $disconnectedStyle = (new \Box\Spout\Writer\Common\Creator\Style\StyleBuilder())
        ->setFontColor('DC2626')->build(); // Red color for disconnected

    // Title row
    $writer->addRow(\Box\Spout\Writer\Common\Creator\WriterEntityFactory::createRowFromArray(['MONTHLY WATER BILLING REPORT'], $titleStyle));
    
    // Subtitle row
    $writer->addRow(\Box\Spout\Writer\Common\Creator\WriterEntityFactory::createRowFromArray(["Barangay: {$barangay} | Billing Month: {$month->format('F Y')}"], $subtitleStyle));
    
    // Empty row
    $writer->addRow(\Box\Spout\Writer\Common\Creator\WriterEntityFactory::createRowFromArray([]));
    
    // Header row
    $writer->addRow(\Box\Spout\Writer\Common\Creator\WriterEntityFactory::createRowFromArray(['Account Number', 'Name', 'Previous', 'Present', 'Usage', 'Status', 'Total'], $headerStyle));

    // Data rows
    foreach ($reportData as $data) {
        $statusStyle = ($data['status'] === 'Disconnected') ? $disconnectedStyle : null;
        
        $row = \Box\Spout\Writer\Common\Creator\WriterEntityFactory::createRow([
            \Box\Spout\Writer\Common\Creator\WriterEntityFactory::createCell($data['account_no']),
            \Box\Spout\Writer\Common\Creator\WriterEntityFactory::createCell($data['customer_name']),
            \Box\Spout\Writer\Common\Creator\WriterEntityFactory::createCell($data['previous_reading']),
            \Box\Spout\Writer\Common\Creator\WriterEntityFactory::createCell($data['current_reading']),
            \Box\Spout\Writer\Common\Creator\WriterEntityFactory::createCell($data['usage_m3']),
            \Box\Spout\Writer\Common\Creator\WriterEntityFactory::createCell(
                $data['status'] === 'Disconnected' ? 'Disconnected' : ucfirst(strtolower($data['status'])),
                $statusStyle
            ),
            \Box\Spout\Writer\Common\Creator\WriterEntityFactory::createCell((float) $data['total_due'], $currencyStyle),
        ]);
        
        $writer->addRow($row);
    }

    $writer->close();

    return response()->download($temporaryFile, $filename, [
        'Content-Type' => 'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet',
    ])->deleteFileAfterSend(true);
}
    public function indexByCustomer(Request $request, Customer $customer)
    {
        $status = $request->query('status');

        $customerId = $customer->getKey();

        $query = Bill::where('customer_id', $customerId);

        if ($status) {
            $query->where('status', $status);
        }

        $bills = $query->orderBy('created_at', 'desc')->get();

        return response()->json([
            'data' => $bills,
        ]);
    }

    public function paymentsByCustomer(Request $request, Customer $customer)
    {
        $limit = $request->query('limit', 10);

        $customerId = $customer->getKey();

        $payments = BillTransaction::whereHas('bill', function ($query) use ($customerId) {
            $query->where('customer_id', $customerId);
        })
        ->whereIn('type', ['payment', 'void'])
        ->with(['bill', 'processedBy'])
        ->orderBy('transaction_date', 'desc')
        ->limit($limit)
        ->get()
        ->map(function ($transaction) {
            return [
                'id' => $transaction->getKey(),
                'reference_number' => $transaction->bill?->reference_number ?? $transaction->bill_reference_number,
                'amount_paid' => $transaction->amount,
                'cash_received' => $transaction->cash_received,
                'change' => $transaction->change,
                'payment_method' => $transaction->payment_method,
                'processed_by' => $transaction->processedBy?->name,
                'created_at' => $transaction->transaction_date,
                'notes' => $transaction->notes,
                'type' => $transaction->type,
            ];
        });

        return response()->json([
            'data' => $payments,
        ]);
    }

    public function pay(Request $request, Bill $bill)
    {
        // Check if bill can be paid
        if ($bill->status !== 'pending') {
            return response()->json([
                'message' => 'Only pending bills can be paid.'
            ], 422);
        }

        $validated = $request->validate([
            'amount_paid' => ['required', 'numeric', 'min:0'],
            'cash_received' => ['required', 'numeric', 'min:0'],
            'change_amount' => ['required', 'numeric'],
            'payment_method' => ['required', 'string', 'in:cash'],
        ]);

        // Update bill status to paid
        $bill->status = 'paid';
        $bill->save();

        // Create transaction record
        BillTransaction::create([
            'bill_reference_number' => $bill->reference_number,
            'type' => 'payment',
            'amount' => $validated['amount_paid'],
            'cash_received' => $validated['cash_received'],
            'change' => $validated['change_amount'],
            'transaction_date' => now(),
            'payment_method' => $validated['payment_method'],
            'processed_by_user_id' => auth()->id(),
        ]);

        return response()->json([
            'data' => $bill,
        ]);
    }

    public function todayPayments(Request $request)
    {
        $today = now()->toDateString();

        $payments = Bill::where('status', 'paid')
            ->whereDate('updated_at', $today)
            ->get();

        return response()->json([
            'count' => $payments->count(),
            'total' => $payments->sum('total_due'),
        ]);
    }

    public function monthlyPayments(Request $request)
    {
        $currentMonth = now()->month;
        $currentYear = now()->year;

        $payments = Bill::where('status', 'paid')
            ->whereYear('updated_at', $currentYear)
            ->whereMonth('updated_at', $currentMonth)
            ->get();

        return response()->json([
            'count' => $payments->count(),
            'total' => $payments->sum('total_due'),
        ]);
    }

    public function monthlyCollectable(Request $request)
    {
        $currentMonth = now()->month;
        $currentYear = now()->year;

        $collectable = Bill::where('status', 'pending')
            ->whereYear('due_date', $currentYear)
            ->whereMonth('due_date', $currentMonth)
            ->get();

        return response()->json([
            'count' => $collectable->count(),
            'total' => $collectable->sum('total_due'),
        ]);
    }

    public function stats(Request $request)
    {
        $pending = Bill::where('status', 'pending')->count();

        $overdue = Bill::where('status', 'pending')
            ->where('due_date', '<', now()->toDateString())
            ->count();

        return response()->json([
            'pending' => $pending,
            'overdue' => $overdue,
        ]);
    }
    public function markPaid(Request $request, Bill $bill)
    {
        $validated = $request->validate([
            'status' => ['nullable', 'string', 'in:paid'],
        ]);

        // Apply penalty at payment time (if overdue beyond grace days).
        $settings = Setting::query()->orderBy('id')->first();
        $penaltyAfterDays = $settings ? (int) $settings->penalty_after_days : (int) env('PENALTY_AFTER_DAYS', 0);
        $penaltyAmount = $settings ? (float) $settings->penalty_amount : (float) env('PENALTY_AMOUNT', 0.00);

        $penalty = 0.0;
        if ($bill->due_date) {
            $threshold = Carbon::parse($bill->due_date)->addDays($penaltyAfterDays);
            if (now()->greaterThan($threshold) && $penaltyAmount > 0) {
                $penalty = $penaltyAmount;
            }
        }

        $bill->penalty = $penalty;
        $bill->total_due = (float) $bill->charges + (float) $bill->penalty;

        $bill->status = 'paid';
        $bill->save();

        // Create transaction record
        BillTransaction::create([
            'bill_reference_number' => $bill->reference_number,
            'type' => 'payment',
            'amount' => $bill->total_due,
            'cash_received' => $bill->total_due,
            'change' => 0,
            'transaction_date' => now(),
            'payment_method' => 'cash',
            'processed_by_user_id' => auth()->id(),
        ]);

        return response()->json([
            'data' => $bill,
        ]);
    }

    public function void(Request $request, Bill $bill)
    {
        // Check if bill can be voided
        if ($bill->status !== 'paid') {
            return response()->json([
                'message' => 'Only paid bills can be voided.'
            ], 422);
        }

        $validated = $request->validate([
            'reason' => ['nullable', 'string', 'max:1000'],
        ]);

        $bill->status = 'pending';
        $bill->void_reason = $validated['reason'] ?? null;
        $bill->save();

        // Create transaction record for void
        BillTransaction::create([
            'bill_reference_number' => $bill->reference_number,
            'type' => 'void',
            'amount' => $bill->total_due,
            'transaction_date' => now(),
            'processed_by_user_id' => auth()->id(),
            'notes' => $validated['reason'] ?? null,
        ]);

        return response()->json([
            'data' => $bill,
        ]);
    }

    public function transactions(Request $request)
    {
        $limit = $request->query('limit', 50);

        $transactions = BillTransaction::with(['bill.customer', 'processedBy'])
            ->orderBy('transaction_date', 'desc')
            ->limit($limit)
            ->get();

        return response()->json([
            'data' => $transactions,
        ]);
    }

    public function getPayment(Request $request, Bill $bill)
    {
        $transaction = BillTransaction::where('bill_reference_number', $bill->reference_number)
            ->where('type', 'payment')
            ->first();

        if (!$transaction) {
            return response()->json([
                'message' => 'No payment found for this bill.'
            ], 404);
        }

        return response()->json($transaction);
    }

    /**
     * Bulk upsert bills coming from the device.
     * Expects: { bills: [ {bill_id, reference_number, customer_id, reading_id, bill_date, rate_per_m3, charges, penalty, total_due, status, created_at, updated_at} ] }
     */
 public function sync(Request $request)
{
    $validated = $request->validate([
        'bills' => ['required', 'array', 'max:2000'],
        'bills.*.reference_number' => ['required', 'string', 'max:255'],
        'bills.*.customer_id' => ['nullable', 'integer'],
        'bills.*.reading_id' => ['nullable', 'integer'],
        'bills.*.device_uid' => ['nullable', 'string', 'max:255'],
        'bills.*.bill_date' => ['nullable', 'string'],
        'bills.*.rate_per_m3' => ['nullable', 'numeric'],
        'bills.*.charges' => ['nullable', 'numeric'],
        'bills.*.penalty' => ['nullable', 'numeric'],
        'bills.*.total_due' => ['nullable', 'numeric'],
        'bills.*.status' => ['nullable', 'string', 'max:255'],
        'bills.*.customer_account_number' => ['required', 'string', 'max:255'],
    ]);

    $bills = $validated['bills'];
    $processed = 0;
    $skipped = 0;

    foreach ($bills as $row) {
        // ONLY use account_no - NEVER trust customer_id from device!
        $customer = Customer::where('account_no', $row['customer_account_number'])->first();
        
        if (!$customer) {
            \Log::warning('Bill sync: Customer not found', [
                'account_no' => $row['customer_account_number']
            ]);
            $skipped++;
            continue;
        }

        // Find correct reading_id from OUR database
        $readingId = null;
        if (!empty($row['bill_date'])) {
            $billDate = Carbon::parse($row['bill_date']);
            
            // Try to find reading by customer_account_number and same month/year
            $reading = Reading::where('customer_account_number', $row['customer_account_number'])
                ->whereYear('reading_at', $billDate->year)
                ->whereMonth('reading_at', $billDate->month)
                ->orderBy('reading_at', 'desc')
                ->first();
            
            if ($reading) {
                $readingId = $reading->reading_id;
            }
        }

        // Skip if we can't find a reading_id
        if (!$readingId) {
            \Log::error('Bill sync: Cannot find reading_id', [
                'account' => $row['customer_account_number'],
                'bill_date' => $row['bill_date'] ?? 'null',
                'reference' => $row['reference_number']
            ]);
            $skipped++;
            continue;
        }

        Bill::updateOrCreate(
            ['reference_number' => $row['reference_number']],
            [
                'customer_id' => $customer->customer_id,
                'reading_id' => $readingId,
                'device_uid' => $row['device_uid'] ?? null,
                'bill_date' => $row['bill_date'] ?? null,
                'rate_per_m3' => $row['rate_per_m3'] ?? 0,
                'charges' => $row['charges'] ?? 0,
                'penalty' => $row['penalty'] ?? 0,
                'total_due' => $row['total_due'] ?? 0,
                'status' => $row['status'] ?? 'Pending',
                'customer_account_number' => $row['customer_account_number'],
            ]
        );

        $processed++;
    }

    \Log::info('Bills synced', ['processed' => $processed, 'skipped' => $skipped]);

    return response()->json([
        'processed' => $processed,
        'skipped' => $skipped,
    ]);
}
    /**
     * Bulk upsert bill transactions coming from the device.
     * Expects: { bill_transactions: [ {...} ] }
     */
    public function syncTransactions(Request $request)
    {
        \Log::info('Syncing bill transactions', ['count' => count($request->bill_transactions ?? [])]);

        $validated = $request->validate([
            // Allow large device exports; client will still batch to keep requests small.
            'bill_transactions' => ['required', 'array', 'max:2000'],
            'bill_transactions.*.bill_transaction_id' => ['nullable', 'integer'],
            'bill_transactions.*.bill_id' => ['nullable', 'integer'],
            'bill_transactions.*.bill_reference_number' => ['required', 'string'],
            'bill_transactions.*.type' => ['required', 'string'],
            'bill_transactions.*.source' => ['required', 'string'],
            'bill_transactions.*.amount' => ['required', 'numeric'],
            'bill_transactions.*.cash_received' => ['nullable', 'numeric'],
            'bill_transactions.*.change' => ['required', 'numeric'],
            'bill_transactions.*.transaction_date' => ['required', 'string'],
            'bill_transactions.*.payment_method' => ['required', 'string'],
            'bill_transactions.*.processed_by_device_uid' => ['nullable', 'string'],
            'bill_transactions.*.notes' => ['nullable', 'string'],
            'bill_transactions.*.created_at' => ['nullable', 'string'],
            'bill_transactions.*.updated_at' => ['nullable', 'string'],
        ]);

        $transactions = $validated['bill_transactions'];
        $processed = 0;

        foreach ($transactions as $row) {
            try {
                BillTransaction::updateOrCreate(
                    [
                        'bill_reference_number' => $row['bill_reference_number'],
                        'transaction_date' => $row['transaction_date'],
                        'amount' => $row['amount'],
                    ],
                    [
                        'device_uid' => $row['processed_by_device_uid'] ?? null,
                        'type' => $row['type'],
                        'source' => $row['source'],
                        'cash_received' => $row['cash_received'] ?? null,
                        'change' => $row['change'],
                        'payment_method' => $row['payment_method'],
                        'processed_by_device_uid' => $row['processed_by_device_uid'] ?? null,
                        'notes' => $row['notes'] ?? null,
                        'Synced' => true,
                        'last_sync' => now(),
                        'created_at' => $row['created_at'] ?? now(),
                        'updated_at' => $row['updated_at'] ?? now(),
                    ]
                );

                $processed++;
            } catch (\Exception $e) {
                \Log::error('Failed to sync bill transaction', [
                    'bill_reference_number' => $row['bill_reference_number'],
                    'transaction_date' => $row['transaction_date'],
                    'amount' => $row['amount'],
                    'error' => $e->getMessage()
                ]);
            }
        }

        \Log::info('Bill transactions synced', ['processed' => $processed]);

        return response()->json([
            'processed' => $processed,
        ]);
    }

    /**
     * Mark bill transactions as synced after successful device sync.
     */
    public function markTransactionsSynced(Request $request)
    {
        $validated = $request->validate([
            'bill_transaction_ids' => ['required', 'array'],
            'bill_transaction_ids.*' => ['required', 'integer'],
        ]);

        $updated = BillTransaction::whereIn('bill_transaction_id', $validated['bill_transaction_ids'])
            ->update([
                'Synced' => true,
                'last_sync' => now(),
            ]);

        return response()->json([
            'updated' => $updated,
            'message' => "{$updated} bill transactions marked as synced",
        ]);
    }

    /**
     * Mark bills as synced after successful device sync.
     */
    public function markSynced(Request $request)
    {
        $validated = $request->validate([
            'bill_ids' => ['required', 'array'],
            'bill_ids.*' => ['required', 'integer'],
        ]);

        $updated = Bill::whereIn('bill_id', $validated['bill_ids'])
            ->update([
                'Synced' => true,
                'last_sync' => now(),
            ]);

        return response()->json([
            'updated' => $updated,
            'message' => "{$updated} bills marked as synced",
        ]);
    }
}
