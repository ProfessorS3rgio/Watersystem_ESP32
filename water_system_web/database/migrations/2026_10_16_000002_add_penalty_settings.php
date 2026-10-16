<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    public function up(): void
    {
        Schema::table('settings', function (Blueprint $table) {
            $table->integer('penalty_after_days')->default(0)->after('disconnection_days');
            $table->decimal('penalty_amount', 10, 2)->default(0)->after('penalty_after_days');
        });
    }

    public function down(): void
    {
        Schema::table('settings', function (Blueprint $table) {
            $table->dropColumn(['penalty_after_days', 'penalty_amount']);
        });
    }
};
