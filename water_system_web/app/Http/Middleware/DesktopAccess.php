<?php

namespace App\Http\Middleware;

use Closure;
use Illuminate\Http\Request;

class DesktopAccess
{
    public function handle(Request $request, Closure $next)
    {
        $url = parse_url(getenv('APP_URL'));
        $expectedHost = '127.0.0.1:'.$url['port'];
        abort_unless($request->getHttpHost() === $expectedHost, 403);
        $token = getenv('DMBC_DESKTOP_TOKEN');
        abort_unless(is_string($token) && strlen($token) >= 64, 503);

        if ($request->isMethod('GET') && $request->path() === 'desktop-launch/'.$token) {
            return redirect('/')->withCookie(cookie('dmbc_desktop_access', $token, 0, '/', null, false, true, false, 'Strict'));
        }

        abort_unless(hash_equals($token, (string) $request->cookies->get('dmbc_desktop_access')), 403);
        $origin = $request->headers->get('Origin');
        abort_if($origin !== null && $origin !== getenv('APP_URL'), 403);
        abort_if($request->headers->get('Sec-Fetch-Site') === 'cross-site', 403);

        return $next($request);
    }
}
