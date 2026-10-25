<?php
foreach (['pdo_sqlite', 'mbstring', 'fileinfo', 'curl', 'openssl', 'zip', 'gd'] as $extension) {
    if (!extension_loaded($extension)) {
        fwrite(STDERR, "Missing extension: {$extension}\n");
        exit(1);
    }
}
echo "Bundled PHP extensions verified.\n";
