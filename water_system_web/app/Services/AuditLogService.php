<?php

namespace App\Services;

use App\Models\AuditLog;
use Illuminate\Database\Eloquent\Model;

class AuditLogService
{
    public function syncOperation(string $operation, int $received, array $result = [], ?string $deviceUid = null): AuditLog
    {
        return $this->record('sync_' . $operation, 'SyncRun', null, $result, [
            'received' => $received,
            'device_uid' => $deviceUid,
            'sync_run_id' => request()->header('X-Sync-Run-ID'),
        ]);
    }

    public function record(string $action, Model|string $subject, ?array $before = null, ?array $after = null, array $metadata = []): AuditLog
    {
        $syncRunId = request()->header('X-Sync-Run-ID');
        if ($syncRunId && empty($metadata['sync_run_id'])) {
            $metadata['sync_run_id'] = $syncRunId;
        }
        $request = request();
        $user = auth()->user();
        $attributes = is_object($subject) ? $subject->getAttributes() : [];

        return AuditLog::create([
            'actor_type' => $user ? 'user' : ($metadata['actor_type'] ?? 'system'),
            'actor_id' => $user?->getAuthIdentifier(),
            'action' => $action,
            'auditable_type' => is_object($subject) ? $subject::class : $subject,
            'auditable_id' => is_object($subject) ? $subject->getKey() : ($metadata['auditable_id'] ?? null),
            'before' => $before,
            'after' => $after ?? ($attributes ?: null),
            'metadata' => $metadata,
            'ip_address' => $request?->ip(),
            'device_uid' => $metadata['device_uid'] ?? null,
        ]);
    }
}
