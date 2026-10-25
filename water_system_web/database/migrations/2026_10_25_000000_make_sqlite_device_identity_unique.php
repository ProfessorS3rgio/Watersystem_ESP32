<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\DB;
use Illuminate\Support\Facades\Schema;

return new class extends Migration {
    public function up(): void
    {
        // SQLite requires referenced parent keys to have a unique index.
        if (DB::connection()->getDriverName() === 'sqlite') {
            Schema::table('device', fn (Blueprint $table) => $table->unique('device_uid', 'device_uid_sqlite_unique'));
        }
    }

    public function down(): void
    {
        // Child foreign keys still depend on this index.
    }
};
