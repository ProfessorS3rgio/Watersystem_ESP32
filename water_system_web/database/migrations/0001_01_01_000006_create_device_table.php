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
        Schema::create('device', function (Blueprint $table) {
            $table->unsignedBigInteger('brgy_id');
            $table->string('device_mac')->unique();
            $table->string('device_uid');
            $table->string('firmware_version');
            $table->string('device_name')->nullable();
            $table->integer('print_count')->default(0);
            $table->integer('customer_count')->default(0);
            $table->datetime('last_sync')->nullable();
            $table->timestamps();

            $table->index('device_uid');
            $table->foreign('brgy_id')->references('brgy_id')->on('barangay_sequence');
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('device');
    }
};
