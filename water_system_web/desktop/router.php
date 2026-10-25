<?php

declare(strict_types=1);
$root = realpath(dirname(__DIR__).'/public');
$path = rawurldecode(parse_url($_SERVER['REQUEST_URI'], PHP_URL_PATH) ?: '/');
$file = realpath($root.$path);
// Only the compiled public files are available to the local webview.
if ($file && str_starts_with($file, $root.DIRECTORY_SEPARATOR) && is_file($file) && strtolower(pathinfo($file, PATHINFO_EXTENSION)) !== 'php') {
    return false;
}
putenv('APP_KEY='.trim(file_get_contents(getenv('DMBC_DESKTOP_DATA').'/application.key')));
require $root.'/index.php';
