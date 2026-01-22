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
        Schema::create('customer', function (Blueprint $table) {
            $table->id('customer_id');
            $table->string('account_no')->unique();
            $table->unsignedBigInteger('type_id');
            $table->string('customer_name');
            $table->unsignedBigInteger('deduction_id')->nullable();
            $table->unsignedBigInteger('brgy_id');
            $table->string('address');
            $table->integer('previous_reading');
            $table->enum('status', ['active', 'disconnected'])->default('active');
            $table->boolean('Synced')->default(false);
            $table->timestamps();
            $table->datetime('last_sync')->nullable();

            $table->foreign('type_id')->references('type_id')->on('customer_type');
            $table->foreign('deduction_id')->references('deduction_id')->on('deduction');
            $table->foreign('brgy_id')->references('brgy_id')->on('barangay_sequence');
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('customer');
    }
};
