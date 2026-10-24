<?php

namespace App\Http\Controllers;

use App\Models\AuditLog;
use App\Services\AuditLogService;
use Carbon\CarbonImmutable;
use Illuminate\Http\Request;

class AuditLogController extends Controller
{
    public function index(Request $request)
    {
        $filters = $request->validate([
            'page' => ['nullable', 'integer', 'min:1'],
            'per_page' => ['nullable', 'integer', 'in:15,30,60'],
            'snapshot_id' => ['nullable', 'integer', 'min:0'],
            'q' => ['nullable', 'string', 'max:120'],
            'action' => ['nullable', 'string', 'max:100'],
            'auditable_type' => ['nullable', 'string', 'max:255'],
            'actor_type' => ['nullable', 'string', 'in:user,system,device'],
            'date_from' => ['nullable', 'date_format:Y-m-d'],
            'date_to' => ['nullable', 'date_format:Y-m-d', ...($request->filled('date_from') ? ['after_or_equal:date_from'] : [])],
            'sync_run_id' => ['nullable', 'string', 'max:128'],
            'request_id' => ['nullable', 'uuid'],
        ]);
        // Freeze the upper ID until explicitly refreshed so inserts cannot shift pages.
        $latestId = (int) (AuditLog::max('id') ?? 0);
        $snapshot = min((int) ($filters['snapshot_id'] ?? $latestId), $latestId);
        $query = AuditLog::query()->with('actor:id,name')->where('id', '<=', $snapshot);
        foreach (['action', 'auditable_type', 'actor_type'] as $field) {
            if (!empty($filters[$field])) $query->where($field, $filters[$field]);
        }
        foreach (['sync_run_id', 'request_id'] as $field) {
            if (!empty($filters[$field])) $query->where('metadata->'.$field, $filters[$field]);
        }
        $timezone = config('app.timezone', 'UTC');
        if (!empty($filters['date_from'])) {
            $start = CarbonImmutable::createFromFormat('!Y-m-d', $filters['date_from'], 'Asia/Manila')->setTimezone($timezone);
            $query->where('created_at', '>=', $start);
        }
        if (!empty($filters['date_to'])) {
            $end = CarbonImmutable::createFromFormat('!Y-m-d', $filters['date_to'], 'Asia/Manila')->addDay()->setTimezone($timezone);
            $query->where('created_at', '<', $end);
        }
        if (($term = trim($filters['q'] ?? '')) !== '') {
            $query->where(function ($search) use ($term) {
                $like = '%'.$term.'%';
                $search->where('action', 'like', $like)->orWhere('device_uid', 'like', $like)
                    ->orWhere('auditable_type', 'like', $like)->orWhere('metadata', 'like', $like)
                    ->orWhere('before', 'like', $like)->orWhere('after', 'like', $like)
                    ->orWhereHas('actor', fn ($actor) => $actor->where('name', 'like', $like));
                if (ctype_digit($term)) $search->orWhere('id', (int) $term)->orWhere('auditable_id', (int) $term);
            });
        }
        $page = $query->orderByDesc('id')->paginate((int) ($filters['per_page'] ?? 30));
        $page->through(fn (AuditLog $log) => $this->present($log, false));
        return response()->json([
            ...$page->toArray(),
            'snapshot_id' => $snapshot,
            'timezone' => 'Asia/Manila',
            'filter_options' => [
                'actions' => AuditLog::where('id', '<=', $snapshot)->distinct()->orderBy('action')->pluck('action'),
                'types' => AuditLog::where('id', '<=', $snapshot)->distinct()->orderBy('auditable_type')->pluck('auditable_type'),
            ],
        ])->header('Cache-Control', 'no-store');
    }

    public function show(AuditLog $auditLog)
    {
        $auditLog->load('actor:id,name');
        return response()->json(['data' => $this->present($auditLog, true)])->header('Cache-Control', 'no-store');
    }

    private function present(AuditLog $log, bool $detail): array
    {
        $metadata = AuditLogService::redact($log->metadata) ?? [];
        $after = AuditLogService::redact($log->after);
        $before = AuditLogService::redact($log->before);
        $data = [
            'id' => $log->id,
            'created_at' => $log->created_at?->toISOString(),
            'action' => $log->action,
            'auditable_type' => $log->auditable_type,
            'auditable_id' => $log->auditable_id,
            'actor_type' => $log->actor_type,
            'actor_id' => $log->actor_id,
            'actor_name' => $log->actor_type === 'user' ? ($metadata['actor_name'] ?? $log->actor?->name ?? 'User #'.($log->actor_id ?? 'unknown')) : ucfirst($log->actor_type),
            'device_uid' => $log->device_uid,
            'subject_label' => $after['account_no'] ?? $before['account_no'] ?? $after['reference_number'] ?? $before['reference_number'] ?? null,
            'subject_name' => $after['customer_name'] ?? $before['customer_name'] ?? null,
            'imported_row_count' => is_array($metadata['imported_rows'] ?? null) ? count($metadata['imported_rows']) : 0,
            'metadata' => $detail ? $metadata : array_filter($metadata, fn ($value) => !is_array($value) && !is_object($value)),
        ];
        if ($detail) $data += ['before' => $before, 'after' => $after, 'ip_address' => $log->ip_address];
        return $data;
    }
}
