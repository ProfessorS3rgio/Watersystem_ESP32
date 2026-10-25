<?php

namespace App\Services;

use App\Models\Customer;
use App\Models\IncidentEvent;
use App\Models\ServiceIncident;
use Illuminate\Database\UniqueConstraintViolationException;
use Illuminate\Support\Facades\DB;
use Illuminate\Validation\ValidationException;

class IncidentService
{
    public function __construct(private AuditLogService $audit) {}

    public function create(array $input): array
    {
        $fields = ['customer_id', 'title', 'description', 'location', 'category', 'priority', 'pipe_section', 'assigned_to'];
        $canonical = [];
        foreach ($fields as $field) $canonical[$field] = $field === 'customer_id' ? (int) $input[$field] : ($input[$field] ?? null);
        $hash = hash('sha256', json_encode([$canonical, auth()->id()], JSON_THROW_ON_ERROR));
        $existing = ServiceIncident::where('client_token', $input['client_token'])->first();
        if ($existing) return [$this->replayed($existing, $hash), false];
        try {
            return DB::transaction(function () use ($input, $hash) {
                $customer = Customer::findOrFail($input['customer_id']);
                $incident = ServiceIncident::create([
                    ...$input, 'request_hash' => $hash, 'reference_number' => 'PENDING-'.$input['client_token'],
                    'account_no' => $customer->account_no, 'customer_name' => $customer->customer_name,
                    'customer_address' => $customer->address,
                    'barangay' => DB::table('barangay_sequence')->where('brgy_id', $customer->brgy_id)->value('barangay'),
                    'reported_by' => auth()->id(), 'reported_by_name' => auth()->user()->name,
                    'status' => 'reported', 'version' => 1,
                ]);
                $incident->update(['reference_number' => 'INC-'.now('Asia/Manila')->format('Ymd').'-'.str_pad((string) $incident->id, 6, '0', STR_PAD_LEFT)]);
                $this->event($incident, 'reported', $input['description'], null, 'reported');
                $this->audit->record('incident_reported', $incident, null, $this->snapshot($incident), $this->context($incident));
                return [$incident, true];
            });
        } catch (UniqueConstraintViolationException $error) {
            $existing = ServiceIncident::where('client_token', $input['client_token'])->first();
            if (!$existing) throw $error;
            return [$this->replayed($existing, $hash), false];
        }
    }

    private function replayed(ServiceIncident $incident, string $hash): ServiceIncident
    {
        abort_unless(hash_equals($incident->request_hash, $hash), 409, 'This submission key was already used for a different report. Reload the form before submitting another incident.');
        return $incident;
    }

    public function update(ServiceIncident $incident, array $input): ServiceIncident
    {
        return DB::transaction(function () use ($incident, $input) {
            $incident = ServiceIncident::query()->lockForUpdate()->findOrFail($incident->id);
            abort_unless($incident->version === (int) $input['version'], 409, 'This incident changed in another window. Reload it before saving.');
            $before = $this->snapshot($incident);
            $from = $incident->status;
            $to = $input['status'] ?? $from;
            $message = $input['note'] ?? null;
            $reopening = in_array($from, ['resolved', 'closed', 'cancelled'], true) && in_array($to, ServiceIncident::OPEN, true);
            if ($to !== $from) {
                if (!in_array($to, ServiceIncident::TRANSITIONS[$from], true)) throw ValidationException::withMessages(['status' => ['That status change is not allowed.']]);
                if (($reopening || $to === 'cancelled' || ($from === 'in_progress' && $to === 'reported')) && !$message) throw ValidationException::withMessages(['note' => ['Add a reason for this status change.']]);
                if ($to === 'resolved' && mb_strlen(trim($input['resolution'] ?? '')) < 5) throw ValidationException::withMessages(['resolution' => ['Describe the repair and how the issue was checked before marking it resolved.']]);
            }
            $editable = ['title', 'description', 'location', 'category', 'priority', 'pipe_section', 'assigned_to'];
            if (in_array($from, ['resolved', 'closed', 'cancelled'], true) && !$reopening) {
                foreach ($editable as $field) {
                    if (array_key_exists($field, $input) && $input[$field] !== $incident->$field) throw ValidationException::withMessages([$field => ['Reopen the incident before changing its report details.']]);
                }
            }
            $changes = array_intersect_key($input, array_flip($editable));
            if ($to !== $from) {
                $changes['status'] = $to;
                if ($to === 'resolved') { $changes['resolution'] = $input['resolution']; $changes['resolved_at'] = now(); $changes['closed_at'] = null; }
                if (in_array($to, ['closed', 'cancelled'], true)) $changes['closed_at'] = now();
                if ($reopening) { $changes['resolution'] = null; $changes['resolved_at'] = null; $changes['closed_at'] = null; $changes['reopened_count'] = $incident->reopened_count + 1; }
            } elseif (array_key_exists('resolution', $input) && $input['resolution'] !== $incident->resolution) {
                throw ValidationException::withMessages(['resolution' => ['Record a resolution when changing the status to Resolved. Existing repair notes remain in the history.']]);
            }
            $incident->fill($changes);
            if (!$incident->isDirty() && !$message) return $incident;
            $dirty = $incident->getDirty();
            $dirty['version'] = $incident->version + 1; $dirty['updated_at'] = now();
            $saved = ServiceIncident::whereKey($incident->id)->where('version', $input['version'])->update($dirty);
            abort_unless($saved === 1, 409, 'This incident changed in another window. Reload it before saving.');
            $incident->refresh();
            $after = $this->snapshot($incident);
            $diff = [];
            foreach ([...$editable, 'status', 'resolution', 'resolved_at', 'closed_at'] as $field) {
                if (($before[$field] ?? null) !== ($after[$field] ?? null)) $diff[$field] = ['before' => $before[$field] ?? null, 'after' => $after[$field] ?? null];
            }
            $kind = $reopening ? 'reopened' : ($to !== $from ? 'status_changed' : ($diff ? 'updated' : 'note_added'));
            $this->event($incident, $kind, $message ?? ($to === 'resolved' ? $input['resolution'] : null), $from, $to, $diff);
            $this->audit->record('incident_'.$kind, $incident, $before, $after, [...$this->context($incident), 'note' => $message]);
            return $incident;
        });
    }

    private function snapshot(ServiceIncident $incident): array { return $incident->makeHidden(['client_token', 'request_hash'])->attributesToArray(); }
    private function context(ServiceIncident $incident): array { return ['reference_number' => $incident->reference_number, 'account_no' => $incident->account_no]; }
    private function event(ServiceIncident $incident, string $kind, ?string $message, ?string $from, string $to, ?array $changes = null): void
    {
        IncidentEvent::create(['incident_id' => $incident->id, 'actor_id' => auth()->id(), 'actor_name' => auth()->user()->name,
            'kind' => $kind, 'message' => $message, 'from_status' => $from, 'to_status' => $to, 'changes' => $changes, 'created_at' => now()]);
    }
}
