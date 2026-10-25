<?php
declare(strict_types=1);
require __DIR__.'/transfer-mysql.php';
$profile = $argv[1] ?? '';
$transfer = $argv[2] ?? '';
$completed = json_decode(file_get_contents($transfer.'/completed.json'), true, 512, JSON_THROW_ON_ERROR);
$snapshot = json_decode(file_get_contents($transfer.'/mysql-original.json'), true, 512, JSON_THROW_ON_ERROR);
$pdo = database($profile.'/dmbc.sqlite');
validateDatabase($pdo);
$counts = [];
foreach (DATA_TABLES as $table) {
    $expected = $snapshot['tables'][$table];
    if (in_array($table, ['users', 'audit_logs'], true)) {
        $ids = array_column($expected['rows'], 'id');
        if ($ids) {
            $statement = $pdo->prepare('SELECT * FROM "'.$table.'" WHERE id IN ('.implode(',', array_fill(0, count($ids), '?')).')');
            $statement->execute($ids); $rows = $statement->fetchAll();
        } else $rows = [];
    } else $rows = $pdo->query('SELECT * FROM "'.$table.'"')->fetchAll();
    if (tableDigest($rows, $expected['columns']) !== $expected['sha256']) throw new RuntimeException('Imported row values differ from the MySQL backup: '.$table);
    $counts[$table] = (int) $pdo->query('SELECT COUNT(*) FROM "'.$table.'"')->fetchColumn();
}
foreach ($completed['financial_totals'] as $field => $expected) {
    [$table, $column] = explode('.', $field);
    $actual = number_format((float) $pdo->query('SELECT SUM("'.$column.'") FROM "'.$table.'"')->fetchColumn(), 2, '.', '');
    if ($actual !== $expected) throw new RuntimeException('Financial total mismatch: '.$field);
}
foreach (['application.key', 'initial-login.txt'] as $file) {
    if (hash_file('sha256', $profile.'/'.$file) !== hash_file('sha256', $transfer.'/desktop-before-'.$file)) throw new RuntimeException('Desktop '.$file.' was changed.');
}
echo json_encode(['verified_installed_database' => true, 'all_source_rows_match_backup' => true, 'financial_totals_match' => true, 'foreign_keys_valid' => true, 'desktop_key_and_login_preserved' => true, 'counts' => $counts], JSON_PRETTY_PRINT | JSON_THROW_ON_ERROR)."\n";
