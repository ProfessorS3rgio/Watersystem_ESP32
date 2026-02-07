<?php

use Illuminate\Support\Facades\Route;
use App\Http\Controllers\AuthController;
use App\Http\Controllers\CustomerController;
use App\Http\Controllers\ReadingController;
use App\Http\Controllers\BillController;
use App\Http\Controllers\DeviceController;
use App\Http\Controllers\SettingsController;
use App\Http\Controllers\DashboardController;
use App\Http\Controllers\CustomerTypeController;
use App\Http\Controllers\BarangaySequenceController;
use App\Http\Controllers\DeductionController;

Route::get('/', function () {
    return view('welcome');
});

// Lightweight endpoint to refresh CSRF/XSRF cookies for SPA requests.
Route::get('/csrf-token', function () {
    return response()->json(['csrf_token' => csrf_token()]);
});

// Auth routes
Route::post('/login', [AuthController::class, 'login']);
Route::post('/logout', [AuthController::class, 'logout'])->middleware('auth');
Route::get('/user', [AuthController::class, 'user'])->middleware('auth');

// Customers (JSON)
Route::middleware('auth')->group(function () {
    Route::get('/dashboard', [DashboardController::class, 'index']);

    Route::get('/customer-types', [CustomerTypeController::class, 'index']);
    Route::get('/barangays', [BarangaySequenceController::class, 'index']);
    Route::get('/deductions', [DeductionController::class, 'index']);

    Route::get('/customers', [CustomerController::class, 'index']);
    Route::post('/customers', [CustomerController::class, 'store']);
    Route::put('/customers/{customer}', [CustomerController::class, 'update']);
    Route::delete('/customers/{customer}', [CustomerController::class, 'destroy']);
    Route::post('/customers/sync', [CustomerController::class, 'sync']);
    Route::post('/customers/mark-synced', [CustomerController::class, 'markSynced']);

    Route::get('/customers/{customer}/readings', [ReadingController::class, 'indexByCustomer']);

    Route::get('/customers/{customer}/bills', [BillController::class, 'indexByCustomer']);
    Route::get('/customers/{customer}/payments', [BillController::class, 'paymentsByCustomer']);
    Route::get('/bills', [BillController::class, 'index']);
    Route::post('/bills/{bill}/pay', [BillController::class, 'pay']);
    Route::get('/bills/{bill}/payment', [BillController::class, 'getPayment']);
    Route::get('/bills/today-payments', [BillController::class, 'todayPayments']);
    Route::get('/bills/monthly-payments', [BillController::class, 'monthlyPayments']);
    Route::get('/bills/stats', [BillController::class, 'stats']);
    Route::get('/bills/transactions', [BillController::class, 'transactions']);

    Route::post('/readings/sync', [ReadingController::class, 'sync']);
    Route::post('/bills/sync', [BillController::class, 'sync']);
    Route::post('/devices/sync', [DeviceController::class, 'sync']);

    Route::post('/bills/{bill}/mark-paid', [BillController::class, 'markPaid']);
    Route::post('/bills/{bill}/void', [BillController::class, 'void']);

    Route::get('/settings', [SettingsController::class, 'show']);
    Route::put('/settings', [SettingsController::class, 'update']);
});
