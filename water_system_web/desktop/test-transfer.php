<?php
declare(strict_types=1);
require __DIR__.'/transfer-mysql.php';
$realProfile = $argv[1] ?? '';
$realTransfer = $argv[2] ?? '';
$validation = $realTransfer.'/validation-'.bin2hex(random_bytes(4));
$assertions = 0;
function check(bool $condition, string $message): void {
    global $assertions;
    if (!$condition) throw new RuntimeException($message);
    $assertions++;
}
function fixture(string $label): array {
    global $validation, $realProfile, $realTransfer;
    $profile = $validation.'/'.$label;
    $transfer = $profile.'/transfers/test';
    mkdir($transfer.'/staged-profile', 0700, true);
    mkdir($profile.'/storage/framework/sessions', 0700, true);
    file_put_contents($profile.'/storage/framework/sessions/old-session', 'test');
    $live = database($realProfile.'/dmbc.sqlite');
    $live->exec('VACUUM INTO '.$live->quote($profile.'/dmbc.sqlite'));
    foreach (['application.key', 'initial-login.txt'] as $file) copy($realProfile.'/'.$file, $profile.'/'.$file);
    copy($realTransfer.'/staged-profile/dmbc.sqlite', $transfer.'/staged-profile/dmbc.sqlite');
    copy($realTransfer.'/manifest.json', $transfer.'/manifest.json');
    return [$profile, $transfer];
}

[$profile, $transfer] = fixture('success');
ob_start(); try { finalize($profile, $transfer); } finally { ob_end_clean(); }
$pdo = database($profile.'/dmbc.sqlite');
check((int) $pdo->query('SELECT COUNT(*) FROM customer')->fetchColumn() === 276, 'Customer rows were lost.');
check((int) $pdo->query('SELECT COUNT(*) FROM bill')->fetchColumn() === 470, 'Bill rows were lost.');
check((int) $pdo->query('SELECT COUNT(*) FROM bill_transaction')->fetchColumn() === 335, 'Payment rows were lost.');
check((int) $pdo->query('SELECT COUNT(*) FROM users')->fetchColumn() === 2, 'Both login accounts must be preserved.');
preg_match('/Password: ([^\r\n]+)/', file_get_contents($profile.'/initial-login.txt'), $password);
$statement = $pdo->query("SELECT id,password FROM users WHERE email='admin@dmbc.local'");
$admin = $statement->fetch();
check(password_verify($password[1], $admin['password']), 'Generated desktop login was changed.');
check((int) $admin['id'] !== 1, 'Desktop user must not replace the MySQL user identity.');
check((int) $pdo->query("SELECT actor_id FROM audit_logs WHERE json_extract(metadata, '$.preserved_from_desktop') = 1 LIMIT 1")->fetchColumn() === (int) $admin['id'], 'Existing desktop audit actor was not remapped.');
check($pdo->query("SELECT COUNT(*) FROM audit_logs WHERE action='mysql_data_imported'")->fetchColumn() === 1, 'Import audit event missing.');
check(!is_file($profile.'/storage/framework/sessions/old-session'), 'Old login sessions must be invalidated.');
check(is_file($transfer.'/desktop-old-sessions/old-session'), 'Prior sessions were not backed up.');
check(hash_file('sha256', $profile.'/application.key') === hash_file('sha256', $realProfile.'/application.key'), 'Encryption key was changed.');
validateDatabase($pdo);
$pdo = null;

[$profile, $transfer] = fixture('bad-foreign-key');
$candidate = $transfer.'/staged-profile/dmbc.sqlite';
$pdo = database($candidate);
$pdo->exec("UPDATE reading SET device_uid='MISSING-DEVICE' WHERE reading_id=(SELECT reading_id FROM reading LIMIT 1)");
$pdo = null;
$manifest = json_decode(file_get_contents($transfer.'/manifest.json'), true, 512, JSON_THROW_ON_ERROR);
$manifest['candidate_sha256'] = hash_file('sha256', $candidate); saveJson($transfer.'/manifest.json', $manifest);
$before = hash_file('sha256', $profile.'/dmbc.sqlite');
$stopped = false;
try { finalize($profile, $transfer); } catch (RuntimeException $error) { $stopped = str_contains($error->getMessage(), 'Foreign-key validation failed'); }
check($stopped, 'Invalid foreign keys must stop the transfer.');
check(hash_file('sha256', $profile.'/dmbc.sqlite') === $before, 'Failed validation changed the desktop database.');
check(is_file($profile.'/storage/framework/sessions/old-session'), 'Failed validation changed desktop sessions.');

[$profile, $transfer] = fixture('existing-records');
$pdo = database($profile.'/dmbc.sqlite');
$pdo->exec("INSERT INTO customer (account_no,type_id,customer_name,brgy_id,address,previous_reading) VALUES ('DESKTOP-ONLY',1,'Do not discard',1,'Test',0)");
$pdo = null;
$before = hash_file('sha256', $profile.'/dmbc.sqlite');
$stopped = false;
try { finalize($profile, $transfer); } catch (RuntimeException $error) { $stopped = str_contains($error->getMessage(), 'explicit merge is required'); }
check($stopped, 'New desktop customer records must stop replacement.');
check(hash_file('sha256', $profile.'/dmbc.sqlite') === $before, 'Desktop-only records were altered.');
echo "Transfer dry-run checks passed: {$assertions} assertions, account and audit identity preservation, session invalidation, integrity validation, and refusal to discard desktop-only records.\n";
