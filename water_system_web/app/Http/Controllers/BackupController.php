<?php

namespace App\Http\Controllers;

use App\Services\AuditLogService;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\DB;
use Symfony\Component\Process\Process;
use ZipArchive;

class BackupController extends Controller
{
    private const SNAPSHOT_TABLES = [
        'settings', 'customer_type', 'deduction', 'barangay_sequence', 'customer', 'customer_profiles',
        'customer_deduction', 'device', 'reading', 'bill', 'bill_deduction',
        'bill_transaction', 'audit_logs', 'service_incidents', 'incident_events', 'sync_reviews',
    ];
    private const PRESERVED_TABLES = ['audit_logs', 'service_incidents', 'incident_events', 'sync_reviews'];

    public function createRestorePoint(Request $request, AuditLogService $audit)
    {
        $payload = $this->buildJsonPayload();
        $rowCount = collect($payload['tables'])->sum(fn ($rows) => count($rows));
        $snapshot = DB::table('backup_snapshots')->insertGetId([
            'label' => $request->input('label', 'Before device sync'),
            'source' => $request->input('source', 'sync'),
            'created_by' => $request->user()?->id,
            'table_count' => count($payload['tables']),
            'row_count' => $rowCount,
            'payload' => json_encode($payload, JSON_THROW_ON_ERROR | JSON_UNESCAPED_UNICODE),
            'created_at' => now(),
            'updated_at' => now(),
        ]);

        $audit->record('restore_point_created', 'BackupSnapshot', null, ['id' => $snapshot], [
            'auditable_id' => $snapshot,
            'source' => $request->input('source', 'sync'),
            'row_count' => $rowCount,
        ]);

        return response()->json([
            'id' => $snapshot,
            'label' => $request->input('label', 'Before device sync'),
            'row_count' => $rowCount,
        ], 201);
    }

    public function restorePoints()
    {
        return response()->json([
            'data' => DB::table('backup_snapshots')
                ->select(['id', 'label', 'source', 'table_count', 'row_count', 'created_at'])
                ->latest()
                ->limit(50)
                ->get(),
        ]);
    }

    public function restore(Request $request, int $snapshot, AuditLogService $audit)
    {
        $record = DB::table('backup_snapshots')->where('id', $snapshot)->first();
        if (!$record) {
            return response()->json(['message' => 'Restore point not found.'], 404);
        }

        $payload = json_decode($record->payload, true, 512, JSON_THROW_ON_ERROR);
        if (($payload['format'] ?? null) !== 'watersystem-backup' || !is_array($payload['tables'] ?? null)) {
            return response()->json(['message' => 'Restore point is invalid.'], 422);
        }

        DB::transaction(function () use ($payload, $audit, $snapshot, $record) {
            // A snapshot from before profiles existed must not erase newer
            // personal records. Retain only profiles of the same account/ID.
            $legacyProfiles = !array_key_exists('customer_profiles', $payload['tables']) && DB::getSchemaBuilder()->hasTable('customer_profiles')
                ? DB::table('customer_profiles as p')->join('customer as c', 'c.customer_id', '=', 'p.customer_id')->get(['p.*', 'c.account_no as original_account_no']) : collect();
            // Preserve operational history and only reattach an account with the same identity.
            $links = DB::getSchemaBuilder()->hasTable('service_incidents')
                ? DB::table('service_incidents as i')->leftJoin('customer as c', 'c.customer_id', '=', 'i.customer_id')
                    ->whereNotNull('i.customer_id')->get(['i.id', 'i.customer_id', 'i.status', 'c.account_no as current_account'])
                : collect();
            $this->setForeignKeyChecks(false);
            try {
                foreach (array_reverse(self::SNAPSHOT_TABLES) as $table) {
                    if (in_array($table, self::PRESERVED_TABLES, true)) continue;
                    if (DB::getSchemaBuilder()->hasTable($table)) DB::table($table)->delete();
                }
                foreach (self::SNAPSHOT_TABLES as $table) {
                    if (in_array($table, self::PRESERVED_TABLES, true)) continue;
                    if (!DB::getSchemaBuilder()->hasTable($table)) continue;
                    foreach (array_chunk($payload['tables'][$table] ?? [], 500) as $chunk) {
                        if ($chunk) DB::table($table)->insert($chunk);
                    }
                }
                $restoredAccounts = $links->isEmpty() ? collect() : DB::table('customer')->whereIn('customer_id', $links->pluck('customer_id'))->pluck('account_no', 'customer_id');
                foreach ($legacyProfiles as $legacy) {
                    $sameAccount = DB::table('customer')->where('customer_id', $legacy->customer_id)->where('account_no', $legacy->original_account_no)->exists();
                    if ($sameAccount) {
                        $row = (array) $legacy; unset($row['original_account_no']);
                        DB::table('customer_profiles')->insert($row);
                        // Keep the restored display name in agreement with the retained name parts.
                        $profile = new \App\Models\CustomerProfile($row);
                        if ($profile->hasStructuredName()) DB::table('customer')->where('customer_id', $legacy->customer_id)->update(['customer_name' => $profile->fullName(), 'Synced' => false]);
                    }
                }
                foreach ($links as $link) {
                    $retained = $link->current_account !== null && ($restoredAccounts[$link->customer_id] ?? null) === $link->current_account;
                    DB::table('service_incidents')->where('id', $link->id)->update(['customer_id' => $retained ? $link->customer_id : null]);
                    if (!$retained) {
                        DB::table('service_incidents')->where('id', $link->id)->increment('version');
                        \App\Models\IncidentEvent::create(['incident_id' => $link->id, 'actor_id' => auth()->id(), 'actor_name' => auth()->user()?->name ?? 'System',
                            'kind' => 'account_unlinked', 'from_status' => $link->status, 'to_status' => $link->status,
                            'message' => 'The linked account was not present with the same identity in restored data. Original customer details and incident history were retained.',
                            'changes' => ['customer_id' => ['before' => $link->customer_id, 'after' => null]], 'created_at' => now()]);
                    }
                }
            } finally { $this->setForeignKeyChecks(true); }
            $audit->record('restore_point_restored', 'BackupSnapshot', null, ['id' => $snapshot], [
                'auditable_id' => $snapshot, 'row_count' => $record->row_count,
                'audit_history_preserved' => true, 'incident_history_preserved' => true,
            ]);
        });

        return response()->json(['message' => 'Restore point restored successfully.']);
    }

    public function export(Request $request, AuditLogService $audit)
    {
        $driver = DB::connection()->getDriverName();
        if (!in_array($driver, ['mysql', 'sqlite'], true)) {
            return response()->json(['message' => 'This database does not support archive exports.'], 422);
        }

        $audit->record('data_backup_created', 'Backup', null, null, ['source' => 'web']);
        $payload = $this->buildJsonPayload();
        $sql = $driver === 'mysql' ? $this->createSqlDump() : null;
        $filename = 'watersystem-backup-' . now()->format('Ymd-His') . '.zip';
        $zipPath = tempnam(storage_path('app'), 'watersystem-backup-');

        $zip = new ZipArchive();
        if ($zip->open($zipPath, ZipArchive::CREATE | ZipArchive::OVERWRITE) !== true) {
            return response()->json(['message' => 'Unable to create backup archive.'], 500);
        }

        $zip->addFromString('watersystem-data.json', json_encode($payload, JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE));
        if ($driver === 'sqlite') {
            // VACUUM INTO creates a consistent standalone copy, including WAL data.
            $sqlitePath = storage_path('app/backup-'.bin2hex(random_bytes(12)).'.sqlite');
            try {
                DB::statement('VACUUM INTO '.DB::connection()->getPdo()->quote($sqlitePath));
                $zip->addFromString('watersystem-database.sqlite', file_get_contents($sqlitePath));
            } finally {
                if (is_file($sqlitePath)) unlink($sqlitePath);
            }
        } else {
            $zip->addFromString('watersystem-database.sql', $sql);
        }
        $zip->addFromString('backup-manifest.json', json_encode([
            'format' => 'watersystem-backup',
            'version' => 2,
            'created_at' => now()->toIso8601String(),
            'database' => config('database.default'),
            'json_tables' => array_keys($payload['tables']),
        ], JSON_PRETTY_PRINT));
        $zip->close();

        return response()->download($zipPath, $filename, ['Content-Type' => 'application/zip'])->deleteFileAfterSend(true);
    }

    private function buildJsonPayload(): array
    {
        $payload = [
            'format' => 'watersystem-backup',
            'version' => 2,
            'created_at' => now()->toIso8601String(),
            'application' => config('app.name'),
            'tables' => [],
        ];

        foreach (self::SNAPSHOT_TABLES as $table) {
            if (DB::getSchemaBuilder()->hasTable($table)) {
                $payload['tables'][$table] = DB::table($table)->get()->map(fn ($row) => (array) $row)->all();
            }
        }

        return $payload;
    }

    private function setForeignKeyChecks(bool $enabled): void
    {
        if (DB::connection()->getDriverName() === 'mysql') {
            DB::statement('SET FOREIGN_KEY_CHECKS=' . ($enabled ? '1' : '0'));
        } elseif (DB::connection()->getDriverName() === 'sqlite') {
            // foreign_keys cannot be toggled inside the restore transaction.
            // Defer checks until commit; SQLite resets this flag automatically.
            if (!$enabled) DB::statement('PRAGMA defer_foreign_keys = ON');
        }
    }

    private function createSqlDump(): string
    {
        $connection = config('database.connections.mysql');
        $password = (string) ($connection['password'] ?? '');
        $dumpBinary = $this->findDumpBinary();
        if (!$dumpBinary) {
            throw new \RuntimeException('MySQL dump failed: mysqldump was not found. Set MYSQLDUMP_PATH in .env or add MySQL\'s bin folder to PATH.');
        }

        $process = new Process([
            $dumpBinary,
            '--host=' . ($connection['host'] ?? '127.0.0.1'),
            '--port=' . ($connection['port'] ?? 3306),
            '--protocol=TCP',
            '--user=' . ($connection['username'] ?? 'root'),
            '--single-transaction',
            '--routines',
            '--triggers',
            $connection['database'],
        ], base_path(), array_merge($_ENV, $_SERVER, ['MYSQL_PWD' => $password]));
        $process->setTimeout(120);
        $process->run();

        if ($process->isSuccessful()) return $process->getOutput();

        // Laravel's PDO connection may work even when the native client cannot
        // create a socket. Fall back to a portable schema-and-data dump.
        return $this->createPortableSqlDump();
    }

    private function createPortableSqlDump(): string
    {
        $tables = ['settings', 'customer_type', 'deduction', 'barangay', 'customer', 'customer_deduction', 'reading', 'bill', 'bill_deduction', 'bill_transaction', 'device', 'audit_logs'];
        $pdo = DB::connection()->getPdo();
        $lines = ['-- WaterSystem portable MySQL backup', '-- Generated: ' . now()->toIso8601String(), 'SET FOREIGN_KEY_CHECKS=0;', ''];

        foreach ($tables as $table) {
            if (!DB::getSchemaBuilder()->hasTable($table)) continue;
            $quotedTable = '`' . str_replace('`', '``', $table) . '`';
            $create = DB::select("SHOW CREATE TABLE {$quotedTable}");
            if ($create) {
                $definition = array_values((array) $create[0])[1] ?? null;
                if ($definition) $lines[] = "DROP TABLE IF EXISTS {$quotedTable};\n{$definition};\n";
            }

            $columns = DB::select("SHOW COLUMNS FROM {$quotedTable}");
            $names = array_map(fn ($column) => '`' . str_replace('`', '``', $column->Field) . '`', $columns);
            if (!$names) continue;

            DB::table($table)->orderByRaw('1')->chunk(200, function ($rows) use (&$lines, $pdo, $quotedTable, $names) {
                foreach ($rows as $row) {
                    $values = [];
                    foreach (array_keys((array) $row) as $key) {
                        $value = $row->{$key};
                        $values[] = $value === null ? 'NULL' : $pdo->quote((string) $value);
                    }
                    $lines[] = "INSERT INTO {$quotedTable} (" . implode(', ', $names) . ') VALUES (' . implode(', ', $values) . ');';
                }
            });
            $lines[] = '';
        }

        $lines[] = 'SET FOREIGN_KEY_CHECKS=1;';
        return implode("\n", $lines);
    }

    private function findDumpBinary(): ?string
    {
        $configured = env('MYSQLDUMP_PATH');
        $candidates = array_filter([
            $configured,
            'C:\\Program Files\\MySQL\\MySQL Server 8.0\\bin\\mysqldump.exe',
            'C:\\Program Files\\MySQL\\MySQL Server 8.4\\bin\\mysqldump.exe',
            'C:\\Program Files\\MariaDB 11.0\\bin\\mysqldump.exe',
            'C:\\xampp\\mysql\\bin\\mysqldump.exe',
            'C:\\wamp64\\bin\\mysql\\mysql8.0.\\bin\\mysqldump.exe',
            'mysqldump',
            'mysqldump.exe',
        ]);

        foreach ($candidates as $candidate) {
            if ($candidate === 'mysqldump' || $candidate === 'mysqldump.exe') {
                $command = PHP_OS_FAMILY === 'Windows' ? 'where.exe ' . $candidate : 'command -v ' . $candidate;
                $found = trim((string) shell_exec($command . ' 2>NUL'));
                if ($found !== '') return strtok($found, "\r\n");
                continue;
            }
            if (is_file($candidate)) return $candidate;
        }

        return null;
    }
}
