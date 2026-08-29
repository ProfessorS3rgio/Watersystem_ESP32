<?php

namespace App\Http\Controllers;

use App\Models\BarangaySequence;
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
            $status = 'change';
            $message = 'Ready to update';
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

            if ($status !== 'error' && $bothBlank) {
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
            } elseif ($status !== 'error' && !$reading) {
                $status = 'error';
                $message = 'This customer does not have a reading record to update';
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
                    $importRows[] = [
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
                'status' => $status,
                'message' => $message,
            ];
        }

        $token = null;
        $expiresAt = null;
        if ($summary['errors'] === 0 && count($importRows) > 0) {
            $token = (string) Str::uuid();
            $expiresAt = now()->addMinutes(15);
            $request->session()->put("reading_import.{$token}", [
                'expires_at' => $expiresAt->timestamp,
                'barangay' => $barangay->barangay,
                'billing_month' => $metadata['month']->format('Y-m'),
                'rows' => $importRows,
            ]);
        }

        return response()->json([
            'token' => $token,
            'expires_at' => $expiresAt?->toIso8601String(),
            'file_name' => $validated['file']->getClientOriginalName(),
            'sheet_name' => $sheet->getTitle(),
            'barangay' => $barangay->barangay,
            'billing_month' => $metadata['month']->format('Y-m'),
            'billing_month_label' => $metadata['month']->format('F Y'),
            'summary' => $summary,
            'rows' => $previewRows,
        ]);
    }

    public function confirm(Request $request)
    {
        $validated = $request->validate([
            'token' => ['required', 'uuid'],
        ]);

        $sessionKey = "reading_import.{$validated['token']}";
        $payload = $request->session()->pull($sessionKey);

        if (!$payload || ($payload['expires_at'] ?? 0) < now()->timestamp) {
            throw ValidationException::withMessages([
                'token' => 'This import preview has expired. Upload the workbook again.',
            ]);
        }

        $updated = DB::transaction(function () use ($payload) {
            $updated = 0;

            foreach ($payload['rows'] as $row) {
                $customer = Customer::whereKey($row['customer_id'])->lockForUpdate()->first();
                $reading = Reading::whereKey($row['reading_id'])->lockForUpdate()->first();

                if (!$customer
                    || !$reading
                    || (int) $reading->customer_id !== (int) $row['customer_id']
                    || (int) $customer->previous_reading !== (int) $row['old_customer_previous']
                    || (int) $reading->previous_reading !== (int) $row['old_previous']
                    || (int) $reading->current_reading !== (int) $row['old_present']) {
                    throw ValidationException::withMessages([
                        'token' => "Reading {$row['account_no']} changed after preview. Upload the workbook again.",
                    ]);
                }

                $customer->previous_reading = $row['new_previous'];
                $customer->Synced = false;
                $customer->save();

                $reading->previous_reading = $row['new_previous'];
                $reading->current_reading = $row['new_present'];
                $reading->usage_m3 = $row['new_present'] - $row['new_previous'];
                $reading->Synced = false;
                $reading->save();
                $updated++;
            }

            return $updated;
        });

        return response()->json([
            'updated' => $updated,
            'barangay' => $payload['barangay'],
            'billing_month' => $payload['billing_month'],
            'message' => "{$updated} reading records were updated.",
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
        preg_match('/Billing Month:\s*([A-Za-z]+\s+\d{4})/i', $heading, $monthMatch);

        $barangay = trim($barangayMatch[1] ?? '');
        $month = null;

        if (!empty($monthMatch[1])) {
            try {
                $month = Carbon::createFromFormat('F Y', trim($monthMatch[1]))->startOfMonth();
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
        if (!$month) {
            throw ValidationException::withMessages(['file' => 'The workbook billing month could not be detected.']);
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
