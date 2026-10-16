<?php

namespace App\Http\Controllers;

use App\Models\BarangaySequence;
use App\Models\Bill;
use App\Models\BillTransaction;
use App\Models\Customer;
use App\Models\Reading;
use Carbon\Carbon;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\DB;
use Illuminate\Support\Str;
use Illuminate\Validation\ValidationException;
use PhpOffice\PhpSpreadsheet\IOFactory;
use PhpOffice\PhpSpreadsheet\Spreadsheet;

class ReadingImportController extends Controller
{
    public function preview(Request $request)
    {
        $validated = $request->validate([
            'file' => ['required', 'file', 'mimes:xlsx', 'max:5120'],
        ]);

        $reader = IOFactory::createReader('Xlsx');
        $reader->setReadDataOnly(true);
        $spreadsheet = $reader->load($validated['file']->getRealPath());
        [$sheet, $rows, $headerRow, $columns] = $this->locateReadingSheet($spreadsheet);
        $metadata = $this->readMetadata($rows, $validated['file']->getClientOriginalName());
        $barangay = BarangaySequence::whereRaw('LOWER(barangay) = ?', [Str::lower($metadata['barangay'])])->first();

        if (!$barangay) {
            throw ValidationException::withMessages([
                'file' => "Barangay '{$metadata['barangay']}' was not found in the database.",
            ]);
        }

        $sourceRows = $this->extractRows($rows, $headerRow, $columns);
        $fileAccounts = collect($sourceRows)->pluck('account_no')->filter()->unique()->values();
        $customerRecords = Customer::where('brgy_id', $barangay->brgy_id)
            ->when($fileAccounts->isNotEmpty(), fn ($query) => $query->orWhereIn('account_no', $fileAccounts))
            ->get();
        $customers = $customerRecords->keyBy('account_no');
        $customersBySequence = $customerRecords
            ->filter(fn ($customer) => (int) $customer->brgy_id === (int) $barangay->brgy_id)
            ->mapWithKeys(function ($customer) {
                preg_match('/(\d+)$/', $customer->account_no, $match);
                return isset($match[1]) ? [(int) $match[1] => $customer] : [];
            });

        foreach ($sourceRows as &$source) {
            $source['customer'] = $source['account_no']
                ? $customers->get($source['account_no'])
                : $customersBySequence->get($source['sequence_no']);
            $source['account_no'] = $source['account_no'] ?: $source['customer']?->account_no;
        }
        unset($source);

        $customerIds = collect($sourceRows)->pluck('customer.customer_id')->filter()->unique()->values();
        $latestReadings = Reading::whereIn('customer_id', $customerIds)
            ->orderByDesc('reading_at')
            ->orderByDesc('reading_id')
            ->get()
            ->unique('customer_id')
            ->keyBy('customer_id');
        $defaultDeviceUid = $latestReadings->pluck('device_uid')
            ->filter()
            ->countBy()
            ->sortDesc()
            ->keys()
            ->first();
        $latestBills = Bill::whereIn('customer_id', $customerIds)
            ->orderByDesc('bill_date')
            ->orderByDesc('bill_id')
            ->get()
            ->unique('customer_id')
            ->keyBy('customer_id');

        $seenAccounts = [];
        $previewRows = [];
        $importRows = [];
        $summary = [
            'total_rows' => count($sourceRows),
            'changes' => 0,
            'unchanged' => 0,
            'skipped' => 0,
            'warnings' => 0,
            'errors' => 0,
        ];

        foreach ($sourceRows as $source) {
            $accountNo = $source['account_no'];
            $customer = $source['customer'];
            $reading = $customer ? $latestReadings->get($customer->customer_id) : null;
            $bill = $customer ? $latestBills->get($customer->customer_id) : null;
            $status = 'change';
            $message = 'Ready to update';
            $importRow = null;
            $duplicateKey = $accountNo ?: "row-{$source['row']}";

            if (isset($seenAccounts[$duplicateKey])) {
                $status = 'error';
                $message = "Duplicate account; first found on row {$seenAccounts[$duplicateKey]}";
            } else {
                $seenAccounts[$duplicateKey] = $source['row'];
            }

            $previous = $this->parseInteger($source['previous']);
            $present = $this->parseInteger($source['present']);
            $bothBlank = $this->isBlank($source['previous']) && $this->isBlank($source['present']);
            $nonReadingNote = $this->nonReadingNote($source['previous'], $source['present']);

            if ($status !== 'error' && $nonReadingNote) {
                $status = 'skipped';
                $message = "Marked as {$nonReadingNote} in workbook";
            } elseif ($status !== 'error' && $bothBlank) {
                $status = 'skipped';
                $message = 'No readings in workbook';
            } elseif ($status !== 'error' && ($previous === null || $present === null)) {
                $status = 'error';
                $message = 'Previous and present readings must both be whole numbers';
            } elseif ($status !== 'error' && $present < $previous) {
                $status = 'error';
                $message = 'Present reading cannot be lower than previous reading';
            } elseif ($status !== 'error' && !$customer) {
                $status = 'error';
                $message = $source['sequence_no']
                    ? "No {$barangay->barangay} account matches sequence {$source['sequence_no']}"
                    : 'Account was not found in the database';
            } elseif ($status !== 'error' && (int) $customer->brgy_id !== (int) $barangay->brgy_id) {
                $status = 'error';
                $message = "Account does not belong to {$barangay->barangay}";
            } elseif ($status !== 'error' && !$reading && $previous === 0 && $present === 0) {
                $status = 'skipped';
                $message = 'No reading record; zero readings skipped';
            } elseif ($status !== 'error' && !$reading && !$defaultDeviceUid) {
                $status = 'error';
                $message = 'No barangay device is available for the first reading record';
            } elseif ($status !== 'error' && !$reading) {
                $nameDiffers = $this->normalizeName($source['customer_name']) !== $this->normalizeName($customer->customer_name);
                $status = $nameDiffers ? 'warning' : 'change';
                $message = $nameDiffers
                    ? "Will create first reading; name differs from database: {$customer->customer_name}"
                    : 'Will create this customer\'s first reading record';
                $importRow = [
                    'apply_reading' => true,
                    'reading_id' => null,
                    'device_uid' => $defaultDeviceUid,
                    'customer_id' => $customer->customer_id,
                    'account_no' => $accountNo,
                    'old_customer_previous' => (int) $customer->previous_reading,
                    'old_previous' => null,
                    'old_present' => null,
                    'new_previous' => $previous,
                    'new_present' => $present,
                ];
            } elseif ($status !== 'error') {
                $nameDiffers = $this->normalizeName($source['customer_name']) !== $this->normalizeName($customer->customer_name);
                $unchanged = (int) $reading->previous_reading === $previous
                    && (int) $reading->current_reading === $present;

                if ($unchanged) {
                    $status = 'unchanged';
                    $message = 'Already matches the database';
                } elseif ($nameDiffers) {
                    $status = 'warning';
                    $message = "Name differs from database: {$customer->customer_name}";
                }

                if (!$unchanged) {
                    $importRow = [
                        'apply_reading' => true,
                        'reading_id' => $reading->reading_id,
                        'customer_id' => $customer->customer_id,
                        'account_no' => $accountNo,
                        'old_customer_previous' => (int) $customer->previous_reading,
                        'old_previous' => (int) $reading->previous_reading,
                        'old_present' => (int) $reading->current_reading,
                        'new_previous' => $previous,
                        'new_present' => $present,
                    ];
                }
            }

            $filePaymentStatus = $source['has_payment_status']
                ? $this->parsePaymentStatus($source['payment_status'])
                : null;
            $databasePaymentStatus = $bill ? Str::ucfirst(Str::lower($bill->status)) : null;
            $paymentStatusChanges = $bill
                && in_array($filePaymentStatus, ['Paid', 'Pending'], true)
                && Str::lower($bill->status) !== Str::lower($filePaymentStatus);

            if ($status !== 'error' && $source['has_payment_status'] && $filePaymentStatus === null) {
                $status = 'error';
                $message = 'Payment status must be blank, Paid, Pending, Not Paid, or Not Pad';
                $importRow = null;
            } elseif ($status !== 'error' && $paymentStatusChanges) {
                $importRow ??= [
                    'apply_reading' => false,
                    'customer_id' => $customer->customer_id,
                    'account_no' => $accountNo,
                ];
                $importRow['bill_id'] = $bill->bill_id;
                $importRow['old_bill_status'] = $bill->status;
                $importRow['old_bill_total_due'] = (string) $bill->total_due;
                $importRow['new_bill_status'] = $filePaymentStatus;

                $paymentMessage = "Latest bill ({$bill->bill_date->format('M j, Y')}) will change from {$databasePaymentStatus} to {$filePaymentStatus}";
                if (in_array($status, ['skipped', 'unchanged'], true)) {
                    $status = 'change';
                    $message = $paymentMessage;
                } else {
                    $message .= "; {$paymentMessage}";
                }
            }

            if ($importRow && $status !== 'error') {
                $importRow['source_row'] = $source['row'];
                $importRows[] = $importRow;
            }

            $summary[$this->summaryKey($status)]++;
            $previewRows[] = [
                'row' => $source['row'],
                'account_no' => $accountNo,
                'customer_name' => $source['customer_name'],
                'file_previous' => $previous,
                'file_present' => $present,
                'file_usage' => ($previous !== null && $present !== null) ? $present - $previous : null,
                'database_previous' => $reading ? (int) $reading->previous_reading : null,
                'database_present' => $reading ? (int) $reading->current_reading : null,
                'bill_date' => $bill?->bill_date?->format('Y-m-d'),
                'database_payment_status' => $databasePaymentStatus,
                'file_payment_status' => $filePaymentStatus === 'Ignored' ? null : $filePaymentStatus,
                'payment_status_changes' => $paymentStatusChanges,
                'status' => $status,
                'message' => $message,
            ];
        }

        $token = null;
        $expiresAt = null;
        if (count($importRows) > 0) {
            $token = (string) Str::uuid();
            $expiresAt = now()->addMinutes(15);
            $request->session()->put("reading_import.{$token}", [
                'expires_at' => $expiresAt->timestamp,
                'barangay' => $barangay->barangay,
                'billing_month' => $metadata['month']?->format('Y-m'),
                'source_file' => $validated['file']->getClientOriginalName(),
                'rows' => $importRows,
            ]);
        }

        return response()->json([
            'token' => $token,
            'expires_at' => $expiresAt?->toIso8601String(),
            'file_name' => $validated['file']->getClientOriginalName(),
            'sheet_name' => $sheet->getTitle(),
            'barangay' => $barangay->barangay,
            'billing_month' => $metadata['month']?->format('Y-m'),
            'billing_month_label' => $metadata['month']?->format('F Y') ?? 'Not specified',
            'summary' => $summary,
            'rows' => $previewRows,
        ]);
    }

    public function confirm(Request $request)
    {
        $validated = $request->validate([
            'token' => ['required', 'uuid'],
            'selected_accounts' => ['required', 'array', 'min:1', 'max:2000'],
            'selected_accounts.*' => ['required', 'string', 'distinct'],
        ]);

        $sessionKey = "reading_import.{$validated['token']}";
        $payload = $request->session()->pull($sessionKey);

        if (!$payload || ($payload['expires_at'] ?? 0) < now()->timestamp) {
            throw ValidationException::withMessages([
                'token' => 'This import preview has expired. Upload the workbook again.',
            ]);
        }

        $rowsByAccount = collect($payload['rows'])->keyBy('account_no');
        $selectedRows = collect($validated['selected_accounts'])->map(function ($accountNo) use ($rowsByAccount) {
            $row = $rowsByAccount->get($accountNo);
            if (!$row) {
                throw ValidationException::withMessages([
                    'selected_accounts' => "Account {$accountNo} is not an applicable row from this preview.",
                ]);
            }
            return $row;
        })->values();

        $sourceFile = $payload['source_file'] ?? 'workbook import';
        $userId = auth()->id();

        $result = DB::transaction(function () use ($selectedRows, $sourceFile, $userId) {
            $updated = 0;
            $created = 0;
            $billStatusesUpdated = 0;

            foreach ($selectedRows as $row) {
                $customer = Customer::whereKey($row['customer_id'])->lockForUpdate()->first();
                if (!$customer) {
                    throw ValidationException::withMessages([
                        'token' => "Customer {$row['account_no']} changed after preview. Upload the workbook again.",
                    ]);
                }

                if ($row['apply_reading']) {
                    $reading = $row['reading_id']
                        ? Reading::whereKey($row['reading_id'])->lockForUpdate()->first()
                        : null;

                    $customerChanged = (int) $customer->previous_reading !== (int) $row['old_customer_previous'];
                    $readingChanged = $row['reading_id']
                        ? (!$reading
                            || (int) $reading->customer_id !== (int) $row['customer_id']
                            || (int) $reading->previous_reading !== (int) $row['old_previous']
                            || (int) $reading->current_reading !== (int) $row['old_present'])
                        : Reading::where('customer_id', $row['customer_id'])
                            ->orderByDesc('reading_at')
                            ->orderByDesc('reading_id')
                            ->lockForUpdate()
                            ->exists();

                    if ($customerChanged || $readingChanged) {
                        throw ValidationException::withMessages([
                            'token' => "Reading {$row['account_no']} changed after preview. Upload the workbook again.",
                        ]);
                    }

                    $customer->previous_reading = $row['new_previous'];
                    $customer->Synced = false;
                    $customer->save();

                    if (!$reading) {
                        $reading = new Reading();
                        $reading->customer_id = $customer->customer_id;
                        $reading->customer_account_number = $customer->account_no;
                        $reading->device_uid = $row['device_uid'];
                        $reading->reading_at = now();
                        $created++;
                    } else {
                        $updated++;
                    }

                    $reading->previous_reading = $row['new_previous'];
                    $reading->current_reading = $row['new_present'];
                    $reading->usage_m3 = $row['new_present'] - $row['new_previous'];
                    $reading->Synced = false;
                    $reading->save();
                }

                if (!empty($row['bill_id'])) {
                    $bill = Bill::whereKey($row['bill_id'])->lockForUpdate()->first();
                    if (!$bill
                        || (int) $bill->customer_id !== (int) $row['customer_id']
                        || Str::lower($bill->status) !== Str::lower($row['old_bill_status'])
                        || (string) $bill->total_due !== (string) $row['old_bill_total_due']) {
                        throw ValidationException::withMessages([
                            'token' => "Bill status for {$row['account_no']} changed after preview. Upload the workbook again.",
                        ]);
                    }

                    $newStatus = $row['new_bill_status'];
                    $bill->status = $newStatus;
                    if (Str::lower($newStatus) === 'paid') {
                        $bill->amount_paid = $bill->total_due;
                        $bill->balance_due = 0;
                    } elseif (Str::lower($newStatus) === 'void') {
                        $bill->amount_paid = 0;
                        $bill->balance_due = 0;
                    } else {
                        $bill->amount_paid = 0;
                        $bill->balance_due = $bill->total_due;
                    }
                    $bill->Synced = false;
                    $bill->save();

                    BillTransaction::create([
                        'bill_id' => $bill->bill_id,
                        'bill_reference_number' => $bill->reference_number,
                        'device_uid' => $bill->device_uid,
                        'type' => $newStatus === 'Paid' ? 'payment' : 'void',
                        'source' => 'Office',
                        'amount' => $bill->total_due,
                        'cash_received' => $newStatus === 'Paid' ? $bill->total_due : null,
                        'change' => 0,
                        'transaction_date' => now(),
                        'payment_method' => 'workbook_import',
                        'processed_by_user_id' => $userId,
                        'notes' => "Status changed from {$row['old_bill_status']} to {$newStatus} from {$sourceFile}, row {$row['source_row']}.",
                        'Synced' => false,
                    ]);
                    $billStatusesUpdated++;
                }
            }

            return [
                'updated' => $updated,
                'created' => $created,
                'bill_statuses_updated' => $billStatusesUpdated,
            ];
        });

        $applied = $selectedRows->count();

        return response()->json([
            'applied' => $applied,
            'updated' => $result['updated'],
            'created' => $result['created'],
            'bill_statuses_updated' => $result['bill_statuses_updated'],
            'barangay' => $payload['barangay'],
            'billing_month' => $payload['billing_month'],
            'message' => "{$applied} workbook rows were applied.",
        ]);
    }

    private function locateReadingSheet(Spreadsheet $spreadsheet): array
    {
        foreach ($spreadsheet->getWorksheetIterator() as $sheet) {
            $rows = $sheet->toArray(null, true, true, true);
            $columns = $this->locateColumns($rows);

            if ($columns !== null) {
                return [$sheet, $rows, $columns[0], $columns[1]];
            }
        }

        throw ValidationException::withMessages([
            'file' => 'No worksheet contains Account Number, Previous Reading, and Present Reading columns.',
        ]);
    }

    private function locateColumns(array $rows): ?array
    {
        foreach (array_slice($rows, 0, 75, true) as $rowNumber => $row) {
            $mapped = [];
            foreach ($row as $column => $value) {
                $normalized = $this->normalizeHeader($value);
                if ($this->matchesHeader($normalized, ['accountnumber', 'accountno', 'account', 'accountid', 'meternumber', 'meterno'])) $mapped['account'] = $column;
                if ($this->matchesHeader($normalized, ['customername', 'clientname', 'concessionairename', 'name'])) $mapped['name'] = $column;
                if ($this->matchesHeader($normalized, ['previousreading', 'prevreading', 'previous', 'prev', 'oldreading', 'oldmeterreading'])) $mapped['previous'] = $column;
                if ($this->matchesHeader($normalized, ['presentreading', 'currentreading', 'present', 'current', 'newreading', 'newmeterreading'])) $mapped['present'] = $column;
                if ($this->matchesHeader($normalized, ['paymentstatus', 'billstatus', 'status'])) $mapped['status'] = $column;
            }

            if (isset($mapped['previous'], $mapped['present']) && !isset($mapped['account'])) {
                $mapped = $this->inferLegacyColumns($rows, (int) $rowNumber, $mapped);
            }

            if (isset($mapped['previous'], $mapped['present'])
                && (isset($mapped['account']) || isset($mapped['sequence'], $mapped['name']))) {
                return [(int) $rowNumber, $mapped];
            }
        }

        return null;
    }

    private function inferLegacyColumns(array $rows, int $headerRow, array $mapped): array
    {
        foreach ($rows as $rowNumber => $row) {
            if ((int) $rowNumber <= $headerRow || (int) $rowNumber > $headerRow + 12) continue;

            $sequenceColumn = null;
            $nameColumn = null;

            foreach ($row as $column => $value) {
                if (in_array($column, [$mapped['previous'], $mapped['present']], true)) continue;

                if ($sequenceColumn === null && $this->parseInteger($value) !== null) {
                    $sequenceColumn = $column;
                } elseif ($nameColumn === null && is_string($value) && trim($value) !== '') {
                    $nameColumn = $column;
                }
            }

            if ($sequenceColumn !== null && $nameColumn !== null) {
                $mapped['sequence'] = $sequenceColumn;
                $mapped['name'] = $nameColumn;
                return $mapped;
            }
        }

        return $mapped;
    }

    private function matchesHeader(string $header, array $aliases): bool
    {
        foreach ($aliases as $alias) {
            if ($header === $alias || str_starts_with($header, $alias)) {
                return true;
            }
        }

        return false;
    }

    private function readMetadata(array $rows, string $fileName): array
    {
        $heading = collect(array_slice($rows, 0, 12, true))
            ->flatMap(fn ($row) => array_values($row))
            ->filter(fn ($value) => is_string($value) && trim($value) !== '')
            ->implode(' | ');

        preg_match('/Barangay:\s*(.*?)\s+.+?\s+Billing Month:/i', $heading, $barangayMatch);
        preg_match('/Billing Month:\s*([A-Za-z]+)(?:\s+\d{1,2}(?:\s*-\s*\d{1,2})?\s*,?)?\s+(20\d{2})/i', $heading, $monthMatch);

        $barangay = trim($barangayMatch[1] ?? '');
        $month = null;

        if (!empty($monthMatch[1]) && !empty($monthMatch[2])) {
            try {
                $month = Carbon::createFromFormat('F Y', trim("{$monthMatch[1]} {$monthMatch[2]}"))->startOfMonth();
            } catch (\Throwable) {
                $month = null;
            }
        }

        if (!$month && preg_match('/(20\d{2})-(0[1-9]|1[0-2])/', $fileName, $fileMonth)) {
            $month = Carbon::createFromFormat('Y-m', "{$fileMonth[1]}-{$fileMonth[2]}")->startOfMonth();
        }

        if (!$month && preg_match('/\b(January|February|March|April|May|June|July|August|September|October|November|December)\b.*?\b(20\d{2})\b/i', $fileName, $fileMonth)) {
            $month = Carbon::parse("1 {$fileMonth[1]} {$fileMonth[2]}")->startOfMonth();
        }

        if ($barangay === '') {
            $normalizedFileName = $this->normalizeHeader(pathinfo($fileName, PATHINFO_FILENAME));
            $barangay = BarangaySequence::all()
                ->first(fn ($item) => str_contains($normalizedFileName, $this->normalizeHeader($item->barangay)))
                ?->barangay ?? '';
        }

        if ($barangay === '') {
            throw ValidationException::withMessages(['file' => 'The workbook barangay could not be detected.']);
        }
        return ['barangay' => $barangay, 'month' => $month];
    }

    private function extractRows(array $rows, int $headerRow, array $columns): array
    {
        $result = [];
        foreach ($rows as $rowNumber => $row) {
            if ((int) $rowNumber <= $headerRow) continue;

            $accountNo = isset($columns['account'])
                ? Str::upper(trim((string) ($row[$columns['account']] ?? '')))
                : null;
            $sequenceNo = isset($columns['sequence'])
                ? $this->parseInteger($row[$columns['sequence']] ?? null)
                : null;
            $customerName = trim((string) ($row[$columns['name'] ?? ''] ?? ''));

            if ($accountNo !== null) {
                if ($accountNo === '') continue;
                if ($this->normalizeHeader($accountNo) === 'summary') break;
                if (!preg_match('/^[A-Z0-9]+(?:-[A-Z0-9]+)+$/', $accountNo)) continue;
            } elseif ($sequenceNo === null || $customerName === '') {
                continue;
            }

            $result[] = [
                'row' => (int) $rowNumber,
                'account_no' => $accountNo,
                'sequence_no' => $sequenceNo,
                'customer_name' => $customerName,
                'previous' => $row[$columns['previous']] ?? null,
                'present' => $row[$columns['present']] ?? null,
                'has_payment_status' => isset($columns['status']),
                'payment_status' => isset($columns['status']) ? $row[$columns['status']] ?? null : null,
            ];
        }

        if (empty($result)) {
            throw ValidationException::withMessages(['file' => 'No customer reading rows were found.']);
        }

        return $result;
    }

    private function parseInteger(mixed $value): ?int
    {
        if ($this->isBlank($value) || !is_numeric($value)) return null;
        $number = (float) $value;
        if ($number < 0 || floor($number) !== $number) return null;
        return (int) $number;
    }

    private function isBlank(mixed $value): bool
    {
        return $value === null || (is_string($value) && trim($value) === '');
    }

    private function nonReadingNote(mixed ...$values): ?string
    {
        foreach ($values as $value) {
            if (!is_string($value)) continue;

            $normalized = $this->normalizeHeader($value);
            if ($normalized === 'stopmeter' || $normalized === 'stoppedmeter') return 'stopped meter';
            if (in_array($normalized, ['closed', 'disconnected', 'inactive'], true)) return $normalized;
        }

        return null;
    }

    private function parsePaymentStatus(mixed $value): ?string
    {
        $normalized = $this->normalizeHeader($value);

        if ($normalized === '' || $normalized === 'paid') return 'Paid';
        if (in_array($normalized, ['pending', 'due', 'unpaid', 'notpaid', 'notpad'], true)) return 'Pending';
        if (in_array($normalized, ['nobill', 'void', 'closed', 'disconnected', 'inactive', 'stopmeter'], true)) return 'Ignored';

        return null;
    }

    private function normalizeHeader(mixed $value): string
    {
        return preg_replace('/[^a-z0-9]+/', '', Str::lower(trim((string) $value)));
    }

    private function normalizeName(?string $value): string
    {
        return preg_replace('/[^a-z0-9]+/', '', Str::lower((string) $value));
    }

    private function summaryKey(string $status): string
    {
        return match ($status) {
            'change' => 'changes',
            'warning' => 'warnings',
            'unchanged' => 'unchanged',
            'skipped' => 'skipped',
            default => 'errors',
        };
    }
}
