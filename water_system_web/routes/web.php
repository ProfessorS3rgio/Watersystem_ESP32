<?php

use Illuminate\Support\Facades\Route;
use App\Http\Controllers\AuthController;
use App\Http\Controllers\CustomerController;
use App\Http\Controllers\ReadingController;
use App\Http\Controllers\BillController;
use App\Http\Controllers\SettingsController;
use App\Http\Controllers\DashboardController;

Route::get('/', function () {
    return view('welcome');
});

// Auth routes
Route::post('/login', [AuthController::class, 'login']);
Route::post('/logout', [AuthController::class, 'logout'])->middleware('auth');
Route::get('/user', [AuthController::class, 'user'])->middleware('auth');

// Customers (JSON)
Route::middleware('auth')->group(function () {
    Route::get('/dashboard', [DashboardController::class, 'index']);

    Route::get('/customers', [CustomerController::class, 'index']);
    Route::post('/customers', [CustomerController::class, 'store']);
    Route::put('/customers/{customer}', [CustomerController::class, 'update']);
    Route::delete('/customers/{customer}', [CustomerController::class, 'destroy']);
    Route::post('/customers/sync', [CustomerController::class, 'sync']);

    Route::get('/customers/{customer}/readings', [ReadingController::class, 'indexByCustomer']);

    Route::post('/readings/sync', [ReadingController::class, 'sync']);

    Route::post('/bills/{bill}/mark-paid', [BillController::class, 'markPaid']);

    Route::get('/settings', [SettingsController::class, 'show']);
    Route::put('/settings', [SettingsController::class, 'update']);
});
