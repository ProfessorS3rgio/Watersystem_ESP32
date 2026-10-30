<?php

namespace App\Services;

use App\Models\{Bill, Customer, Reading, Setting};
use Carbon\Carbon;

class HistoricalCollectionReading
{
    /** Returns a proposal only when adjacent monthly readings establish the boundaries. */
    public function propose(Customer $customer, Carbon $month, mixed $usage, mixed $minimum, bool $lock = false): array
    {
        $fail = fn ($message) => ['error' => $message];
        $readings = Reading::where('customer_id', $customer->customer_id)->orderBy('reading_at')->orderBy('reading_id')
            ->when($lock, fn ($query) => $query->lockForUpdate())->get();
        $latest = $readings->last();
        if (! $latest || $month->format('Y-m') >= $latest->reading_at->format('Y-m')) {
            return $fail('Automatic creation is limited to months before this account’s latest reading month. Import readings first.');
        }
        if ($readings->contains(fn ($r) => $r->reading_at->format('Y-m') === $month->format('Y-m'))) {
            return $fail('A reading already exists for this month. Review its missing bill before importing a payment.');
        }
        if (Bill::where('customer_id', $customer->customer_id)->where('bill_date', '>=', $month->toDateString())
            ->where('bill_date', '<', $month->copy()->addMonth()->toDateString())->when($lock, fn ($q) => $q->lockForUpdate())->exists()) {
            return $fail('A bill already exists for this month. Upload the workbook again.');
        }
        if ($usage === null || trim((string) $usage) === '' || ! is_numeric($usage) || $usage < 0
            || floor((float) $usage) != $usage || $usage > 2147483647 || ((float) $minimum > 0 && (float) $usage === 0)) {
            return $fail('Actual whole-number consumption in Cubic Used is required; a minimum-charge amount cannot establish meter readings.');
        }
        $usage = (int) $usage;
        $before = $readings->filter(fn ($r) => $r->reading_at->format('Y-m') === $month->copy()->subMonth()->format('Y-m'))->values();
        $after = $readings->filter(fn ($r) => $r->reading_at->format('Y-m') === $month->copy()->addMonth()->format('Y-m'))->values();
        if ($before->count() > 1 || $after->count() > 1 || ($before->isEmpty() && $after->isEmpty())) {
            return $fail('A unique reading in the immediately preceding or following month is required to reconstruct meter values.');
        }
        $prior = $before->first(); $next = $after->first();
        $previous = $prior ? $prior->current_reading : $next->previous_reading - $usage;
        $present = $previous + $usage;
        if ($previous < 0 || $present > 2147483647 || ($next && $present !== $next->previous_reading)
            || ($prior && $next && $prior->device_uid !== $next->device_uid)) {
            return $fail('Consumption conflicts with adjacent readings or a meter/device change. Review this historical period manually.');
        }
        $billing = app(ImportBillingCalculator::class)->calculate($customer, $usage);
        $date = $month->copy()->endOfMonth();
        $days = max(0, (int) (Setting::query()->orderBy('id')->value('bill_due_days') ?? 5));
        return ['previous' => $previous, 'present' => $present, 'usage' => $usage, 'device_uid' => ($prior ?? $next)->device_uid,
            'billing' => $billing, 'bill_date' => $date->toDateString(), 'due_date' => $date->copy()->addDays($days)->toDateString(),
            'reading_fingerprint' => hash('sha256', $readings->toJson())];
    }
}
