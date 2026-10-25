<?php

// Runs only against the desktop profile supplied by the native launcher.
declare(strict_types=1);

$data = getenv('DMBC_DESKTOP_DATA');
if (!$data || getenv('DB_CONNECTION') !== 'sqlite' || getenv('DB_DATABASE') !== $data.'/dmbc.sqlite') {
    throw new RuntimeException('A separate desktop SQLite profile is required.');
}
foreach (['storage/app/private', 'storage/app/public', 'storage/framework/cache/data', 'storage/framework/sessions', 'storage/framework/views', 'storage/logs', 'cache'] as $directory) {
    if (!is_dir($data.'/'.$directory) && !mkdir($data.'/'.$directory, 0700, true)) {
        throw new RuntimeException('Cannot create desktop data directory.');
    }
}
$keyFile = $data.'/application.key';
if (!is_file($keyFile)) {
    file_put_contents($keyFile, 'base64:'.base64_encode(random_bytes(32)), LOCK_EX);
}
putenv('APP_KEY='.trim(file_get_contents($keyFile)));
if (!is_file($data.'/dmbc.sqlite')) touch($data.'/dmbc.sqlite');

require dirname(__DIR__).'/vendor/autoload.php';
$app = require dirname(__DIR__).'/bootstrap/app.php';
$kernel = $app->make(Illuminate\Contracts\Console\Kernel::class);
$kernel->bootstrap();

$migrator = $app->make('migrator');
if ($migrator->repositoryExists()) {
    $pending = array_diff(array_keys($migrator->getMigrationFiles(database_path('migrations'))), $migrator->getRepository()->getRan());
    if ($pending !== []) {
        $upgradeBackup = $data.'/before-upgrade-'.gmdate('Ymd-His').'-'.bin2hex(random_bytes(3)).'.sqlite';
        Illuminate\Support\Facades\DB::statement('VACUUM INTO '.Illuminate\Support\Facades\DB::connection()->getPdo()->quote($upgradeBackup));
    }
}

// SQLite table checks must accept partial payments on a fresh installation.
if ($kernel->call('migrate', ['--force' => true]) !== 0) {
    throw new RuntimeException($kernel->output());
}
Illuminate\Support\Facades\DB::transaction(function () use ($data, $kernel) {
    foreach ([
        'settings' => Database\Seeders\SettingsSeeder::class,
        'customer_type' => Database\Seeders\CustomerTypeSeeder::class,
        'deduction' => Database\Seeders\DeductionSeeder::class,
        'barangay_sequence' => Database\Seeders\BarangaySequenceSeeder::class,
    ] as $table => $seeder) {
        if (!Illuminate\Support\Facades\DB::table($table)->exists()) {
            if ($kernel->call('db:seed', ['--class' => $seeder, '--force' => true]) !== 0) {
                throw new RuntimeException($kernel->output());
            }
        }
    }
    if (!App\Models\User::query()->exists()) {
        $password = bin2hex(random_bytes(8));
        // Preserve this file on subsequent launches; never reset an existing account.
        if (file_put_contents($data.'/initial-login.txt', "DMBC development desktop\r\nEmail: admin@dmbc.local\r\nPassword: {$password}\r\n", LOCK_EX) === false) {
            throw new RuntimeException('Unable to save initial login.');
        }
        App\Models\User::create(['name' => 'DMBC Administrator', 'email' => 'admin@dmbc.local', 'password' => $password]);
    }
});
echo "DMBC SQLite profile ready.\n";
