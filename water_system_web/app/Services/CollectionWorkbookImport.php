<?php

namespace App\Services;

use App\Models\{BarangaySequence, Bill, BillTransaction, Customer, CustomerProfile};
use Carbon\Carbon;
use Illuminate\Http\Request;
use Illuminate\Support\Collection;
use Illuminate\Support\Facades\DB;
use Illuminate\Support\Str;
use Illuminate\Validation\ValidationException;
use PhpOffice\PhpSpreadsheet\Spreadsheet;

/** Collection-only workbooks never create readings or recalculate charges/penalties. */
class CollectionWorkbookImport
{
    private function normalize(mixed $value): string
    {
        return preg_replace('/[^a-z0-9]/', '', Str::lower(Str::ascii((string) $value)));
    }

    private function nameTokens(string $name): array
    {
        // CLOSE/CLOSED at the end is a workbook service note, not a surname.
        $name = preg_replace('/\s+(?:close|closed)\s*\.?\s*$/iu', '', trim($name));
        // V.MAGDADARO and V. MAGDADARO have the same name tokens.
        $name = str_replace('.', ' ', $name);
        return array_values(array_filter(array_map(
            fn ($part) => $this->normalize($part),
            preg_split('/\s+/u', trim($name))
        ), fn ($part) => $part !== ''));
    }

    private function namesMatch(string $fileName, string $databaseName): bool
    {
        $file = $this->nameTokens($fileName);
        $database = $this->nameTokens($databaseName);
        if ($file === $database) {
            return $file !== [];
        }
        // Keep first/last names and all spelled-out name parts exact. Only
        // interior single-letter initials may be omitted by either source.
        if (count($file) < 2 || count($database) < 2
            || array_shift($file) !== array_shift($database)
            || array_pop($file) !== array_pop($database)) {
            return false;
        }
        $i = 0; $j = 0;
        while ($i < count($file) && $j < count($database)) {
            if ($file[$i] === $database[$j]) {
                $i++; $j++;
            } elseif (strlen($file[$i]) === 1 && strlen($database[$j]) !== 1) {
                $i++;
            } elseif (strlen($database[$j]) === 1 && strlen($file[$i]) !== 1) {
                $j++;
            } else {
                // Different explicit initials or spelled-out names conflict.
                return false;
            }
        }
        foreach ([array_slice($file, $i), array_slice($database, $j)] as $remaining) {
            foreach ($remaining as $part) {
                if (strlen($part) !== 1) return false;
            }
        }
        return true;
    }

    private function hasMinorSurnameDifference(string $fileName, string $databaseName): bool
    {
        $file = $this->nameTokens($fileName);
        $database = $this->nameTokens($databaseName);
        if (count($file) < 2 || count($database) < 2) return false;
        $fileSurname = array_pop($file);
        $databaseSurname = array_pop($database);
        // A one-character surname difference needs review, never a silent
        // exact match. The remaining names must still pass the normal rules.
        return strlen($fileSurname) >= 5 && strlen($databaseSurname) >= 5
            && levenshtein($fileSurname, $databaseSurname) === 1
            && $this->namesMatch(implode(' ', [...$file, $databaseSurname]), $databaseName);
    }

    private function sheets(Spreadsheet $book): array
    {
        $found = [];
        foreach ($book->getWorksheetIterator() as $sheet) {
            $rows = $sheet->toArray(null, true, false, true);
            foreach (array_slice($rows, 0, 75, true) as $number => $row) {
                $headers = array_map(fn ($v) => $this->normalize($v), $row);
                if (in_array('monthlycollection', $headers, true) && in_array('meterno', $headers, true)
                    && in_array('name', $headers, true) && in_array('no', $headers, true)) {
                    // The supplied template has two collection columns, I and J,
                    // even on sheets whose heading is anchored only in J.
                    if (($headers['K'] ?? '') !== 'meterno' || ($headers['G'] ?? '') !== 'name'
                        || ($headers['F'] ?? '') !== 'no') {
                        continue;
                    }
                    $found[] = ['name' => $sheet->getTitle(), 'header' => $number, 'rows' => $rows];
                    break;
                }
            }
        }
        return $found;
    }

    public function supports(Spreadsheet $book): bool
    {
        return $this->sheets($book) !== [];
    }

    private function metadata(array $sheet, string $filename): array
    {
        $heading = collect(array_slice($sheet['rows'], 0, $sheet['header'] - 1))->flatten()->filter()->implode(' ');
        $barangay = BarangaySequence::all()->first(fn ($b) => str_contains($this->normalize($heading), $this->normalize($b->barangay)));
        if (! $barangay) {
            throw ValidationException::withMessages(['file' => "Cannot identify the barangay on {$sheet['name']}."]);
        }
        // Billing Month takes precedence over collection dates and future month labels.
        $text = preg_replace("/\bSep['’]?t\.?(?![a-z])/i", 'September', $heading);
        if (! preg_match('/Billing Month:\s*([A-Za-z]+)\s+\d{1,2}\s*-\s*\d{1,2}\s*,?\s*(20\d{2})/i', $text, $match)
            && ! preg_match('/\b(January|February|March|April|May|June|July|August|September|October|November|December)\s+(20\d{2})/i', $filename, $match)) {
            throw ValidationException::withMessages(['file' => "Cannot identify the billing month on {$sheet['name']}."]);
        }
        $month = Carbon::parse("1 {$match[1]} {$match[2]}")->startOfMonth();
        $collectionDate = null;
        if (preg_match('/Collection Date:\s*(.+?)\s*To be collect/i', $text, $date)) {
            try {
                $collectionDate = Carbon::parse(trim($date[1]))->setTime(12, 0)->toDateTimeString();
            } catch (\Throwable) {
                // Do not silently date historical receipts as today's payments.
            }
        }
        if (! $collectionDate) {
            throw ValidationException::withMessages(['file' => "Cannot identify the collection date on {$sheet['name']}."]);
        }
        return [$barangay, $month, $collectionDate];
    }

    private function sourceRows(array $sheet): array
    {
        $result = [];
        foreach ($sheet['rows'] as $number => $row) {
            $sequence = $row['F'] ?? null;
            if ($number <= $sheet['header'] || ! is_numeric($sequence) || $sequence <= 0
                || floor((float) $sequence) != $sequence || ! is_string($row['G'] ?? null)
                || ! preg_match('/\p{L}/u', $row['G'])) {
                continue;
            }
            $result[] = ['row' => $number, 'sequence' => (int) $sequence, 'name' => trim($row['G']),
                'meter' => Str::upper(ltrim(trim((string) ($row['K'] ?? '')), "'‘’")),
                'amounts' => [$row['I'] ?? null, $row['J'] ?? null]];
        }
        return $result;
    }

    private function billState(?Bill $bill): ?array
    {
        return $bill ? $bill->only(['bill_id', 'customer_id', 'bill_date', 'status', 'total_due', 'amount_paid', 'balance_due', 'updated_at']) : null;
    }

    public function preview(Request $request, Spreadsheet $book)
    {
        $file = $request->file('file')->getClientOriginalName();
        $sheets = $this->sheets($book);
        $options = [];
        foreach ($sheets as $sheet) {
            [$barangay, $month] = $this->metadata($sheet, $file);
            $options[] = ['name' => $sheet['name'], 'barangay' => $barangay->barangay,
                'billing_month_label' => $month->format('F Y'), 'rows' => count($this->sourceRows($sheet))];
        }
        if (count($sheets) > 1 && ! $request->input('sheet_name')) {
            return response()->json(['requires_sheet_selection' => true, 'file_name' => $file, 'sheets' => $options]);
        }
        $sheet = collect($sheets)->first(fn ($s) => ! $request->input('sheet_name') || $s['name'] === $request->input('sheet_name'));
        if (! $sheet) {
            throw ValidationException::withMessages(['sheet_name' => 'Select a monthly collection worksheet.']);
        }
        [$barangay, $month, $collectionDate] = $this->metadata($sheet, $file);
        $sources = $this->sourceRows($sheet);
        $meters = collect($sources)->pluck('meter')->filter()->countBy();
        $sequences = collect($sources)->pluck('sequence')->countBy();
        $customers = Customer::where('brgy_id', $barangay->brgy_id)->get()->groupBy(function ($c) {
            preg_match('/(\d+)$/', $c->account_no, $m);
            return isset($m[1]) ? (int) $m[1] : -1;
        });
        $rows = []; $applicable = [];
        $summary = ['total_rows' => count($sources), 'changes' => 0, 'unchanged' => 0, 'skipped' => 0, 'warnings' => 0, 'errors' => 0];
        foreach ($sources as $source) {
            $matches = $customers->get($source['sequence'], collect());
            $customer = $matches->count() === 1 ? $matches->first() : null;
            $profile = $customer?->profile;
            $bills = $customer ? Bill::where('customer_id', $customer->customer_id)
                ->where('bill_date', '>=', $month->toDateString())
                ->where('bill_date', '<', $month->copy()->addMonth()->toDateString())->get() : collect();
            $bill = $bills->count() === 1 ? $bills->first() : null;
            $error = null; $amount = 0;
            foreach ($source['amounts'] as $value) {
                if ($value === null || trim((string) $value) === '') continue;
                if (! is_numeric($value) || $value < 0 || abs(round((float) $value, 2) - (float) $value) > 0.000001) {
                    $error = 'Collection amounts must be non-negative numbers with at most two decimal places.';
                    break;
                }
                $amount += (int) round((float) $value * 100);
            }
            $warnings = [];
            if (! $customer || $sequences[$source['sequence']] > 1) $error = 'Account sequence is missing or ambiguous in this barangay.';
            elseif (! $this->namesMatch($source['name'], $customer->customer_name)) {
                if ($this->hasMinorSurnameDifference($source['name'], $customer->customer_name)) {
                    $warnings[] = 'Check surname spelling: workbook "'.$source['name'].'"; account '.$customer->account_no.' is "'.$customer->customer_name.'". Database name will be kept.';
                } else {
                    $warnings[] = 'Name does not match account '.$customer->account_no.': workbook "'.$source['name'].'"; database "'.$customer->customer_name.'". Database name will be kept; collection applies to this account.';
                }
            }
            $meter = $source['meter'];
            if ($meter !== '' && (mb_strlen($meter) > 80 || preg_match('/[\x00-\x1F\x7F]/', $meter))) $error = 'Invalid meter number.';
            $skipMeter = $meter !== '' && ($meters[$meter] > 1 || CustomerProfile::where('meter_number', $meter)->when($customer, fn ($q) => $q->where('customer_id', '!=', $customer->customer_id))->exists());
            if ($skipMeter) $warnings[] = 'Meter number is duplicated or assigned to another account. Meter update will be skipped; collection can still be processed.';
            if (! $skipMeter && $meter !== '' && $profile?->meter_number && $profile->meter_number !== $meter) $error = 'Account already has a different meter number. Resolve in Member profile first.';
            $delta = $amount - (int) round((float) ($bill?->amount_paid ?? 0) * 100);
            if ($amount > 0 && ! $bill) $error = 'A unique existing bill for this month is required to import a payment. Import readings first.';
            elseif ($amount > 0 && (Str::lower($bill->status) === 'void' || $delta < 0 || $amount > (int) round((float) $bill->total_due * 100))) $error = 'Collection conflicts with the existing bill or recorded payments. Resolve before importing.';
            $meterChange = ! $skipMeter && $meter !== '' && $meter !== $profile?->meter_number;
            $paymentChange = $amount > 0 && $delta > 0;
            $status = $error ? 'error' : (($meterChange || $paymentChange) ? 'change' : 'unchanged');
            $message = $error ?? (($meterChange || $paymentChange) ? 'Ready to apply collection and/or meter number.' : 'No new payment or meter changes.');
            if (! $error && $warnings) {
                if ($status === 'change') $status = 'warning';
                $message = implode(' ', $warnings).' '.$message;
            }
            $summary[match ($status) { 'error' => 'errors', 'change' => 'changes', 'warning' => 'warnings', default => 'unchanged' }]++;
            $rows[] = ['row' => $source['row'], 'account_no' => $customer?->account_no, 'customer_name' => $source['name'],
                'status' => $status, 'message' => $message, 'file_payment_amount' => $amount / 100,
                'file_meter_number' => $meter, 'database_meter_number' => $profile?->meter_number,
                'meter_update_skipped' => $skipMeter,
                'database_payment_status' => $bill?->status, 'file_payment_status' => $amount > 0 ? 'Payment recorded' : null,
                'bill_date' => $bill?->bill_date?->format('Y-m-d'), 'bill_action' => $paymentChange ? 'Record collection' : null];
            if (in_array($status, ['change', 'warning'], true)) {
                $applicable[] = ['account_no' => $customer->account_no, 'customer_id' => $customer->customer_id,
                    'customer_name' => $customer->customer_name, 'brgy_id' => $customer->brgy_id,
                    'source_row' => $source['row'], 'old_meter' => $profile?->meter_number, 'meter' => $meterChange ? $meter : null,
                    'warnings' => $warnings,
                    'bill_id' => $paymentChange ? $bill->bill_id : null, 'bill_state' => $this->billState($paymentChange ? $bill : null),
                    'amount' => $amount / 100];
            }
        }
        $token = $applicable ? (string) Str::uuid() : null;
        $expires = now()->addMinutes(15);
        if ($token) $request->session()->put("reading_import.{$token}", ['format' => 'monthly_collection', 'expires_at' => $expires->timestamp,
            'rows' => $applicable, 'source_file' => $file, 'barangay' => $barangay->barangay, 'billing_month' => $month->format('Y-m'), 'collection_date' => $collectionDate, 'sheet_names' => [$sheet['name']]]);
        return response()->json(['format' => 'monthly_collection', 'token' => $token, 'expires_at' => $expires->toIso8601String(),
            'file_name' => $file, 'sheet_name' => $sheet['name'], 'barangay' => $barangay->barangay, 'billing_month' => $month->format('Y-m'),
            'billing_month_label' => $month->format('F Y'), 'summary' => $summary, 'rows' => $rows]);
    }

    public function confirm(Collection $rows, array $payload, AuditLogService $audit)
    {
        $result = DB::transaction(function () use ($rows, $payload, $audit) {
            $payments = 0; $meters = 0;
            foreach ($rows as $row) {
                $customer = Customer::whereKey($row['customer_id'])->lockForUpdate()->first();
                $profile = CustomerProfile::where('customer_id', $row['customer_id'])->lockForUpdate()->first();
                if (! $customer || $customer->account_no !== $row['account_no'] || $customer->customer_name !== $row['customer_name']
                    || $customer->brgy_id != $row['brgy_id'] || ($row['meter'] && $profile?->meter_number !== $row['old_meter'])) {
                    throw ValidationException::withMessages(['token' => 'Account changed after preview. Upload again.']);
                }
                if ($row['meter']) {
                    if (CustomerProfile::where('meter_number', $row['meter'])->where('customer_id', '!=', $customer->customer_id)->exists()) {
                        throw ValidationException::withMessages(['token' => 'Meter was assigned after preview. Upload again.']);
                    }
                    CustomerProfile::updateOrCreate(['customer_id' => $customer->customer_id], ['meter_number' => $row['meter']]);
                    $meters++;
                }
                if ($row['bill_id']) {
                    $bill = Bill::whereKey($row['bill_id'])->lockForUpdate()->first();
                    if (json_encode($this->billState($bill)) !== json_encode($row['bill_state'])) {
                        throw ValidationException::withMessages(['token' => 'Bill changed after preview. Upload again.']);
                    }
                    $delta = round($row['amount'] - (float) $bill->amount_paid, 2);
                    $bill->amount_paid = $row['amount'];
                    $bill->balance_due = round((float) $bill->total_due - $row['amount'], 2);
                    $bill->status = $bill->balance_due > 0 ? 'Partially Paid' : 'Paid';
                    $bill->Synced = false;
                    $bill->save();
                    BillTransaction::create(['bill_id' => $bill->bill_id, 'bill_reference_number' => $bill->reference_number,
                        'device_uid' => $bill->device_uid, 'type' => 'payment', 'source' => 'Office', 'amount' => $delta,
                        'cash_received' => $delta, 'change' => 0, 'transaction_date' => $payload['collection_date'], 'payment_method' => 'workbook_import',
                        'processed_by_user_id' => auth()->id(), 'notes' => "Monthly collection from {$payload['source_file']}, row {$row['source_row']}, billing month {$payload['billing_month']}.", 'Synced' => false]);
                    $payments++;
                }
            }
            $result = ['applied' => $rows->count(), 'payments_imported' => $payments, 'meters_updated' => $meters,
                'updated' => 0, 'created' => 0, 'bills_created' => 0, 'bills_updated' => 0, 'bill_statuses_updated' => $payments];
            $audit->record('monthly_collections_imported', 'ReadingImport', null, $result, ['source_file' => $payload['source_file'],
                'billing_month' => $payload['billing_month'], 'imported_rows' => $rows->all()]);
            return $result;
        });
        return response()->json([...$result, 'barangay' => $payload['barangay'], 'billing_month' => $payload['billing_month'], 'message' => "{$result['applied']} collection rows were applied."]);
    }
}
