<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\DB;
use Illuminate\Support\Facades\Schema;

return new class extends Migration {
    public function up(): void
    {
        if (DB::connection()->getDriverName() === 'sqlite') {
            Schema::table('bill', function (Blueprint $table) {
                $table->enum('status', ['Paid', 'Partially Paid', 'Void', 'Pending', 'Due'])->default('Pending')->change();
            });
        }
    }

    public function down(): void
    {
        // Retain the valid status to preserve existing partial payments.
    }
};
