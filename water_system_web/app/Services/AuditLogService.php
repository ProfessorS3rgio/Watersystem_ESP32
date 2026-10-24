<?php

namespace App\Services;

use App\Models\AuditLog;
use Illuminate\Database\Eloquent\Model;
use Illuminate\Support\Str;

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
        $requestId = $request->attributes->get('_audit_request_id');
        if (!$requestId) {
            $requestId = (string) Str::uuid();
            $request->attributes->set('_audit_request_id', $requestId);
        }
        $metadata['request_id'] = $requestId;
        if ($user) $metadata['actor_name'] = $user->name;

        return AuditLog::create([
            'actor_type' => $user ? 'user' : ($metadata['actor_type'] ?? 'system'),
            'actor_id' => $user?->getAuthIdentifier(),
            'action' => $action,
            'auditable_type' => is_object($subject) ? $subject::class : $subject,
            'auditable_id' => is_object($subject) ? $subject->getKey() : ($metadata['auditable_id'] ?? null),
            'before' => self::redact($before),
            'after' => self::redact($after ?? ($before === null ? ($attributes ?: null) : null)),
            'metadata' => self::redact($metadata),
            'ip_address' => $request?->ip(),
            'device_uid' => $metadata['device_uid'] ?? null,
        ]);
    }

    /** Also applied when reading legacy rows that predate capture-time redaction. */
    public static function redact(?array $data, int $depth = 0): ?array
    {
        if ($data === null) return null;
        if ($depth > 12) return ['value' => '[Nested data omitted]'];
        $sensitive = ['password', 'passwordconfirmation', 'remembertoken', 'token', 'accesstoken', 'refreshtoken', 'pairingcode', 'wifipairingcode', 'authorization', 'cookie', 'csrftoken', 'apikey', 'secret', 'clientsecret', 'privatekey'];
        foreach ($data as $key => $value) {
            $normalized = strtolower(preg_replace('/[^a-z0-9]/i', '', (string) $key));
            if (in_array($normalized, $sensitive, true) || preg_match('/password|secret|privatekey|(?:token|apikey|authorization|cookie)$/', $normalized)) $data[$key] = '[REDACTED]';
            elseif (is_array($value)) $data[$key] = self::redact($value, $depth + 1);
        }
        return $data;
    }
}
