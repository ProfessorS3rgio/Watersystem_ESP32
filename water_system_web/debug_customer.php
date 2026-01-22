<?php

require_once __DIR__ . '/vendor/autoload.php';

$app = require_once __DIR__ . '/bootstrap/app.php';
$app->make(Illuminate\Contracts\Console\Kernel::class)->bootstrap();

use App\Models\Customer;

$customers = Customer::where('brgy_id', 2)->orderBy('customer_id')->get();

if ($customers->count() > 0) {
    $firstCustomer = $customers[0];
    echo "First customer:\n";
    echo json_encode($firstCustomer->toArray(), JSON_PRETTY_PRINT) . "\n";
    echo "\nCustomer ID: " . $firstCustomer->customer_id . "\n";
} else {
    echo "No customers.\n";
}