<?php

namespace App\Http\Controllers;

use App\Services\AuditLogService;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\DB;
use Symfony\Component\Process\Process;
use ZipArchive;

class BackupController extends Controller
{
    public function export(Request $request, AuditLogService $audit)
    {
        if (config('database.default') !== 'mysql') {
            return response()->json(['message' => 'SQL backups currently require a MySQL database.'], 422);
        }

        $audit->record('data_backup_created', 'Backup', null, null, ['source' => 'web']);
        $payload = $this->buildJsonPayload();
        $sql = $this->createSqlDump();
        $filename = 'watersystem-backup-' . now()->format('Ymd-His') . '.zip';
        $zipPath = tempnam(storage_path('app'), 'watersystem-backup-');

        $zip = new ZipArchive();
        if ($zip->open($zipPath, ZipArchive::CREATE | ZipArchive::OVERWRITE) !== true) {
            return response()->json(['message' => 'Unable to create backup archive.'], 500);
        }

        $zip->addFromString('watersystem-data.json', json_encode($payload, JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE));
        $zip->addFromString('watersystem-database.sql', $sql);
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
        $tables = [
            'settings', 'customer_type', 'deduction', 'barangay', 'customer',
            'customer_deduction', 'reading', 'bill', 'bill_deduction',
            'bill_transaction', 'device', 'audit_logs',
        ];

        $payload = [
            'format' => 'watersystem-backup',
            'version' => 1,
            'created_at' => now()->toIso8601String(),
            'application' => config('app.name'),
            'tables' => [],
        ];

        foreach ($tables as $table) {
            if (DB::getSchemaBuilder()->hasTable($table)) {
                $payload['tables'][$table] = DB::table($table)->get()->map(fn ($row) => (array) $row)->all();
            }
        }

        return $payload;
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
