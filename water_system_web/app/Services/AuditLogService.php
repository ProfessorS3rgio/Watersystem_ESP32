<?php

namespace App\Services;

use App\Models\AuditLog;
use Illuminate\Database\Eloquent\Model;

class AuditLogService
{
    public function record(string $action, Model|string $subject, ?array $before = null, ?array $after = null, array $metadata = []): AuditLog
    {
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
