<?php
declare(strict_types=1);
require __DIR__.'/transfer-mysql.php';
$profile = $argv[1] ?? '';
$transfer = $argv[2] ?? '';
$apply = ($argv[3] ?? '') === '--apply';
$snapshot = json_decode(file_get_contents($transfer.'/mysql-original.json'), true, 512, JSON_THROW_ON_ERROR);
$sql = file_get_contents($transfer.'/mysql-original.sql');
$pdo = database($profile.'/dmbc.sqlite');
$adjustments = [];
if ($apply) $pdo->exec('BEGIN IMMEDIATE');
try {
    foreach (DATA_TABLES as $table) {
        $data = $snapshot['tables'][$table];
        $next = $data['next_auto_increment'] ?? null;
        if ($next === null && preg_match('/CREATE TABLE `'.preg_quote($table, '/').'` \(.*?\) ENGINE=[^;]+;/s', $sql, $create)) {
            if (preg_match('/AUTO_INCREMENT=(\d+)/', $create[0], $match)) $next = (int) $match[1];
        }
        if ($next === null) continue;
        $statement = $pdo->prepare('SELECT seq FROM sqlite_sequence WHERE name=?'); $statement->execute([$table]);
        $current = $statement->fetchColumn();
        if ($current !== false && (int) $current + 1 < $next) {
            $adjustments[$table] = ['current_next_id' => (int) $current + 1, 'mysql_next_id' => $next];
            if ($apply) { $statement = $pdo->prepare('UPDATE sqlite_sequence SET seq=? WHERE name=?'); $statement->execute([$next - 1, $table]); }
        }
    }
    if ($apply) $pdo->exec('COMMIT');
} catch (Throwable $error) { if ($apply) $pdo->exec('ROLLBACK'); throw $error; }
echo json_encode(['sequence_adjustments_required' => $adjustments, 'applied' => $apply], JSON_PRETTY_PRINT | JSON_THROW_ON_ERROR)."\n";
