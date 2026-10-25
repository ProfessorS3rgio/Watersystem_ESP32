<?php
declare(strict_types=1);

// Developer-run, one-time transfer. MySQL is only read; all staging is outside
// the repository. The installed application must be stopped before finalize.
require dirname(__DIR__).'/vendor/autoload.php';
$app = require dirname(__DIR__).'/bootstrap/app.php';
$kernel = $app->make(Illuminate\Contracts\Console\Kernel::class);
$kernel->bootstrap();

use Illuminate\Support\Facades\DB;

const DATA_TABLES = ['users', 'barangay_sequence', 'deduction', 'customer_type', 'device', 'customer', 'reading', 'bill', 'customer_deduction', 'bill_deduction', 'bill_transaction', 'settings', 'sync_runs', 'backup_snapshots', 'audit_logs', 'service_incidents', 'incident_events', 'sync_reviews'];

function database(string $path): PDO
{
    $pdo = new PDO('sqlite:'.$path, null, null, [PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION, PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC]);
    $pdo->exec('PRAGMA busy_timeout=5000');
    return $pdo;
}

function saveJson(string $path, array $value): void
{
    if (file_put_contents($path, json_encode($value, JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE | JSON_THROW_ON_ERROR), LOCK_EX) === false) throw new RuntimeException('Could not write transfer record.');
}

function insertRow(PDO $pdo, string $table, array $row): void
{
    $columns = array_map(fn ($column) => '"'.str_replace('"', '""', $column).'"', array_keys($row));
    $statement = $pdo->prepare('INSERT INTO "'.$table.'" ('.implode(',', $columns).') VALUES ('.implode(',', array_fill(0, count($row), '?')).')');
    $statement->execute(array_values($row));
}

function normalized(mixed $value, string $type): mixed
{
    if ($value === null) return null;
    if (preg_match('/^(?:decimal|numeric)\(\d+,(\d+)\)/i', $type, $match)) return number_format((float) $value, (int) $match[1], '.', '');
    if (preg_match('/^(?:tinyint|smallint|mediumint|bigint|int|integer|bit)/i', $type)) return (string) (int) $value;
    if (preg_match('/^(?:float|double|real)/i', $type)) return sprintf('%.12g', (float) $value);
    return (string) $value;
}

function tableDigest(array $rows, array $columns): string
{
    $digests = [];
    foreach ($rows as $row) {
        $canonical = [];
        foreach ($columns as $column) $canonical[$column['Field']] = normalized($row[$column['Field']], $column['Type']);
        $digests[] = hash('sha256', json_encode($canonical, JSON_UNESCAPED_UNICODE | JSON_THROW_ON_ERROR));
    }
    sort($digests);
    return hash('sha256', implode('', $digests));
}

function validateDatabase(PDO $pdo): void
{
    if ($pdo->query('PRAGMA integrity_check')->fetchColumn() !== 'ok') throw new RuntimeException('SQLite integrity check failed.');
    $violations = $pdo->query('PRAGMA foreign_key_check')->fetchAll();
    if ($violations !== []) throw new RuntimeException('Foreign-key validation failed: '.json_encode($violations, JSON_THROW_ON_ERROR));
}

function stage(string $profile, bool $backupOnly = false): void
{
    $source = DB::connection('mysql');
    $sourcePdo = $source->getPdo();
    $transfer = $profile.'/transfers/mysql-'.gmdate('Ymd-His').'-'.bin2hex(random_bytes(3));
    $candidateProfile = $transfer.'/staged-profile';
    if (!mkdir($candidateProfile, 0700, true)) throw new RuntimeException('Cannot create staging directory.');
    $snapshot = ['format' => 'dmbc-mysql-transfer', 'version' => 1, 'created_at' => gmdate('c'), 'tables' => []];
    $source->statement('SET TRANSACTION ISOLATION LEVEL REPEATABLE READ');
    $source->statement('SET TRANSACTION READ ONLY');
    $source->beginTransaction();
    try {
        $sqlFile = fopen($transfer.'/mysql-original.sql', 'xb');
        if ($sqlFile === false) throw new RuntimeException('Cannot create MySQL backup.');
        fwrite($sqlFile, "-- DMBC consistent MySQL snapshot; source was not modified.\nSET NAMES utf8mb4;\nSET SESSION sql_mode='NO_AUTO_VALUE_ON_ZERO';\nSET FOREIGN_KEY_CHECKS=0;\n");
        try {
            foreach ($source->select('SHOW TABLE STATUS') as $table) {
                if ($table->Engine !== 'InnoDB') throw new RuntimeException('Consistent transfer requires InnoDB tables: '.$table->Name);
                $name = $table->Name;
                if (!preg_match('/^[a-z_]+$/', $name)) throw new RuntimeException('Unexpected table name.');
                $quoted = '`'.$name.'`';
                $columns = array_map(fn ($column) => (array) $column, $source->select('SHOW COLUMNS FROM '.$quoted));
                $rows = array_map(fn ($row) => (array) $row, $source->table($name)->get()->all());
                $create = (array) $source->selectOne('SHOW CREATE TABLE '.$quoted);
                fwrite($sqlFile, "\n".$create['Create Table'].";\n");
                foreach ($rows as $row) {
                    $values = array_map(fn ($value) => $value === null ? 'NULL' : $sourcePdo->quote((string) $value), array_values($row));
                    fwrite($sqlFile, 'INSERT INTO '.$quoted.' (`'.implode('`,`', array_keys($row)).'`) VALUES ('.implode(',', $values).");\n");
                }
                $snapshot['tables'][$name] = ['columns' => $columns, 'rows' => $rows, 'sha256' => tableDigest($rows, $columns), 'next_auto_increment' => $table->Auto_increment === null ? null : (int) $table->Auto_increment];
            }
            fwrite($sqlFile, "SET FOREIGN_KEY_CHECKS=1;\n");
        } finally { fclose($sqlFile); }
        $source->commit();
    } catch (Throwable $error) { $source->rollBack(); throw $error; }
    saveJson($transfer.'/mysql-original.json', $snapshot);
    if ($backupOnly) {
        echo json_encode(['source_backup_directory' => $transfer, 'tables' => count($snapshot['tables'])], JSON_PRETTY_PRINT | JSON_THROW_ON_ERROR)."\n";
        return;
    }

    $candidate = $candidateProfile.'/dmbc.sqlite';
    touch($candidate);
    config(['database.connections.desktop_import' => [
        'driver' => 'sqlite', 'database' => $candidate, 'prefix' => '', 'foreign_key_constraints' => false,
        'busy_timeout' => 5000, 'journal_mode' => 'DELETE', 'synchronous' => 'FULL',
    ]]);
    if ($kernel = app(Illuminate\Contracts\Console\Kernel::class)) {
        if ($kernel->call('migrate', ['--database' => 'desktop_import', '--force' => true]) !== 0) throw new RuntimeException($kernel->output());
    }
    $target = DB::connection('desktop_import')->getPdo();
    $target->exec('PRAGMA foreign_keys=OFF');
    $target->beginTransaction();
    try {
        foreach (DATA_TABLES as $table) {
            $data = $snapshot['tables'][$table] ?? null;
            if ($data === null) throw new RuntimeException('Missing source table: '.$table);
            $targetColumns = array_column($target->query('PRAGMA table_info("'.$table.'")')->fetchAll(PDO::FETCH_ASSOC), 'name');
            foreach ($data['columns'] as $column) if (!in_array($column['Field'], $targetColumns, true)) throw new RuntimeException('Target schema is missing '.$table.'.'.$column['Field']);
            foreach ($data['rows'] as $row) insertRow($target, $table, $row);
            if (($data['next_auto_increment'] ?? null) !== null) {
                // Preserve IDs reserved by MySQL even if its highest rows were deleted.
                $query = $target->prepare('INSERT INTO sqlite_sequence(name,seq) SELECT ?,0 WHERE NOT EXISTS (SELECT 1 FROM sqlite_sequence WHERE name=?)');
                $query->execute([$table, $table]);
                $query = $target->prepare('UPDATE sqlite_sequence SET seq=MAX(seq,?) WHERE name=?');
                $query->execute([$data['next_auto_increment'] - 1, $table]);
            }
        }
        $target->commit();
    } catch (Throwable $error) { $target->rollBack(); throw $error; }
    $target->exec('PRAGMA foreign_keys=ON');
    validateDatabase($target);
    $counts = [];
    foreach (DATA_TABLES as $table) {
        $data = $snapshot['tables'][$table];
        $actual = $target->query('SELECT * FROM "'.$table.'"')->fetchAll(PDO::FETCH_ASSOC);
        if (tableDigest($actual, $data['columns']) !== $data['sha256']) throw new RuntimeException('Copied row values do not match: '.$table);
        $counts[$table] = count($actual);
    }
    $totals = [];
    foreach (['bill' => ['charges', 'penalty', 'total_due', 'amount_paid', 'balance_due'], 'bill_transaction' => ['amount', 'cash_received', 'change']] as $table => $fields) {
        foreach ($fields as $field) {
            $expected = number_format(array_sum(array_column($snapshot['tables'][$table]['rows'], $field)), 2, '.', '');
            $actual = number_format((float) $target->query('SELECT SUM("'.$field.'") FROM "'.$table.'"')->fetchColumn(), 2, '.', '');
            if ($expected !== $actual) throw new RuntimeException('Financial total mismatch: '.$table.'.'.$field);
            $totals[$table.'.'.$field] = $actual;
        }
    }
    $manifest = ['format' => 'dmbc-mysql-transfer', 'stage_verified' => true, 'created_at' => $snapshot['created_at'], 'counts' => $counts, 'financial_totals' => $totals, 'source_json_sha256' => hash_file('sha256', $transfer.'/mysql-original.json'), 'candidate_sha256' => hash_file('sha256', $candidate), 'excluded_operational_tables' => ['migrations', 'sessions', 'password_reset_tokens', 'cache', 'cache_locks', 'jobs', 'job_batches', 'failed_jobs']];
    saveJson($transfer.'/manifest.json', $manifest);
    echo json_encode(['transfer_directory' => $transfer, 'verified' => true, 'counts' => $counts, 'financial_totals' => $totals], JSON_PRETTY_PRINT | JSON_THROW_ON_ERROR)."\n";
}

function assertSourceUnchanged(string $transfer): void
{
    $manifest = json_decode(file_get_contents($transfer.'/manifest.json'), true, 512, JSON_THROW_ON_ERROR);
    if (hash_file('sha256', $transfer.'/mysql-original.json') !== $manifest['source_json_sha256']) throw new RuntimeException('Source backup checksum changed.');
    $snapshot = json_decode(file_get_contents($transfer.'/mysql-original.json'), true, 512, JSON_THROW_ON_ERROR);
    $source = DB::connection('mysql');
    $source->statement('SET TRANSACTION ISOLATION LEVEL REPEATABLE READ');
    $source->statement('SET TRANSACTION READ ONLY');
    $source->beginTransaction();
    try {
        foreach (DATA_TABLES as $table) {
            if (!isset($snapshot['tables'][$table])) throw new RuntimeException('The schema has gained '.$table.' since this snapshot. Create a fresh stage before importing.');
            $columns = array_map(fn ($column) => (array) $column, $source->select('SHOW COLUMNS FROM `'.$table.'`'));
            if ($columns !== $snapshot['tables'][$table]['columns']) throw new RuntimeException('MySQL schema changed since staging: '.$table);
            $rows = array_map(fn ($row) => (array) $row, $source->table($table)->get()->all());
            if (tableDigest($rows, $columns) !== $snapshot['tables'][$table]['sha256']) throw new RuntimeException('MySQL data changed since staging: '.$table.'. Create a fresh stage before importing.');
        }
        $source->commit();
    } catch (Throwable $error) { $source->rollBack(); throw $error; }
}

function finalize(string $profile, string $transfer): void
{
    $manifest = json_decode(file_get_contents($transfer.'/manifest.json'), true, 512, JSON_THROW_ON_ERROR);
    $candidate = $transfer.'/staged-profile/dmbc.sqlite';
    $live = $profile.'/dmbc.sqlite';
    if (!($manifest['stage_verified'] ?? false) || hash_file('sha256', $candidate) !== $manifest['candidate_sha256']) throw new RuntimeException('Staged candidate is not the verified file.');
    if (is_file($transfer.'/completed.json')) throw new RuntimeException('This transfer was already applied.');
    if (!str_starts_with(str_replace('\\', '/', realpath($transfer)), str_replace('\\', '/', realpath($profile)).'/transfers/')) throw new RuntimeException('Transfer must be under the selected desktop profile.');
    $desktop = database($live);
    // This first deployment is empty apart from references, login and audit rows.
    // Stop rather than silently discard records entered in the desktop meanwhile.
    foreach (['customer', 'reading', 'bill', 'bill_transaction', 'device', 'sync_runs', 'backup_snapshots', 'service_incidents', 'incident_events'] as $table) {
        if ((int) $desktop->query('SELECT COUNT(*) FROM "'.$table.'"')->fetchColumn() !== 0) throw new RuntimeException('Desktop already contains '.$table.' records; an explicit merge is required.');
    }
    $desktop->exec('VACUUM INTO '.$desktop->quote($transfer.'/desktop-before-import.sqlite'));
    foreach (['application.key', 'initial-login.txt'] as $file) {
        if (!copy($profile.'/'.$file, $transfer.'/desktop-before-'.$file)) throw new RuntimeException('Cannot back up desktop '.$file);
    }
    $prior = database($transfer.'/desktop-before-import.sqlite');
    $target = database($candidate);
    $target->exec('PRAGMA foreign_keys=ON');
    $target->beginTransaction();
    try {
        $userMap = [];
        foreach ($prior->query('SELECT * FROM users')->fetchAll() as $user) {
            $find = $target->prepare('SELECT id,password FROM users WHERE email=?'); $find->execute([$user['email']]);
            $sameEmail = $find->fetch();
            if ($sameEmail) {
                if ($sameEmail['password'] !== $user['password']) throw new RuntimeException('Desktop and MySQL have different passwords for the same email; resolve account ownership before importing.');
                $userMap[$user['id']] = (int) $sameEmail['id'];
            } else {
                $oldId = $user['id']; unset($user['id']); insertRow($target, 'users', $user); $userMap[$oldId] = (int) $target->lastInsertId();
            }
        }
        $preserved = 0;
        foreach ($prior->query('SELECT * FROM audit_logs ORDER BY id')->fetchAll() as $event) {
            $originalId = $event['id']; unset($event['id']);
            if ($event['actor_type'] === 'user' && $event['actor_id'] !== null) $event['actor_id'] = $userMap[$event['actor_id']] ?? null;
            $metadata = json_decode($event['metadata'] ?: '{}', true, 512, JSON_THROW_ON_ERROR);
            $metadata['preserved_from_desktop'] = true; $metadata['original_desktop_event_id'] = $originalId;
            $event['metadata'] = json_encode($metadata, JSON_THROW_ON_ERROR);
            insertRow($target, 'audit_logs', $event); $preserved++;
        }
        insertRow($target, 'audit_logs', ['actor_type' => 'system', 'actor_id' => null, 'action' => 'mysql_data_imported', 'auditable_type' => 'DatabaseTransfer', 'auditable_id' => null, 'before' => null, 'after' => null, 'metadata' => json_encode(['source' => 'mysql', 'counts' => $manifest['counts'], 'source_sha256' => $manifest['source_json_sha256'], 'preserved_desktop_events' => $preserved, 'desktop_login_preserved' => true], JSON_THROW_ON_ERROR), 'ip_address' => null, 'device_uid' => null, 'created_at' => gmdate('Y-m-d H:i:s'), 'updated_at' => gmdate('Y-m-d H:i:s')]);
        $target->commit();
    } catch (Throwable $error) { $target->rollBack(); throw $error; }
    validateDatabase($target);
    $target->exec('PRAGMA journal_mode=DELETE');
    $target = null; $desktop = null; $prior = null;
    $replacement = $profile.'/dmbc-import-'.bin2hex(random_bytes(6)).'.sqlite';
    if (!copy($candidate, $replacement)) throw new RuntimeException('Cannot prepare desktop replacement.');
    $sessions = $profile.'/storage/framework/sessions';
    $oldLive = $transfer.'/desktop-replaced.sqlite';
    if (is_file($live.'-wal') || is_file($live.'-shm')) throw new RuntimeException('Desktop SQLite is still open. Stop the application before replacing it.');
    if (!rename($live, $oldLive)) throw new RuntimeException('Cannot move existing database; the application may still be running.');
    try {
        if (!rename($replacement, $live)) throw new RuntimeException('Cannot activate imported database.');
        if (is_dir($sessions)) {
            if (!rename($sessions, $transfer.'/desktop-old-sessions')) throw new RuntimeException('Cannot invalidate old login sessions.');
        }
        if (!mkdir($sessions, 0700, true)) throw new RuntimeException('Cannot create clean login sessions.');
    } catch (Throwable $error) {
        if (is_file($live)) rename($live, $transfer.'/failed-replacement.sqlite');
        rename($oldLive, $live);
        if (!is_dir($sessions) && is_dir($transfer.'/desktop-old-sessions')) rename($transfer.'/desktop-old-sessions', $sessions);
        throw $error;
    }
    $manifest['completed_at'] = gmdate('c'); $manifest['preserved_desktop_events'] = $preserved;
    saveJson($transfer.'/completed.json', $manifest);
    echo json_encode(['imported' => true, 'backup_directory' => $transfer, 'counts' => $manifest['counts'], 'desktop_login_preserved' => true, 'preserved_desktop_events' => $preserved], JSON_PRETTY_PRINT | JSON_THROW_ON_ERROR)."\n";
}

if (realpath($_SERVER['SCRIPT_FILENAME'] ?? '') === __FILE__) try {
    $operation = $argv[1] ?? '';
    $profile = $argv[2] ?? '';
    if (!is_file($profile.'/dmbc.sqlite') || !is_file($profile.'/application.key')) throw new RuntimeException('An existing installed DMBC desktop profile is required.');
    if ($operation === 'stage') stage($profile);
    elseif ($operation === 'backup-source') stage($profile, true);
    elseif ($operation === 'finalize' && !empty($argv[3])) { assertSourceUnchanged($argv[3]); finalize($profile, $argv[3]); }
    else throw new RuntimeException('Usage: php desktop/transfer-mysql.php stage <profile> OR finalize <profile> <transfer-directory>');
} catch (Throwable $error) {
    fwrite(STDERR, 'Transfer stopped: '.$error->getMessage()."\n");
    exit(1);
}
