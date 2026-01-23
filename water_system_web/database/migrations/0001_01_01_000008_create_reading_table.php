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
        Schema::create('reading', function (Blueprint $table) {
            $table->id('reading_id');
            $table->unsignedBigInteger('customer_id');
            $table->string('device_uid');
            $table->integer('previous_reading');
            $table->integer('current_reading');
            $table->integer('usage_m3');
            $table->dateTime('reading_at');
            $table->unsignedBigInteger('read_by_user_id')->nullable();
            $table->timestamps();

            $table->foreign('customer_id')->references('customer_id')->on('customer');
            $table->foreign('device_uid')->references('device_uid')->on('device');
            $table->foreign('read_by_user_id')->references('id')->on('users');
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('reading');
    }
};
