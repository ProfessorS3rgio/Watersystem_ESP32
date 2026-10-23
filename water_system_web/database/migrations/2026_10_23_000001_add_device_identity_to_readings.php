<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    public function up(): void
    {
        Schema::table('reading', function (Blueprint $table) {
            $table->unsignedBigInteger('device_reading_id')->nullable()->after('device_uid');
            $table->unique(['device_uid', 'device_reading_id'], 'reading_device_record_unique');
        });
    }

    public function down(): void
    {
        Schema::table('reading', function (Blueprint $table) {
            $table->dropUnique('reading_device_record_unique');
            $table->dropColumn('device_reading_id');
        });
    }
};
