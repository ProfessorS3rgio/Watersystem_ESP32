<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    /**
     * Run the migrations.
     */
    public function up(): void
    {
        Schema::table('bill_transaction', function (Blueprint $table) {
            $table->string('processed_by_device_uid')->nullable()->after('processed_by_user_id');
            $table->dropForeign(['device_uid']);
            $table->dropColumn('device_uid');
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::table('bill_transaction', function (Blueprint $table) {
            $table->string('device_uid')->after('bill_reference_number');
            $table->foreign('device_uid')->references('device_uid')->on('device');
            $table->dropColumn('processed_by_device_uid');
        });
    }
};
