<?php

namespace App\Services;

use Illuminate\Support\Facades\DB;

class RestorePointService
{
    public const TABLES = [
        'settings', 'customer_type', 'deduction', 'barangay_sequence', 'customer', 'customer_profiles',
        'customer_deduction', 'device', 'reading', 'bill', 'bill_deduction',
        'bill_transaction', 'audit_logs', 'service_incidents', 'incident_events', 'sync_reviews',
    ];

    public function buildPayload(): array
    {
        $payload = ['format' => 'watersystem-backup', 'version' => 2,
            'created_at' => now()->toIso8601String(), 'application' => config('app.name'), 'tables' => []];
        foreach (self::TABLES as $table) {
            if (DB::getSchemaBuilder()->hasTable($table)) {
                $payload['tables'][$table] = DB::table($table)->get()->map(fn ($row) => (array) $row)->all();
            }
        }
        return $payload;
    }

    public function create(string $label, string $source, ?int $userId, AuditLogService $audit): array
    {
        return DB::transaction(function () use ($label, $source, $userId, $audit) {
            $payload = $this->buildPayload();
            $rowCount = collect($payload['tables'])->sum(fn ($rows) => count($rows));
            $id = DB::table('backup_snapshots')->insertGetId([
                'label' => $label, 'source' => $source, 'created_by' => $userId,
                'table_count' => count($payload['tables']), 'row_count' => $rowCount,
                'payload' => json_encode($payload, JSON_THROW_ON_ERROR | JSON_UNESCAPED_UNICODE),
                'created_at' => now(), 'updated_at' => now(),
            ]);
            $audit->record('restore_point_created', 'BackupSnapshot', null, ['id' => $id], [
                'auditable_id' => $id, 'source' => $source, 'row_count' => $rowCount,
            ]);
            return ['id' => $id, 'label' => $label, 'row_count' => $rowCount];
        });
    }
}
