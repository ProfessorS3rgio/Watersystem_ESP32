<?php

use Illuminate\Foundation\Inspiring;
use Illuminate\Support\Facades\Artisan;
use Illuminate\Support\Facades\Schedule;
use App\Services\BillingStatusService;

Artisan::command('inspire', function () {
    $this->comment(Inspiring::quote());
})->purpose('Display an inspiring quote');

Schedule::call(fn () => app(BillingStatusService::class)->refreshDueBills())
    ->dailyAt('00:05')
    ->timezone('Asia/Manila')
    ->name('refresh-due-bills')
    ->withoutOverlapping();
