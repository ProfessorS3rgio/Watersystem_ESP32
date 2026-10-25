<?php

use Illuminate\Foundation\Application;
use Illuminate\Foundation\Configuration\Exceptions;
use Illuminate\Foundation\Configuration\Middleware;

$application = Application::configure(basePath: dirname(__DIR__))
    ->withRouting(
        web: __DIR__.'/../routes/web.php',
        commands: __DIR__.'/../routes/console.php',
        health: '/up',
    )
    ->withMiddleware(function (Middleware $middleware): void {
        if (getenv('DMBC_DESKTOP_DATA')) {
            $middleware->prepend(\App\Http\Middleware\DesktopAccess::class);
        }
        $middleware->web(remove: [
            \Illuminate\Foundation\Http\Middleware\VerifyCsrfToken::class,
        ]);
    })
    ->withExceptions(function (Exceptions $exceptions): void {
        //
    })->create();

if ($desktopData = getenv('DMBC_DESKTOP_DATA')) {
    $application->useStoragePath($desktopData.'/storage');
    // Laravel's default absolute-cache prefixes omit Windows drive letters.
    $application->addAbsoluteCachePathPrefix(substr($desktopData, 0, 3));
}

return $application;
