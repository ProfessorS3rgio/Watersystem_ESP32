<?php
declare(strict_types=1);
require dirname(__DIR__).'/vendor/autoload.php';
$app = require dirname(__DIR__).'/bootstrap/app.php';
$app->make(Illuminate\Contracts\Console\Kernel::class)->bootstrap();
$database = Illuminate\Support\Facades\DB::connection('mysql');
$tables = $database->select('SHOW TABLE STATUS');
$report = [];
foreach ($tables as $table) {
    $report[] = ['table' => $table->Name, 'engine' => $table->Engine, 'rows' => $database->table($table->Name)->count()];
}
echo json_encode(['tables' => $report, 'users' => $database->table('users')->select('id', 'email')->get(), 'duplicate_device_uids' => $database->table('device')->select('device_uid')->groupBy('device_uid')->havingRaw('COUNT(*) > 1')->count()], JSON_PRETTY_PRINT | JSON_THROW_ON_ERROR)."\n";
