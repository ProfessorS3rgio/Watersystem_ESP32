<?php
namespace App\Services;

use App\Models\{Bill, BillTransaction, Customer, Reading};
use Carbon\Carbon;
use Illuminate\Support\Facades\{DB, Validator};

/** Builds a read-only import plan. Only a saved, approved plan may be applied. */
class SyncVerificationService
{
    public function validate(array $input): array
    {
        $payload = Validator::make($input, [
            'device' => 'required|array', 'device.device_uid' => 'required|string|max:100',
            'device.brgy_id' => 'required|integer|exists:barangay_sequence,brgy_id',
            'device.firmware_version' => 'nullable|string|max:100', 'device.verified_sync_ack' => 'nullable|integer',
            'readings' => 'present|array|max:5000', 'bills' => 'present|array|max:5000', 'transactions' => 'present|array|max:5000',
            'readings.*.reading_id' => 'required|integer|min:1', 'readings.*.device_uid' => 'nullable|string|max:100',
            'readings.*.customer_account_number' => 'required|string|max:100',
            'readings.*.previous_reading' => 'required|integer', 'readings.*.current_reading' => 'required|integer',
            'readings.*.usage_m3' => 'required|integer', 'readings.*.reading_at' => 'required|integer',
            'bills.*.bill_id' => 'required|integer|min:1', 'bills.*.reference_number' => 'required|string|max:100',
            'bills.*.device_uid' => 'nullable|string|max:100', 'bills.*.reading_id' => 'required|integer|min:1',
            'bills.*.customer_account_number' => 'required|string|max:100', 'bills.*.bill_date' => 'required|string|max:30',
            'bills.*.rate_per_m3' => 'required|numeric', 'bills.*.charges' => 'required|numeric',
            'bills.*.penalty' => 'required|numeric', 'bills.*.total_due' => 'required|numeric', 'bills.*.status' => 'required|string|max:30',
            'transactions.*.bill_transaction_id' => 'required|integer|min:1', 'transactions.*.bill_reference_number' => 'required|string|max:100',
            'transactions.*.processed_by_device_uid' => 'nullable|string|max:100',
            'transactions.*.type' => 'required|string|max:30', 'transactions.*.source' => 'required|string|max:30',
            'transactions.*.amount' => 'required|numeric', 'transactions.*.cash_received' => 'nullable|numeric',
            'transactions.*.change' => 'required|numeric', 'transactions.*.transaction_date' => 'required|string|max:30',
            'transactions.*.payment_method' => 'required|string|max:100', 'transactions.*.notes' => 'nullable|string|max:4000',
        ])->validate();
        foreach (['readings' => ['reading_id', 'previous_reading', 'current_reading', 'usage_m3', 'reading_at'], 'bills' => ['bill_id', 'reading_id'], 'transactions' => ['bill_transaction_id']] as $kind => $fields) {
            foreach ($payload[$kind] as &$row) foreach ($fields as $field) $row[$field] = (int) $row[$field];
            unset($row);
        }
        return $payload;
    }

    private function cents($number): int { return (int) round((float) $number * 100); }
    private function date(string $value, string $format): bool {
        $date = \DateTimeImmutable::createFromFormat('!'.$format, $value);
        return $date && $date->format($format) === $value;
    }

    public function preview(array $payload, bool $lock = false): array
    {
        $uid = $payload['device']['device_uid'];
        $accounts = collect([...$payload['readings'], ...$payload['bills']])->pluck('customer_account_number')->unique()->values()->all();
        $refs = collect([...$payload['bills'], ...$payload['transactions']])->map(fn ($r) => $r['reference_number'] ?? $r['bill_reference_number'])->unique()->values()->all();
        $accounts = array_values(array_unique([...$accounts, ...DB::table('bill')->whereIn('reference_number', $refs)->pluck('customer_account_number')->all()]));
        $queries = [
            'customers' => DB::table('customer')->whereIn('account_no', $accounts)->orderBy('customer_id'),
            'readings' => DB::table('reading')->where(fn ($q) => $q->whereIn('customer_account_number', $accounts)->orWhere('device_uid', $uid))->orderBy('reading_id'),
            'bills' => DB::table('bill')->where(fn ($q) => $q->whereIn('customer_account_number', $accounts)->orWhereIn('reference_number', $refs))->orderBy('bill_id'),
            'transactions' => DB::table('bill_transaction')->where(fn ($q) => $q->whereIn('bill_reference_number', $refs)->orWhere('processed_by_device_uid', $uid))->orderBy('bill_transaction_id'),
            'settings' => DB::table('settings')->orderBy('id'),
            'devices' => DB::table('device')->where('device_uid', $uid)->orderBy('device_uid'),
        ];
        $state = [];
        foreach ($queries as $key => $query) $state[$key] = ($lock ? $query->lockForUpdate() : $query)->get();
        $customers = $state['customers']->keyBy('account_no');
        $readings = $state['readings']; $bills = $state['bills']->keyBy('reference_number'); $transactions = $state['transactions'];
        $rows = []; $seen = []; $readingMap = []; $billMap = [];
        $add = function (string $kind, array $raw, string $key, string $label) use (&$rows, &$seen) {
            $index = count($rows);
            $rows[] = ['kind' => $kind, 'key' => $key, 'label' => $label, 'account' => $raw['customer_account_number'] ?? '', 'status' => 'ready', 'issues' => [], 'warnings' => [], 'incoming' => $raw, 'existing_id' => null];
            if (isset($seen[$kind.':'.$key])) {
                $rows[$index]['issues'][] = 'Repeated identity in this device export.';
                $rows[$seen[$kind.':'.$key]]['issues'][] = 'Repeated identity in this device export.';
            }
            $seen[$kind.':'.$key] = $index;
            return $index;
        };
        $customerCheck = function (int $i, array $raw) use (&$rows, $customers, $payload) {
            $customer = $customers->get($raw['customer_account_number']);
            $rows[$i]['customer_name'] = $customer?->customer_name ?? 'Unknown customer';
            $rows[$i]['customer_id'] = $customer?->customer_id;
            if (!$customer) $rows[$i]['issues'][] = 'Customer account was not found on the website.';
            elseif ((int) $customer->brgy_id !== (int) $payload['device']['brgy_id']) $rows[$i]['issues'][] = 'Customer belongs to a different barangay than this master.';
        };
        $moneyCheck = function (int $i, array $raw, array $fields) use (&$rows) {
            foreach ($fields as $field) if (isset($raw[$field]) && (!is_finite((float) $raw[$field]) || (float) $raw[$field] < 0 || (float) $raw[$field] > 99999999.99)) $rows[$i]['issues'][] = "Invalid {$field} amount.";
        };
        foreach ($payload['readings'] as $raw) {
            $i = $add('reading', $raw, (string) $raw['reading_id'], 'Reading #'.$raw['reading_id']);
            $customerCheck($i, $raw);
            if (($raw['device_uid'] ?? $uid) !== $uid) $rows[$i]['issues'][] = 'Reading belongs to another device.';
            if ($raw['previous_reading'] < 0 || $raw['current_reading'] < $raw['previous_reading'] || $raw['usage_m3'] !== $raw['current_reading'] - $raw['previous_reading']) $rows[$i]['issues'][] = 'Current reading, previous reading, and usage do not agree.';
            if ($raw['reading_at'] < 1577836800 || $raw['reading_at'] > now()->addDay()->timestamp) $rows[$i]['issues'][] = 'Reading date is missing or the device clock is incorrect.';
            $existing = $readings->first(fn ($r) => $r->device_uid === $uid && (int) $r->device_reading_id === (int) $raw['reading_id']);
            $same = fn ($r) => $r->customer_account_number === $raw['customer_account_number'] && (int) $r->previous_reading === (int) $raw['previous_reading'] && (int) $r->current_reading === (int) $raw['current_reading'] && (int) $r->usage_m3 === (int) $raw['usage_m3'] && Carbon::parse($r->reading_at, 'Asia/Manila')->timestamp === (int) $raw['reading_at'];
            if ($existing) {
                $rows[$i]['existing_id'] = $existing->reading_id;
                $rows[$i]['before'] = ['previous_reading' => $existing->previous_reading, 'current_reading' => $existing->current_reading, 'usage_m3' => $existing->usage_m3];
                if ($same($existing)) $rows[$i]['status'] = 'already_saved';
                else $rows[$i]['issues'][] = 'This device reading ID already exists with different values.';
            } else {
                $legacy = $readings->filter($same);
                if ($legacy->count() === 1 && !$legacy->first()->device_reading_id && $legacy->first()->device_uid === $uid) {
                    $rows[$i]['existing_id'] = $legacy->first()->reading_id;
                    $rows[$i]['warnings'][] = 'Existing historical reading will be linked to this device ID; no duplicate reading will be created.';
                } elseif ($legacy->isNotEmpty()) $rows[$i]['issues'][] = 'This meter transition is already recorded with another identity.';
                $history = $readings->where('customer_account_number', $raw['customer_account_number'])->sortByDesc('reading_at')->take(3);
                $average = (float) $history->avg('usage_m3');
                if ($raw['usage_m3'] > max(50, $average * 3)) $rows[$i]['warnings'][] = 'Unusually high usage. Confirm the meter reading before approving.';
                if ($history->isNotEmpty() && Carbon::parse($history->first()->reading_at, 'Asia/Manila')->timestamp > $raw['reading_at']) $rows[$i]['warnings'][] = 'Historical reading: newer readings already exist.';
            }
            $readingMap[$raw['reading_id']] = $i;
        }
        $transitions = []; $priorIncoming = [];
        $orderedReadings = collect($readingMap)->sortBy(fn ($i) => $rows[$i]['incoming']['reading_at']);
        foreach ($orderedReadings as $i) {
            $raw = $rows[$i]['incoming']; $account = $rows[$i]['account'];
            $transition = implode(':', [$account, $raw['reading_at'], $raw['previous_reading'], $raw['current_reading']]);
            if (isset($transitions[$transition])) {
                $rows[$i]['issues'][] = 'Repeated reading at the same time with a different device ID.';
                $rows[$transitions[$transition]]['issues'][] = 'Repeated reading at the same time with a different device ID.';
            }
            $transitions[$transition] = $i;
            if ($rows[$i]['status'] !== 'already_saved') {
                $prior = $readings->filter(fn ($r) => $r->customer_account_number === $account && Carbon::parse($r->reading_at, 'Asia/Manila')->timestamp < $raw['reading_at'])->sortByDesc('reading_at')->first();
                $previous = $prior ? ['at' => Carbon::parse($prior->reading_at, 'Asia/Manila')->timestamp, 'current' => $prior->current_reading] : null;
                if (isset($priorIncoming[$account]) && (!$previous || $priorIncoming[$account]['at'] >= $previous['at'])) $previous = $priorIncoming[$account];
                if ($previous && (int) $previous['current'] !== $raw['previous_reading']) $rows[$i]['issues'][] = 'Previous meter reading does not continue from the preceding recorded reading.';
                if (!$previous && $customers->has($account) && (int) $customers[$account]->previous_reading !== $raw['previous_reading']) $rows[$i]['warnings'][] = 'Previous reading differs from the customer’s opening meter value. Confirm this historical reading.';
            }
            $priorIncoming[$account] = ['at' => $raw['reading_at'], 'current' => $raw['current_reading']];
        }
        $periods = [];
        foreach ($payload['bills'] as $raw) {
            $i = $add('bill', $raw, $raw['reference_number'], $raw['reference_number']);
            $customerCheck($i, $raw); $moneyCheck($i, $raw, ['rate_per_m3', 'charges', 'penalty', 'total_due']);
            if (($raw['device_uid'] ?? $uid) !== $uid) $rows[$i]['issues'][] = 'Bill belongs to another device.';
            if (!$this->date($raw['bill_date'], 'Y-m-d')) $rows[$i]['issues'][] = 'Invalid bill date.';
            elseif ($raw['bill_date'] < '2020-01-01' || $raw['bill_date'] > now('Asia/Manila')->addDay()->toDateString()) $rows[$i]['issues'][] = 'Bill date is outside the valid device clock range.';
            if ($this->cents($raw['charges']) + $this->cents($raw['penalty']) !== $this->cents($raw['total_due'])) $rows[$i]['issues'][] = 'Charges plus penalty do not equal the total due.';
            if (!in_array($raw['status'], ['Pending', 'Due', 'Paid', 'Partially Paid', 'Void'], true)) $rows[$i]['issues'][] = 'Unknown bill status.';
            $readingIndex = $readingMap[$raw['reading_id']] ?? null;
            $reading = $readings->first(fn ($r) => $r->device_uid === $uid && (int) $r->device_reading_id === (int) $raw['reading_id']);
            $account = $readingIndex !== null ? $rows[$readingIndex]['account'] : $reading?->customer_account_number;
            if (!$account || $account !== $raw['customer_account_number']) $rows[$i]['issues'][] = 'Linked reading is missing or belongs to another customer.';
            $rows[$i]['reading_device_id'] = $raw['reading_id'];
            $existing = $bills->get($raw['reference_number']);
            if ($existing) {
                $rows[$i]['existing_id'] = $existing->bill_id;
                $rows[$i]['before'] = ['status' => $existing->status, 'total_due' => $existing->total_due, 'amount_paid' => $existing->amount_paid];
                $expectedReadingId = $readingIndex !== null ? $rows[$readingIndex]['existing_id'] : $reading?->reading_id;
                if ($existing->customer_account_number !== $raw['customer_account_number'] || $existing->device_uid !== $uid || !$expectedReadingId || (int) $existing->reading_id !== (int) $expectedReadingId || substr($existing->bill_date, 0, 10) !== $raw['bill_date']) $rows[$i]['issues'][] = 'Bill reference is already linked to different records.';
                foreach (['rate_per_m3', 'charges', 'penalty', 'total_due'] as $field) if ($this->cents($existing->$field) !== $this->cents($raw[$field])) $rows[$i]['issues'][] = "Existing bill has a different {$field}.";
                if ($existing->status === $raw['status'] || ($existing->status === 'Due' && $raw['status'] === 'Pending')) $rows[$i]['status'] = 'already_saved';
                elseif (!in_array($raw['status'], ['Paid', 'Partially Paid'], true) || in_array($existing->status, ['Paid', 'Void'], true)) $rows[$i]['issues'][] = 'Bill status conflicts with the website. Resolve it before syncing.';
            } else {
                $period = $raw['customer_account_number'].':'.substr($raw['bill_date'], 0, 7);
                if (isset($periods[$period]) || $bills->contains(fn ($b) => $b->customer_account_number === $raw['customer_account_number'] && substr($b->bill_date, 0, 7) === substr($raw['bill_date'], 0, 7) && $b->status !== 'Void')) $rows[$i]['issues'][] = 'Another bill already covers this customer and billing month.';
                if (isset($periods[$period])) $rows[$periods[$period]]['issues'][] = 'Another bill in this batch covers the same billing month.';
                $periods[$period] = $i;
            }
            $rows[$i]['due_date'] = $this->date($raw['bill_date'], 'Y-m-d') ? Carbon::parse($raw['bill_date'])->addDays(max(0, (int) ($state['settings']->first()->bill_due_days ?? 5)))->toDateString() : null;
            $billMap[$raw['reference_number']] = $i;
        }
        $paymentDeltas = []; $paymentEvents = [];
        foreach ($payload['transactions'] as $raw) {
            $i = $add('payment', $raw, (string) $raw['bill_transaction_id'], 'Transaction #'.$raw['bill_transaction_id']);
            $eventKey = implode(':', [$raw['bill_reference_number'], $raw['transaction_date'], $this->cents($raw['amount'])]);
            if (isset($paymentEvents[$eventKey])) {
                $rows[$i]['issues'][] = 'Repeated payment details in this batch. Confirm these are separate transactions.';
                $rows[$paymentEvents[$eventKey]]['issues'][] = 'Repeated payment details in this batch. Confirm these are separate transactions.';
            }
            $paymentEvents[$eventKey] = $i;
            $moneyCheck($i, $raw, ['amount', 'cash_received', 'change']);
            if (($raw['processed_by_device_uid'] ?? $uid) !== $uid) $rows[$i]['issues'][] = 'Payment belongs to another device.';
            if (!$this->date($raw['transaction_date'], 'Y-m-d H:i:s')) $rows[$i]['issues'][] = 'Invalid payment date.';
            elseif ($raw['transaction_date'] < '2020-01-01 00:00:00' || Carbon::parse($raw['transaction_date'], 'Asia/Manila')->timestamp > now()->addDay()->timestamp) $rows[$i]['issues'][] = 'Payment date is outside the valid device clock range.';
            if (!in_array($raw['type'], ['payment', 'void'], true) || $raw['source'] !== 'Device') $rows[$i]['issues'][] = 'Unsupported transaction type or source.';
            if ($raw['type'] === 'payment' && isset($raw['cash_received']) && $this->cents($raw['cash_received']) - $this->cents($raw['change']) !== $this->cents($raw['amount'])) $rows[$i]['issues'][] = 'Cash received minus change does not equal the payment.';
            $ref = $raw['bill_reference_number']; $bill = $bills->get($ref); $billIndex = $billMap[$ref] ?? null;
            $rows[$i]['account'] = $billIndex !== null ? $rows[$billIndex]['account'] : ($bill?->customer_account_number ?? '');
            $customer = $customers->get($rows[$i]['account']) ?? ($bill ? DB::table('customer')->where('customer_id', $bill->customer_id)->first() : null);
            $rows[$i]['customer_name'] = $customer?->customer_name ?? 'Unknown customer';
            if (!$customer) $rows[$i]['issues'][] = 'Payment customer was not found.';
            if (!$bill && $billIndex === null) $rows[$i]['issues'][] = 'Referenced bill was not found in the website or this batch.';
            if ($customer && (int) $customer->brgy_id !== (int) $payload['device']['brgy_id']) $rows[$i]['issues'][] = 'Payment customer belongs to another barangay.';
            if ($bill && $bill->device_uid !== $uid) $rows[$i]['issues'][] = 'Referenced bill belongs to another device.';
            $identity = $transactions->first(fn ($t) => $t->processed_by_device_uid === $uid && (int) $t->device_transaction_id === (int) $raw['bill_transaction_id']);
            $legacy = $transactions->filter(fn ($t) => $t->bill_reference_number === $ref && $t->transaction_date === $raw['transaction_date'] && $this->cents($t->amount) === $this->cents($raw['amount']));
            $existing = $identity ?? ($legacy->count() === 1 ? $legacy->first() : null);
            if ($legacy->count() > 1 && !$identity) $rows[$i]['issues'][] = 'Several existing transactions match this payment.';
            if ($existing) {
                $rows[$i]['existing_id'] = $existing->bill_transaction_id;
                $same = $existing->bill_reference_number === $ref && $existing->transaction_date === $raw['transaction_date'] && $existing->type === $raw['type'] && $existing->source === $raw['source'] && $existing->processed_by_device_uid === $uid && $existing->payment_method === $raw['payment_method'] && (string) $existing->notes === (string) ($raw['notes'] ?? '');
                foreach (['amount', 'cash_received', 'change'] as $field) if ($this->cents($existing->$field) !== $this->cents($raw[$field] ?? 0)) $same = false;
                if (!$same) $rows[$i]['issues'][] = 'Existing transaction has different values. It will not be overwritten.';
                elseif ($existing->device_transaction_id) $rows[$i]['status'] = 'already_saved';
                else $rows[$i]['warnings'][] = 'Existing historical transaction will be linked to this device ID.';
            } elseif ($raw['type'] === 'payment') {
                if ($bill && $bill->status === 'Paid') {
                    $rows[$i]['history_only'] = true;
                    $rows[$i]['warnings'][] = 'This bill is already paid. The missing payment history will be added without increasing its paid balance.';
                } else $paymentDeltas[$ref] = ($paymentDeltas[$ref] ?? 0) + $this->cents($raw['amount']);
            }
        }
        foreach (collect($rows)->where('kind', 'payment')->groupBy('incoming.bill_reference_number') as $ref => $group) {
            $bill = $bills->get($ref);
            if (!$bill) continue;
            $ledger = $transactions->where('bill_reference_number', $ref)->where('type', 'payment')->sum(fn ($t) => $this->cents($t->amount));
            $added = $group->filter(fn ($r) => !$r['existing_id'] && $r['incoming']['type'] === 'payment')->sum(fn ($r) => $this->cents($r['incoming']['amount']));
            if ($ledger + $added > $this->cents($bill->total_due)) {
                foreach ($rows as &$row) if ($row['kind'] === 'payment' && $row['incoming']['bill_reference_number'] === $ref) $row['issues'][] = 'Payment history would exceed the bill total.';
                unset($row);
            }
        }
        foreach ($billMap as $ref => $i) {
            $bill = $bills->get($ref); $raw = $rows[$i]['incoming'];
            $paid = $this->cents($bill?->amount_paid ?? 0) + ($paymentDeltas[$ref] ?? 0);
            $total = $this->cents($raw['total_due']);
            if (($paymentDeltas[$ref] ?? 0) > 0) $rows[$i]['status'] = 'ready';
            if (!$bill && $raw['status'] === 'Paid' && $paid === 0) {
                $rows[$i]['warnings'][] = 'Device marks this bill paid, but no payment history was exported. Approving accepts its paid balance.';
                $paid = $total;
            }
            if ($paid > $total) $rows[$i]['issues'][] = 'Payments exceed this bill’s total.';
            if ($raw['status'] === 'Void' && $paid > 0) $rows[$i]['issues'][] = 'A void bill cannot receive payments.';
            if (in_array($raw['status'], ['Paid', 'Partially Paid'], true) && (($raw['status'] === 'Paid' && $paid < $total) || ($raw['status'] === 'Partially Paid' && ($paid <= 0 || $paid >= $total)))) $rows[$i]['issues'][] = 'Bill status does not agree with the available payment history.';
            $rows[$i]['amount_paid'] = $paid / 100;
            $rows[$i]['balance_due'] = $raw['status'] === 'Void' ? 0 : max(0, $total - $paid) / 100;
            $rows[$i]['planned_status'] = $raw['status'] === 'Void' ? 'Void' : ($paid >= $total && $total > 0 ? 'Paid' : ($paid > 0 ? 'Partially Paid' : $raw['status']));
        }
        foreach ($paymentDeltas as $ref => $amount) if (!isset($billMap[$ref]) && ($bill = $bills->get($ref))) {
            if ($this->cents($bill->amount_paid) + $amount > $this->cents($bill->total_due) || $bill->status === 'Void') {
                foreach ($rows as &$row) if ($row['kind'] === 'payment' && $row['incoming']['bill_reference_number'] === $ref) $row['issues'][] = 'Payment exceeds the bill balance or the bill is void.';
                unset($row);
            }
        }
        $globalIssues = [];
        if ($state['devices']->contains(fn ($d) => (int) $d->brgy_id !== (int) $payload['device']['brgy_id'])) $globalIssues[] = 'This master is registered to a different barangay. Check the device assignment.';
        if ((int) ($payload['device']['verified_sync_ack'] ?? 0) !== 1) $globalIssues[] = 'Update the master firmware before approving: verified export acknowledgments are required.';
        $summary = ['ready' => 0, 'already_saved' => 0, 'blocked' => 0, 'warnings' => 0, 'readings' => count($payload['readings']), 'bills' => count($payload['bills']), 'payments' => count($payload['transactions'])];
        foreach ($rows as &$row) {
            $row['issues'] = array_values(array_unique($row['issues']));
            if ($row['issues']) $row['status'] = 'blocked';
            $summary[$row['status']]++;
            $summary['warnings'] += count($row['warnings']);
        }
        unset($row);
        return ['rows' => $rows, 'summary' => $summary, 'issues' => $globalIssues, 'can_approve' => !$globalIssues && !$summary['blocked'], 'fingerprint' => hash('sha256', json_encode($state, JSON_THROW_ON_ERROR))];
    }

    public function apply(array $payload, array $plan): void
    {
        $uid = $payload['device']['device_uid']; $readingIds = [];
        foreach ($plan['rows'] as $row) {
            $raw = $row['incoming'];
            if ($row['kind'] !== 'reading') continue;
            if ($row['existing_id']) {
                $readingIds[$raw['reading_id']] = $row['existing_id'];
                if ($row['status'] !== 'already_saved') DB::table('reading')->where('reading_id', $row['existing_id'])->update(['device_reading_id' => $raw['reading_id']]);
            } else {
                $reading = Reading::create(['customer_id' => $row['customer_id'], 'device_uid' => $uid, 'device_reading_id' => $raw['reading_id'], 'customer_account_number' => $row['account'], 'previous_reading' => $raw['previous_reading'], 'current_reading' => $raw['current_reading'], 'usage_m3' => $raw['usage_m3'], 'reading_at' => Carbon::createFromTimestamp($raw['reading_at'], 'Asia/Manila'), 'Synced' => true, 'last_sync' => now()]);
                $readingIds[$raw['reading_id']] = $reading->reading_id;
            }
        }
        $billRefs = [];
        foreach ($plan['rows'] as $row) {
            if ($row['kind'] !== 'bill' || $row['status'] === 'already_saved') continue;
            $raw = $row['incoming'];
            $bill = $row['existing_id'] ? Bill::findOrFail($row['existing_id']) : new Bill();
            if (!$bill->exists) $bill->fill(['reference_number' => $raw['reference_number'], 'customer_id' => $row['customer_id'], 'reading_id' => $readingIds[$raw['reading_id']] ?? Reading::where('device_uid', $uid)->where('device_reading_id', $raw['reading_id'])->firstOrFail()->reading_id, 'device_uid' => $uid, 'customer_account_number' => $row['account'], 'bill_date' => $raw['bill_date'], 'due_date' => $row['due_date'], 'rate_per_m3' => $raw['rate_per_m3'], 'charges' => $raw['charges'], 'penalty' => $raw['penalty'], 'total_due' => $raw['total_due']]);
            $bill->fill(['status' => $row['planned_status'], 'amount_paid' => $row['amount_paid'], 'balance_due' => $row['balance_due']]);
            $bill->Synced = true; $bill->last_sync = now(); $bill->save(); $billRefs[$raw['reference_number']] = true;
        }
        foreach ($plan['rows'] as $row) {
            if ($row['kind'] !== 'payment' || $row['status'] === 'already_saved') continue;
            $raw = $row['incoming'];
            if ($row['existing_id']) DB::table('bill_transaction')->where('bill_transaction_id', $row['existing_id'])->update(['device_transaction_id' => $raw['bill_transaction_id']]);
            else {
                $bill = Bill::where('reference_number', $raw['bill_reference_number'])->firstOrFail();
                $transaction = new BillTransaction();
                $transaction->fill(['bill_reference_number' => $raw['bill_reference_number'], 'bill_id' => $bill->bill_id, 'device_uid' => $uid, 'processed_by_device_uid' => $uid, 'type' => $raw['type'], 'source' => $raw['source'], 'amount' => $raw['amount'], 'cash_received' => $raw['cash_received'] ?? null, 'change' => $raw['change'], 'transaction_date' => $raw['transaction_date'], 'payment_method' => $raw['payment_method'], 'notes' => $raw['notes'] ?? null, 'Synced' => true, 'last_sync' => now()]);
                $transaction->device_transaction_id = $raw['bill_transaction_id']; $transaction->save();
                if ($raw['type'] === 'payment' && empty($row['history_only']) && !isset($billRefs[$raw['bill_reference_number']])) {
                    $bill->amount_paid = ((float) $bill->amount_paid) + (float) $raw['amount'];
                    $bill->balance_due = max(0, (float) $bill->total_due - (float) $bill->amount_paid);
                    $bill->status = $bill->balance_due <= 0.001 ? 'Paid' : 'Partially Paid'; $bill->save();
                }
            }
        }
    }
}
