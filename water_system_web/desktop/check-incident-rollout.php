<?php
declare(strict_types=1);
require __DIR__.'/transfer-mysql.php';
$mode = $argv[1] ?? '';
$profile = $argv[2] ?? '';
$manifest = $argv[3] ?? '';
if (!in_array($mode, ['capture', 'verify'], true) || !$profile || !$manifest) throw new RuntimeException('Usage: capture|verify <desktop-profile> <baseline-json>');
$tables = ['users', 'customer', 'reading', 'bill', 'bill_transaction', 'settings', 'customer_type', 'barangay_sequence', 'deduction', 'device', 'customer_deduction', 'bill_deduction', 'audit_logs', 'backup_snapshots', 'sync_runs'];
$fingerprint = function (PDO $pdo) use ($tables): array {
    $result = [];
    foreach ($tables as $table) {
        $rows = $pdo->query('SELECT * FROM "'.$table.'"')->fetchAll(PDO::FETCH_ASSOC);
        $hashes = array_map(fn ($row) => hash('sha256', json_encode($row, JSON_THROW_ON_ERROR | JSON_UNESCAPED_UNICODE)), $rows);
        sort($hashes); $result[$table] = ['rows' => count($rows), 'sha256' => hash('sha256', implode('', $hashes))];
    }
    return $result;
};
$current = ['desktop' => $fingerprint(database($profile.'/dmbc.sqlite')), 'key' => hash_file('sha256', $profile.'/application.key'), 'login' => hash_file('sha256', $profile.'/initial-login.txt')];
if ($mode === 'capture') {
    if (is_file($manifest)) throw new RuntimeException('Baseline already exists; do not overwrite it.');
    if (!is_dir(dirname($manifest))) mkdir(dirname($manifest), 0700, true);
    $pdo = database($profile.'/dmbc.sqlite');
    $pdo->exec('VACUUM INTO '.$pdo->quote(dirname($manifest).'/desktop-before-incidents.sqlite'));
    saveJson($manifest, $current);
    echo "Pre-upgrade desktop backup and record fingerprints saved.\n";
} else {
    $before = json_decode(file_get_contents($manifest), true, 512, JSON_THROW_ON_ERROR);
    foreach ($before['desktop'] as $table => $expected) if ($current['desktop'][$table] !== $expected) throw new RuntimeException('Existing desktop records changed during rollout: '.$table);
    if ($current['key'] !== $before['key'] || $current['login'] !== $before['login']) throw new RuntimeException('Existing desktop login or key changed.');
    echo json_encode(['existing_data_unchanged' => true, 'login_preserved' => true, 'counts' => array_map(fn ($row) => $row['rows'], $current['desktop'])], JSON_PRETTY_PRINT | JSON_THROW_ON_ERROR)."\n";
}
