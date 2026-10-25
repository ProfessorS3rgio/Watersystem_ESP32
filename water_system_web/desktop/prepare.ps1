$ErrorActionPreference = 'Stop'
$project = Split-Path $PSScriptRoot -Parent
$resources = Join-Path $project 'src-tauri/resources'
$backend = Join-Path $resources 'backend'
$runtime = Join-Path $resources 'php'
New-Item -ItemType Directory -Force -Path $backend, $runtime | Out-Null

# Copy an allowlist; never package .env, live databases, logs, hot files or uploads.
foreach ($folder in @('app', 'config', 'database/migrations', 'database/seeders', 'routes', 'resources/views', 'vendor')) {
    $destination = Join-Path $backend $folder
    New-Item -ItemType Directory -Force -Path $destination | Out-Null
    & robocopy (Join-Path $project $folder) $destination /MIR /NFL /NDL /NJH /NJS /NP | Out-Null
    if ($LASTEXITCODE -ge 8) { throw "Unable to stage $folder" }
}
New-Item -ItemType Directory -Force -Path "$backend/desktop" | Out-Null
Copy-Item "$PSScriptRoot/initialize.php", "$PSScriptRoot/router.php", "$PSScriptRoot/check-runtime.php" "$backend/desktop"
# Reject accidental inclusion of development profiles from a previous staging run.
if (Test-Path "$backend/desktop/.test-data") {
    $staleProfile = (Resolve-Path "$backend/desktop/.test-data").Path
    $expectedRoot = (Resolve-Path $resources).Path + [IO.Path]::DirectorySeparatorChar
    if (!$staleProfile.StartsWith($expectedRoot, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unexpected staged profile path' }
    Remove-Item -LiteralPath $staleProfile -Recurse -Force
}
New-Item -ItemType Directory -Force -Path "$backend/bootstrap/cache", "$backend/public" | Out-Null
Copy-Item "$project/bootstrap/app.php", "$project/bootstrap/providers.php" "$backend/bootstrap"
Copy-Item "$project/artisan", "$project/composer.json" $backend
Copy-Item "$project/public/index.php" "$backend/public"
Push-Location $project
try { & npm.cmd run build; if ($LASTEXITCODE -ne 0) { throw 'Frontend build failed' } } finally { Pop-Location }
foreach ($folder in @('build', 'images')) {
    & robocopy "$project/public/$folder" "$backend/public/$folder" /MIR /NFL /NDL /NJH /NJS /NP | Out-Null
    if ($LASTEXITCODE -ge 8) { throw "Unable to stage public/$folder" }
}

$phpSource = if ($env:DMBC_PHP_SOURCE) { $env:DMBC_PHP_SOURCE } else { Split-Path (Get-Command php.exe).Source -Parent }
Copy-Item "$phpSource/php.exe" $runtime
Get-ChildItem -LiteralPath $phpSource -Filter '*.dll' | Copy-Item -Destination $runtime
New-Item -ItemType Directory -Force -Path "$runtime/ext" | Out-Null
$extensions = @('curl', 'fileinfo', 'gd', 'mbstring', 'openssl', 'pdo_sqlite', 'zip')
foreach ($extension in $extensions) { Copy-Item "$phpSource/ext/php_$extension.dll" "$runtime/ext" }
@('extension_dir="ext"', 'date.timezone=UTC', 'memory_limit=256M', 'upload_max_filesize=32M', 'post_max_size=32M', 'log_errors=On', 'display_errors=Off') + ($extensions | ForEach-Object { "extension=$_" }) | Set-Content "$runtime/php.ini"
foreach ($license in @('license.txt', 'LICENSE', 'readme-redist-bins.txt')) {
    if (Test-Path "$phpSource/$license") { Copy-Item "$phpSource/$license" $runtime }
}
& "$runtime/php.exe" -c "$runtime/php.ini" "$PSScriptRoot/check-runtime.php"
if ($LASTEXITCODE -ne 0) { throw 'Bundled PHP runtime validation failed' }
Write-Host 'DMBC desktop resources ready. No live data was copied.'
exit 0
