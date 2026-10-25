<?php

// Read-only diagnostic. Never prints password material or resets an account.
declare(strict_types=1);
$profile = $argv[1] ?? '';
if (!$profile || !is_file($profile.'/initial-login.txt') || !is_file($profile.'/dmbc.sqlite')) {
    fwrite(STDERR, "A desktop profile containing SQLite and initial-login.txt is required.\n");
    exit(1);
}
$login = file_get_contents($profile.'/initial-login.txt');
preg_match('/Email: ([^\r\n]+)/', $login, $email);
preg_match('/Password: ([^\r\n]+)/', $login, $password);
$database = new PDO('sqlite:'.$profile.'/dmbc.sqlite');
$statement = $database->prepare('SELECT password FROM users WHERE email = ?');
$statement->execute([$email[1] ?? '']);
$hash = $statement->fetchColumn();
echo json_encode(['account_exists' => $hash !== false, 'generated_password_matches_database' => $hash !== false && password_verify($password[1] ?? '', $hash)], JSON_THROW_ON_ERROR)."\n";
